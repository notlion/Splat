#pragma once

#include "cinder/Vector.h"

#pragma pack(push, 1)
struct Particle {
  ci::vec3 position;
  float scale;
  ci::vec4 color;
};
#pragma pack(pop)
