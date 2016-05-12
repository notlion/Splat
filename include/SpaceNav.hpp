#pragma once

#include "cinder/app/Window.h"

class SpaceNav {
  void *deviceHandle;

public:
  void init(const ci::app::Window &window);
  void poll();
};
