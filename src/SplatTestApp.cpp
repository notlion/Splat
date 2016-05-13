#include "cinder/Log.h"
#include "cinder/Rand.h"
#include "cinder/Vector.h"
#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

#include "3DConnexion.h"
#include "BodyCam.hpp"

using namespace ci;
using namespace ci::app;

static const size_t kMaxParticles = glm::pow(2 << 9, 2);

class SplatTestApp : public App {
  gl::GlslProgRef particleSimProg;
  gl::BatchRef particleBatch;
  gl::TextureRef particleTexture;

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
};

void SplatTestApp::setup() {
  camera.setEyePoint(vec3(0.0f, 0.0f, -5.0f));
  camera.setViewDirection(vec3(0.0f, 0.0f, 1.0f));

  cameraBody = Body3(camera);

  {
    connexion::Device::initialize(getRenderer()->getHwnd());
    auto id = connexion::Device::getAllDeviceIds().front();
    spaceNav = connexion::Device::create(id);
    spaceNav->getMotionSignal().connect([this](const connexion::MotionEvent &event) {
      cameraTranslation = event.translation;
      cameraRotation = event.rotation;
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

  {
    auto fmt = gl::Texture::Format().mipmap();
    particleTexture = gl::Texture::create(loadImage(loadAsset("splat_0.png")), fmt);
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

  {
    const auto &ori = cameraBody.orientation;
    cameraBody.impulse = glm::rotate(ori, cameraTranslation * vec3(1, 1, -1)) * 0.000015f;
    cameraBody.angularImpulse = quat(cameraRotation * vec3(1, 1, -1) * 0.000005f);
  }

  cameraBody.step();
  cameraBody.applyTransform(camera);
}

void SplatTestApp::draw() {
  gl::clear(Color(0, 0, 0));
  gl::enableDepth(false);
  gl::enableAlphaBlendingPremult();
  gl::enable(GL_PROGRAM_POINT_SIZE);

  gl::setMatrices(camera);

  gl::ScopedTextureBind scopedTex(particleTexture);
  particleBatch->getGlslProg()->uniform("texture", 0);
  particleBatch->getGlslProg()->uniform("time", float(getElapsedSeconds()));
  particleBatch->draw();
}

CINDER_APP(SplatTestApp, RendererGl)
