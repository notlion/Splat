#include "cinder/Log.h"
#include "cinder/Rand.h"
#include "cinder/Vector.h"
#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

#include "3DConnexion.h"
#include "BodyCam.hpp"
#include "Sort.hpp"

using namespace ci;
using namespace ci::app;

static constexpr uint32_t sqr(uint32_t x) {
  return x * x;
}

static const uint32_t kMaxParticles = sqr(2 << 10);
static const uint32_t kWorkGroupSizeX = 64;


class SplatTestApp : public App {
  gl::GlslProgRef particleSimProg;
  gl::BatchRef particleBatch;
  gl::TextureRef particleTexture;
  gl::BufferObjRef particlePositions, particlePositionsPrev;

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
    particleSimProg = gl::GlslProg::create(fmt);
  }

  {
    auto progFmt = gl::GlslProg::Format()
                       .vertex(loadAsset("render_vs.glsl"))
                       .fragment(loadAsset("render_fs.glsl"));
    auto prog = gl::GlslProg::create(progFmt);

    auto meshLayout =
        gl::VboMesh::Layout().usage(GL_DYNAMIC_DRAW).attrib(geom::Attrib::POSITION, 4);
    auto mesh = gl::VboMesh::create(kMaxParticles, GL_POINTS, {meshLayout});

    {
      auto positions = std::unique_ptr<vec4[]>(new vec4[kMaxParticles]);
      for (size_t i = 0; i < kMaxParticles; ++i) {
        positions[i] = vec4(ci::Rand::randVec3(), 0.0f);
      }

      auto bufferSize = kMaxParticles * sizeof(vec4);
      particlePositions = gl::BufferObj::create(GL_SHADER_STORAGE_BUFFER, bufferSize,
                                                positions.get(), GL_DYNAMIC_COPY);
      particlePositionsPrev = gl::BufferObj::create(GL_SHADER_STORAGE_BUFFER, bufferSize,
                                                    positions.get(), GL_DYNAMIC_COPY);
    }

    particleBatch = gl::Batch::create(mesh, prog);
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
    cameraBody.impulse = glm::rotate(ori, cameraTranslation * vec3(1, 1, -1)) * 0.00001f;
    cameraBody.angularImpulse = quat(cameraRotation * vec3(1, 1, -1) * 0.00000333f);
  }

  cameraBody.step();
  cameraBody.applyTransform(camera);

  {
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, particlePositions->getId());

    particleSimProg->bind();
    particleSimProg->uniform("time", float(getElapsedSeconds()));

    glDispatchCompute(kMaxParticles / kWorkGroupSizeX, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);
  }

  {
    auto outputAttrib = particleBatch->getVboMesh()->findAttrib(geom::Attrib::POSITION);
    if (outputAttrib) {
      radixSort->sort(particlePositions->getId(), outputAttrib->second->getId(),
                      -camera.getViewDirection(), -2.0f, 2.0f);
    }
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
    particleBatch->getGlslProg()->uniform("texture", 0);
    particleBatch->getGlslProg()->uniform("pointSize", getWindowHeight() / 100.0f);
    particleBatch->getGlslProg()->uniform("time", float(getElapsedSeconds()));
    particleBatch->draw();
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
