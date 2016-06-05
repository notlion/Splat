#pragma once

#include "Particle.hpp"
#include "Sort.hpp"

#include "cinder/AxisAlignedBox.h"
#include "cinder/Filesystem.h"
#include "cinder/gl/gl.h"

namespace splat {

using namespace ci;

struct ParticleSys {
  gl::TextureRef particleTexture;

  gl::GlslProgRef particleUpdateProg, particleRenderProg;
  gl::VboRef particleIds;
  gl::VaoRef particleAttrs;
  gl::SsboRef particles, particlesPrev, particlesSorted;

  gl::Texture3dRef densityTexture, densityGradTexture;
  gl::GlslProgRef densityAccumProg, densityGradProg, densityDebugRenderProg;

  RadixSortRef radixSort;

  AxisAlignedBox volumeBounds;
  uvec3 volumeRes;

  bool shaderInit, shaderCompile;

  ParticleSys();

  void update(float time, uint32_t frameId, const vec3 &eyePos, const vec3 &eyeVel,
              const vec3 &viewDirection);
  void draw(float pointSize);

  void loadUpdateShaderMain(const fs::path &filepath);
};

} // splat
