#include "BodyCam.hpp"

#include "cinder/Log.h"

using namespace ci;

void Body3::step() {
  {
    auto vel = position - positionPrev;
    positionPrev = position;

    impulse += vel * (1.0f - friction);

    position += impulse;
    impulse = vec3(0.0f);
  }

  {
    auto vel = orientation * glm::inverse(orientationPrev);
    orientationPrev = orientation;

    orientation = glm::slerp(vel, quat(), angularFriction) * orientation;
    orientation *= angularImpulse;
    angularImpulse = quat();
  }
}

void Body3::stopAt(const Camera &camera) {
  position = positionPrev = camera.getEyePoint();
  orientation = orientationPrev = camera.getOrientation();
}

void Body3::applyTransform(ci::Camera &camera) {
  camera.setEyePoint(position);
  camera.setOrientation(orientation);
}
