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

static const uint32_t kMaxParticles = sqr(2 << 9);

class SplatTestApp : public App {
  gl::GlslProgRef particleSimProg;
  gl::BatchRef particleBatch;
  gl::TextureRef particleTexture;
  gl::BufferObjRef particlePositionBuffer;

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

  radixSort = std::make_shared<RadixSort>(kMaxParticles, 128);

  {
    auto positions = std::unique_ptr<vec4[]>(new vec4[kMaxParticles]);
    for (size_t i = 0; i < kMaxParticles; ++i) {
      positions[i] = vec4(ci::Rand::randVec3(), 0.0f);
    }
    particlePositionBuffer = gl::BufferObj::create(
        GL_SHADER_STORAGE_BUFFER, kMaxParticles * sizeof(vec4), positions.get(), GL_DYNAMIC_COPY);
  }

  {
    auto progFmt = gl::GlslProg::Format()
                       .vertex(loadAsset("render_vs.glsl"))
                       .fragment(loadAsset("render_fs.glsl"));
    auto prog = gl::GlslProg::create(progFmt);

    auto meshLayout =
        gl::VboMesh::Layout().usage(GL_DYNAMIC_DRAW).attrib(geom::Attrib::POSITION, 4);
    auto mesh = gl::VboMesh::create(kMaxParticles, GL_POINTS, {meshLayout});
    mesh->bufferAttrib(geom::Attrib::POSITION, kMaxParticles * sizeof(vec4), nullptr);

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
    cameraBody.impulse = glm::rotate(ori, cameraTranslation * vec3(1, 1, -1)) * 0.00001f;
    cameraBody.angularImpulse = quat(cameraRotation * vec3(1, 1, -1) * 0.00000333f);
  }

  cameraBody.step();
  cameraBody.applyTransform(camera);

  {
    auto inputId = particlePositionBuffer->getId();
    auto outputAttrib = particleBatch->getVboMesh()->findAttrib(geom::Attrib::POSITION);
    if (outputAttrib) {
      auto outputId = outputAttrib->second->getId();
      radixSort->sort(inputId, outputId, -camera.getViewDirection(), camera.getNearClip(),
                      camera.getFarClip());
    }
  }
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
