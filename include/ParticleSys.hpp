#pragma once

#include "Particle.hpp"
#include "Sort.hpp"

#include "cinder/Filesystem.h"
#include "cinder/gl/gl.h"
#include "cinder/AxisAlignedBox.h"

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

  ParticleSys();

  void update(float time, uint32_t frameId, const vec3 &eyePos, const vec3 &viewDirection);
  void draw(float pointSize);

  void loadUpdateShaderMain(const fs::path &filepath);
};

} // splat
