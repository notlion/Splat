#include "cinder/AxisAlignedBox.h"
#include "cinder/Log.h"
#include "cinder/Rand.h"
#include "cinder/Utilities.h"
#include "cinder/Vector.h"
#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/Ssbo.h"
#include "cinder/gl/gl.h"

#include "3DConnexion.h"
#include "CinderImGui.h"
#include "Watchdog.h"

#include "BodyCam.hpp"
#include "ParticleSys.hpp"
#include "Utils.hpp"


using namespace ci;
using namespace ci::app;


namespace splat {


class SplatTestApp : public App {
  std::unique_ptr<ParticleSys> particleSys;
  fs::path particleUpdateMainFilepath;

  connexion::DeviceRef spaceNav;

  CameraPersp camera;
  Body3 cameraBody;
  vec3 cameraTranslation, cameraRotation;

  std::string updateShaderError;
  bool renderDebugGraphics = false;

public:
  void setup() override;
  void cleanup() override;
  void resize() override;
  void update() override;
  void draw() override;
  void keyDown(KeyEvent event) override;

  void updateGui();
  void resetCamera();
};


void SplatTestApp::resetCamera() {
  camera.setEyePoint(vec3(0.0f, 0.0f, -5.0f));
  camera.setViewDirection(vec3(0.0f, 0.0f, 1.0f));
  cameraBody.stopAt(camera);
}

void SplatTestApp::setup() {
  resetCamera();

  {
    connexion::Device::initialize(getRenderer()->getHwnd());
    const auto &ids = connexion::Device::getAllDeviceIds();
    if (!ids.empty()) {
      spaceNav = connexion::Device::create(ids.front());
      spaceNav->getMotionSignal().connect([this](const connexion::MotionEvent &event) {
        cameraTranslation = event.translation;
        cameraRotation = event.rotation;
      });
      spaceNav->getButtonDownSignal().connect(
          [this](const connexion::ButtonDownEvent &event) { resetCamera(); });
    }
  }

  particleSys = std::make_unique<ParticleSys>();
  particleUpdateMainFilepath = getAssetPath("update_cs.glsl");

  wd::watch(particleUpdateMainFilepath, [this](const fs::path &filepath) {
    updateShaderError.clear();
    try {
      particleSys->loadUpdateShaderMain(filepath);
    } catch (const gl::GlslProgCompileExc &exc) {
      updateShaderError = exc.what();
    }
  });

  ui::initialize(ui::Options().autoRender(false));
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

  particleSys->update(getElapsedSeconds(), getElapsedFrames(), cameraBody.position,
                      cameraBody.position - cameraBody.positionPrev, camera.getViewDirection());

  updateGui();
}


void SplatTestApp::updateGui() {
  ui::NewFrame();
  ui::ScopedWindow scopedWindow("Hello");

  if (ui::CollapsingHeader("Display")) {
    ui::Checkbox("Render Debug Graphics", &renderDebugGraphics);
  }

  if (ui::CollapsingHeader("Shader Status")) {
    if (updateShaderError.empty()) {
      ui::TextUnformatted("Compile Successful");
    } else {
      ui::TextUnformatted(updateShaderError.c_str());
    }
  }
}


void SplatTestApp::draw() {
  gl::clear(Color(0.0f, 0.0f, 0.0f));

  gl::setMatrices(camera);

  if (renderDebugGraphics) {
    gl::enableDepth();
    gl::disableAlphaBlending();
    gl::color(ColorAf(0, 0, 1, 0.5f));
    gl::bindStockShader(gl::ShaderDef().color());
    gl::drawStrokedCube(particleSys->volumeBounds);
    gl::drawCoordinateFrame(0.25f, 0.05f, 0.01f);
  }

  gl::enableDepthRead();
  gl::disableDepthWrite();
  gl::enableAlphaBlendingPremult();
  gl::enable(GL_PROGRAM_POINT_SIZE);

  particleSys->draw(getWindowHeight() / 100.0f);

  if (!isFullScreen()) {
    ui::Render();
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
  switch (event.getChar()) {
    case 's': {
      saveGrab(copyWindowSurface(), getAppPath().parent_path() / "grabs");
    } break;
  }
}


} // splat


static void prepareSettings(App::Settings *settings) {
  settings->setWindowSize(1280, 720);
}

CINDER_APP(splat::SplatTestApp, RendererGl, prepareSettings)
