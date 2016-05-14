#pragma once

#include "cinder/gl/GlslProg.h"

#include <vector>

class RadixSort {
  ci::gl::GlslProgRef scanProg, scanFirstProg, resolveProg, reorderProg;

  ci::gl::BufferObjRef flagsBuffer;
  std::vector<ci::gl::BufferObjRef> scanBuffers, sumBuffers;

  uint32_t elemCount, blockSize, scanLevelCount;

  void sortBits(GLuint inputBufId, GLuint outputBufId, int bitOffset, const ci::vec3 &axis,
                float zMin, float zMax);

public:
  RadixSort(uint32_t elemCount, uint32_t blockSize);

  void sort(GLuint inputBufId, GLuint outputBufId, const ci::vec3 &axis, float zMin, float zMax);
};
