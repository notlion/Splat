#include "cinder/Log.h"
#include "cinder/Rand.h"
#include "cinder/Vector.h"
#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/Ssbo.h"
#include "cinder/gl/gl.h"
#include "cinder/AxisAlignedBox.h"

#include "3DConnexion.h"
#include "Watchdog.h"

#include "BodyCam.hpp"
#include "ParticleSys.hpp"


using namespace ci;
using namespace ci::app;


namespace splat {


class SplatTestApp : public App {
  std::unique_ptr<ParticleSys> particleSys;
  fs::path particleUpdateMainFilepath;

  AxisAlignedBox particleSysBounds;

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

  particleSysBounds.set(vec3(-2.0f), vec3(2.0f));

  particleSys = std::make_unique<ParticleSys>();
  particleUpdateMainFilepath = getAssetPath("update_cs.glsl");

  wd::watch(particleUpdateMainFilepath, [this](const fs::path &filepath) {
    CI_LOG_D("Reloading update shader: " << filepath);
    try {
      particleSys->loadUpdateShaderMain(filepath);
    } catch (const gl::GlslProgCompileExc &exc) {
      CI_LOG_D(exc.what());
    }
  });
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

  particleSys->update(getElapsedSeconds(), getElapsedFrames(), camera.getViewDirection());
}

void SplatTestApp::draw() {
  gl::clear(Color(0, 0, 0));

  gl::enableDepth(false);
  gl::enableAlphaBlendingPremult();
  gl::enable(GL_PROGRAM_POINT_SIZE);

  gl::setMatrices(camera);

  particleSys->draw(getWindowHeight() / 100.0f);
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


} // splat


static void prepareSettings(App::Settings *settings) {
  settings->setWindowSize(1280, 720);
}

CINDER_APP(splat::SplatTestApp, RendererGl, prepareSettings)
