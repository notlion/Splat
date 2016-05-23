#include "cinder/Log.h"
#include "cinder/Rand.h"
#include "cinder/Vector.h"
#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/Ssbo.h"
#include "cinder/gl/gl.h"

#include "3DConnexion.h"
#include "BodyCam.hpp"
#include "Particle.hpp"
#include "Sort.hpp"

#include <numeric>

using namespace ci;
using namespace ci::app;

static constexpr uint32_t sqr(uint32_t x) {
  return x * x;
}

static const uint32_t kMaxParticles = sqr(2 << 9);
static const uint32_t kWorkGroupSizeX = 128;


class SplatTestApp : public App {
  gl::TextureRef particleTexture;

  gl::GlslProgRef particleUpdateProg, particleRenderProg;
  gl::VboRef particleIds;
  gl::VaoRef particleAttrs;
  gl::SsboRef particles, particlesSorted;

  std::shared_ptr<RadixSort> radixSort;

  connexion::DeviceRef spaceNav;

  CameraPersp camera;
  Body3 cameraBody;
  vec3 cameraTranslation, cameraRotation;

public:
  void setup() override;
  void cleanup() override;
  void resize() override;
  void update() override;
  void draw() override;
  void keyDown(KeyEvent event) override;
};


void SplatTestApp::setup() {
  camera.setEyePoint(vec3(0.0f, 0.0f, -5.0f));
  camera.setViewDirection(vec3(0.0f, 0.0f, 1.0f));

  cameraBody = Body3(camera);

  {
    connexion::Device::initialize(getRenderer()->getHwnd());
    const auto &ids = connexion::Device::getAllDeviceIds();
    if (!ids.empty()) {
      spaceNav = connexion::Device::create(ids.front());
      spaceNav->getMotionSignal().connect([this](const connexion::MotionEvent &event) {
        cameraTranslation = event.translation;
        cameraRotation = event.rotation;
      });
    }
  }

  radixSort = std::make_shared<RadixSort>(kMaxParticles, 128);

  {
    auto fmt = gl::Texture::Format().mipmap();
    particleTexture = gl::Texture::create(loadImage(loadAsset("splat_0.png")), fmt);
  }

  {
    auto fmt = gl::GlslProg::Format()
                   .compute(loadAsset("step_cs.glsl"))
                   .preprocess(true)
                   .define("WORK_GROUP_SIZE_X", std::to_string(kWorkGroupSizeX))
                   .define("PARTICLE_COUNT", std::to_string(kMaxParticles));
    particleUpdateProg = gl::GlslProg::create(fmt);
  }

  {
    auto fmt = gl::GlslProg::Format()
                   .vertex(loadAsset("render_vs.glsl"))
                   .fragment(loadAsset("render_fs.glsl"))
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
      initParticles[i] = {ci::Rand::randVec3(), 1.0f, vec4(1.0)};
    }

    auto bufferSize = kMaxParticles * sizeof(Particle);
    particles = gl::Ssbo::create(bufferSize, initParticles.get(), GL_STATIC_DRAW);
    particlesSorted = gl::Ssbo::create(bufferSize, nullptr, GL_STATIC_DRAW);
  }
}

void SplatTestApp::cleanup() {
  connexion::Device::shutdown();
}


void SplatTestApp::resize() {
  camera.setAspectRatio(getWindowAspectRatio());
}


void SplatTestApp::update() {
  if (spaceNav) {
    spaceNav->update();
  }

  {
    const auto &ori = cameraBody.orientation;
    cameraBody.impulse = glm::rotate(ori, cameraTranslation * vec3(1, 1, -1)) * 0.000005f;
    cameraBody.angularImpulse = quat(cameraRotation * vec3(1, 1, -1) * 0.00000333f);
  }

  cameraBody.step();
  cameraBody.applyTransform(camera);

  {
    particles->bindBase(0);

    particleUpdateProg->bind();
    particleUpdateProg->uniform("time", float(getElapsedSeconds()));

    glDispatchCompute(kMaxParticles / kWorkGroupSizeX, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    particles->unbindBase();
  }

  {
    radixSort->sort(particles->getId(), particlesSorted->getId(), -camera.getViewDirection(), -2.0f,
                    2.0f);
  }
}

void SplatTestApp::draw() {
  gl::clear(Color(0, 0, 0));

  gl::enableDepth(false);
  gl::enableAlphaBlendingPremult();
  gl::enable(GL_PROGRAM_POINT_SIZE);

  gl::setMatrices(camera);

  {
    gl::ScopedTextureBind scopedTex(particleTexture);
    gl::ScopedGlslProg scopedProg(particleRenderProg);
    gl::ScopedVao scopedVao(particleAttrs);

    gl::context()->setDefaultShaderVars();

    particleRenderProg->uniform("texture", 0);
    particleRenderProg->uniform("pointSize", getWindowHeight() / 100.0f);

    particlesSorted->bindBase(0);
    gl::drawArrays(GL_POINTS, 0, kMaxParticles);
    particlesSorted->unbindBase();
  }
}


void SplatTestApp::keyDown(KeyEvent event) {
  switch (event.getCode()) {
    case KeyEvent::KEY_ESCAPE: {
      setFullScreen(!isFullScreen());
      if (isFullScreen())
        hideCursor();
      else
        showCursor();
    } break;
  }
}


static void prepareSettings(App::Settings *settings) {
  settings->setWindowSize(1280, 720);
}

CINDER_APP(SplatTestApp, RendererGl, prepareSettings)
