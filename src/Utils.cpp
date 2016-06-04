#include "Utils.hpp"

#include "cinder/ImageIo.h"
#include "cinder/Utilities.h"

#include "Watchdog.h"

namespace splat {

bool startsWith(const std::string &str, const std::string &prefix) {
  if (str.length() < prefix.length() || str.empty() || prefix.empty()) return false;
  size_t i = 0;
  while (i < prefix.length() && str[i] == prefix[i]) ++i;
  return i == prefix.length();
}

int firstIndexOf(const std::string &str, char c) {
  auto it = std::find(str.begin(), str.end(), c);
  if (it == str.end()) return -1;
  return it - str.begin();
}


fs::path saveGrab(const Surface &surf, const fs::path &grabsDirPath) {
  const std::string prefix = "grab_";
  int topId = -1;

  for (auto &p : fs::directory_iterator(grabsDirPath)) {
    const auto &path = p.path();
    const auto &filename = path.filename().string();

    if (startsWith(filename, prefix)) {
      int index = firstIndexOf(filename, '.');
      if (index >= 0) {
        try {
          int id = boost::lexical_cast<int>(&filename[prefix.length()], index - prefix.length());
          topId = std::max(topId, id);
        } catch (const boost::bad_lexical_cast &exc) {
        }
      }
    }
  }

  auto grabPath = grabsDirPath / (prefix + std::to_string(topId + 1) + ".png");
  auto opts = ImageTarget::Options();
  writeImage(grabPath, surf, opts);

  return grabPath;
}

} // splat
