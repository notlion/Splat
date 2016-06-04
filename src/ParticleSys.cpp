#include "ParticleSys.hpp"

#include "cinder/ImageIo.h"
#include "cinder/Rand.h"
#include "cinder/Utilities.h"
#include "cinder/app/App.h"
#include "glm/gtx/extented_min_max.hpp"

#include <numeric>

namespace splat {

static constexpr uint32_t sqr(uint32_t x) {
  return x * x;
}

static const uint32_t kMaxParticles = sqr(2 << 9);
static const uint32_t kWorkGroupSizeX = 128;
static const uint32_t kVolumeGroupSizeXYZ = 8;


ParticleSys::ParticleSys() : shaderInit(true) {
  volumeBounds.set(vec3(-2.0f), vec3(2.0f));
  volumeRes = uvec3(64);

  radixSort = std::make_shared<RadixSort>(kMaxParticles, 128);

  {
    auto fmt = gl::Texture::Format().mipmap();
    particleTexture = gl::Texture::create(loadImage(app::loadAsset("splat_0.png")), fmt);
  }

  {
    auto fmt = gl::GlslProg::Format()
                   .vertex(app::loadAsset("render_vs.glsl"))
                   .fragment(app::loadAsset("render_fs.glsl"))
                   .attribLocation("particleId", 0);
    particleRenderProg = gl::GlslProg::create(fmt);
  }

  {
    std::vector<GLuint> ids(kMaxParticles);
    std::iota(ids.begin(), ids.end(), 0);
    particleIds = gl::Vbo::create(GL_ARRAY_BUFFER, ids, GL_STATIC_DRAW);

    particleAttrs = gl::Vao::create();
    gl::ScopedVao scopedVao(particleAttrs);
    gl::ScopedBuffer scopedIds(particleIds);
    gl::enableVertexAttribArray(0);
    gl::vertexAttribIPointer(0, 1, GL_UNSIGNED_INT, sizeof(GLuint), nullptr);
  }

  {
    auto initParticles = std::unique_ptr<Particle[]>(new Particle[kMaxParticles]);
    for (size_t i = 0; i < kMaxParticles; ++i) {
      initParticles[i] = {vec3(0.0f), 1.0f, vec4(0.0f)}; // Rand::randVec3(), 1.0f, vec4(1.0)
    }

    auto bufferSize = kMaxParticles * sizeof(Particle);
    particles = gl::Ssbo::create(bufferSize, initParticles.get(), GL_STATIC_DRAW);
    particlesPrev = gl::Ssbo::create(bufferSize, initParticles.get(), GL_STATIC_DRAW);
    particlesSorted = gl::Ssbo::create(bufferSize, nullptr, GL_STATIC_DRAW);
  }

  {
    auto fmt = gl::Texture3d::Format()
                   .immutableStorage()
                   .internalFormat(GL_R32UI)
                   .minFilter(GL_LINEAR)
                   .magFilter(GL_LINEAR);
    // NOTE(ryan): Max mipmap level must be specified. Cinder will not automatically calculate for
    // 3d textures. Probably a bug?
    fmt.setMaxMipmapLevel(0);

    densityTexture = gl::Texture3d::create(volumeRes.x, volumeRes.y, volumeRes.z, fmt);

    fmt.setInternalFormat(GL_RGBA16F); // TODO(ryan): Maybe use GL_RGBA16_SNORM?
    densityGradTexture = gl::Texture3d::create(volumeRes.x, volumeRes.y, volumeRes.z, fmt);
  }

  {
    auto fmt = gl::GlslProg::Format().preprocess(true).define("WORK_GROUP_SIZE_X",
                                                              std::to_string(kWorkGroupSizeX));
    densityAccumProg = gl::GlslProg::create(fmt.compute(app::loadAsset("density_accum_cs.glsl")));
  }

  {
    auto fmt = gl::GlslProg::Format().preprocess(true).define("WORK_GROUP_SIZE_XYZ",
                                                              std::to_string(kVolumeGroupSizeXYZ));
    densityGradProg = gl::GlslProg::create(fmt.compute(app::loadAsset("density_grad_cs.glsl")));
  }
}


void ParticleSys::update(float time, uint32_t frameId, const vec3 &eyePos, const vec3 &viewDir) {
  mat4 worldToVolumeMtx = glm::translate(glm::scale(vec3(volumeRes) / vec3(volumeBounds.getSize())),
                                         -volumeBounds.getMin());
  mat4 worldToUnitVolumeMtx =
      glm::translate(glm::scale(vec3(1.0f) / vec3(volumeBounds.getSize())), -volumeBounds.getMin());

  if (particleUpdateProg) {
    particleUpdateProg->bind();
    particleUpdateProg->uniform("time", time);
    particleUpdateProg->uniform("frameId", frameId);
    particleUpdateProg->uniform("eyePos", eyePos);
    particleUpdateProg->uniform("viewDir", viewDir);

    particleUpdateProg->uniform("init", shaderInit);
    particleUpdateProg->uniform("compile", shaderCompile);

    particleUpdateProg->uniform("volumeBoundsMin", volumeBounds.getMin());
    particleUpdateProg->uniform("volumeBoundsMax", volumeBounds.getMax());
    particleUpdateProg->uniform("volumeRes", volumeRes);
    particleUpdateProg->uniform("worldToUnitVolumeMtx", worldToUnitVolumeMtx);

    particleUpdateProg->uniform("densityGradTex", 0);
    particleUpdateProg->uniform("densityTex", 1);
    gl::ScopedTextureBind scopedDensityGradTex(densityGradTexture, 0);
    gl::ScopedTextureBind scopedDensityTex(densityTexture, 1);

    particles->bindBase(0);
    particlesPrev->bindBase(1);

    glDispatchCompute(kMaxParticles / kWorkGroupSizeX, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    particlesPrev->unbindBase();
    particles->unbindBase();

    shaderInit = false;
    shaderCompile = false;
  }

  // NOTE(ryan): Accumulate particles into the density texture.
  {
    glClearTexImage(densityTexture->getId(), 0, GL_RED_INTEGER, GL_UNSIGNED_INT, nullptr);

    densityAccumProg->bind();
    densityAccumProg->uniform("volumeRes", volumeRes);
    densityAccumProg->uniform("worldToVolumeMtx", worldToVolumeMtx);
    // densityAccumProg->uniform("boundsMin", volumeBounds.getMin());
    // densityAccumProg->uniform("oneOverBoundsSize", vec3(1.0f) / volumeBounds.getSize());

    vec3 celSize = vec3(volumeBounds.getSize()) / vec3(volumeRes);
    float celScale = glm::min(celSize.x, celSize.y, celSize.z);
    densityAccumProg->uniform("oneOverCelScale", 1.0f / celScale);

    particles->bindBase(0);
    glBindImageTexture(1, densityTexture->getId(), 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);

    glDispatchCompute(kMaxParticles / kWorkGroupSizeX, 1, 1);
    glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);

    particles->unbindBase();
  }

  // NOTE(ryan): Compute density gradients.
  {
    densityGradProg->bind();
    densityGradProg->uniform("volumeRes", volumeRes);

    glBindImageTexture(0, densityTexture->getId(), 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32UI);
    glBindImageTexture(1, densityGradTexture->getId(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);

    glDispatchCompute(volumeRes.x / kVolumeGroupSizeXYZ, volumeRes.y / kVolumeGroupSizeXYZ,
                      volumeRes.z / kVolumeGroupSizeXYZ);
    glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);
  }

  radixSort->sort(particles->getId(), particlesSorted->getId(), -viewDir, -2.0f, 2.0f);
}

void ParticleSys::draw(float pointSize) {
  gl::ScopedTextureBind scopedTex(particleTexture);
  gl::ScopedGlslProg scopedProg(particleRenderProg);
  gl::ScopedVao scopedVao(particleAttrs);

  gl::context()->setDefaultShaderVars();

  particleRenderProg->uniform("texture", 0);
  particleRenderProg->uniform("pointSize", pointSize);

  particlesSorted->bindBase(0);
  gl::drawArrays(GL_POINTS, 0, kMaxParticles);
  particlesSorted->unbindBase();
}

void ParticleSys::loadUpdateShaderMain(const fs::path &filepath) {
  auto fmt = gl::GlslProg::Format()
                 .compute(loadFile(filepath))
                 .preprocess(true)
                 .define("WORK_GROUP_SIZE_X", std::to_string(kWorkGroupSizeX))
                 .define("PARTICLE_COUNT", std::to_string(kMaxParticles));
  auto updateProg = gl::GlslProg::create(fmt);

  particleUpdateProg = updateProg;
  shaderCompile = true;
}

} // splat
