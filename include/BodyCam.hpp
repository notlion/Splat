#pragma once

#include "cinder/Camera.h"
#include "cinder/Quaternion.h"
#include "cinder/Vector.h"

struct Body3 {
  ci::vec3 position, positionPrev;
  ci::vec3 impulse;

  ci::quat orientation, orientationPrev;
  ci::quat angularImpulse;

  float friction = 0.02f;
  float angularFriction = 0.02f;

  Body3() = default;
  Body3(const ci::vec3 &pos, const ci::quat &ori)
  : position{pos}, positionPrev{pos}, orientation{ori}, orientationPrev{ori} {}
  Body3(const ci::Camera &camera) : Body3{camera.getEyePoint(), camera.getOrientation()} {}

  void step();

  void applyTransform(ci::Camera &camera);
};
