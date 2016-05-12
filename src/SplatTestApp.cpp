#include "cinder/CameraUi.h"
#include "cinder/Log.h"
#include "cinder/Rand.h"
#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

#include "3DConnexion.h"

using namespace ci;
using namespace ci::app;

static const size_t kMaxParticles = 1024 * 1024;

class SplatTestApp : public App {
  gl::GlslProgRef particleSimProg;
  gl::BatchRef particleBatch;

  connexion::DeviceRef spaceNav;

  CameraPersp camera;

public:
  void setup() override;
  void cleanup() override;
  void resize() override;
  void update() override;
  void draw() override;
};

void SplatTestApp::setup() {
  camera.setEyePoint(vec3(0.0f, 0.0f, -5.0f));
  camera.setViewDirection(vec3(0.0f, 0.0f, 1.0f));

  {
    connexion::Device::initialize(getRenderer()->getHwnd());
    auto id = connexion::Device::getAllDeviceIds().front();
    spaceNav = connexion::Device::create(id);
    spaceNav->getMotionSignal().connect([=](const connexion::MotionEvent &event) {
      auto ori = camera.getOrientation();
      auto pos = camera.getEyePoint();

      pos += glm::rotate(ori, event.translation * (vec3(1, 1, -1) * 0.00015f));
      ori *= quat(event.rotation * (vec3(1, 1, -1) * 0.0001f));

      camera.setEyePoint(pos);
      camera.setOrientation(ori);
    });
  }

  {
    auto progFmt = gl::GlslProg::Format()
                       .vertex(loadAsset("render_vs.glsl"))
                       .fragment(loadAsset("render_fs.glsl"));
    auto prog = gl::GlslProg::create(progFmt);

    auto meshLayout =
        gl::VboMesh::Layout().usage(GL_DYNAMIC_DRAW).attrib(geom::Attrib::POSITION, 4);
    auto mesh = gl::VboMesh::create(kMaxParticles, GL_POINTS, {meshLayout});

    auto positions = std::unique_ptr<vec4[]>(new vec4[kMaxParticles]);
    for (size_t i = 0; i < kMaxParticles; ++i) {
      positions[i] = vec4(ci::Rand::randVec3(), 0.0f);
    }
    mesh->bufferAttrib(geom::Attrib::POSITION, kMaxParticles * sizeof(vec4), positions.get());

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
  spaceNav->update();
}

void SplatTestApp::draw() {
  gl::clear(Color(0, 0, 0));

  gl::setMatrices(camera);

  particleBatch->getGlslProg()->uniform("time", float(getElapsedSeconds()));
  particleBatch->draw();
}

CINDER_APP(SplatTestApp, RendererGl)
