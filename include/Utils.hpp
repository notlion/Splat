#pragma once

#include "cinder/Filesystem.h"
#include "cinder/Surface.h"

namespace splat {

using namespace ci;

bool startsWith(const std::string &str, const std::string &prefix);
int firstIndexOf(const std::string &str, char c);

fs::path saveGrab(const Surface &surf, const fs::path &grabsDirPath);

} // splat
