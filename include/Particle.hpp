#pragma once

#include "cinder/Vector.h"

namespace splat {

using namespace ci;

#pragma pack(push, 1)
struct Particle {
  vec3 position;
  float scale;
  vec4 color;
};
#pragma pack(pop)

} // splat
