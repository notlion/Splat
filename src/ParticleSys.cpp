#include "ParticleSys.hpp"

#include "cinder/ImageIo.h"
#include "cinder/Rand.h"
#include "cinder/Utilities.h"
#include "cinder/app/App.h"

#include <numeric>

namespace splat {

static constexpr uint32_t sqr(uint32_t x) {
  return x * x;
}

static const uint32_t kMaxParticles = sqr(2 << 9);
static const uint32_t kWorkGroupSizeX = 128;


ParticleSys::ParticleSys() {
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
      initParticles[i] = {Rand::randVec3(), 1.0f, vec4(1.0)};
    }

    auto bufferSize = kMaxParticles * sizeof(Particle);
    particles = gl::Ssbo::create(bufferSize, initParticles.get(), GL_STATIC_DRAW);
    particlesPrev = gl::Ssbo::create(bufferSize, initParticles.get(), GL_STATIC_DRAW);
    particlesSorted = gl::Ssbo::create(bufferSize, nullptr, GL_STATIC_DRAW);
  }
}


void ParticleSys::update(float time, uint32_t frameId, const vec3 &viewDirection) {
  if (particleUpdateProg) {
    // std::swap(particles, particlesPrev);

    particles->bindBase(0);
    particlesPrev->bindBase(1);

    particleUpdateProg->bind();
    particleUpdateProg->uniform("time", time);
    particleUpdateProg->uniform("frameId", frameId);

    glDispatchCompute(kMaxParticles / kWorkGroupSizeX, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    particlesPrev->unbindBase();
    particles->unbindBase();
  }

  radixSort->sort(particles->getId(), particlesSorted->getId(), -viewDirection, -2.0f, 2.0f);
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
  auto src = loadString(app::loadAsset("step_cs.glsl"));
  auto fmt = gl::GlslProg::Format()
                 .compute(src + "\n#include \"" + filepath.string() + "\"")
                 .preprocess(true)
                 .define("WORK_GROUP_SIZE_X", std::to_string(kWorkGroupSizeX))
                 .define("PARTICLE_COUNT", std::to_string(kMaxParticles));
  auto updateProg = gl::GlslProg::create(fmt);

  particleUpdateProg = updateProg;
}

} // splat
