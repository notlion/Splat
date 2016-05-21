#pragma once

#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Ssbo.h"

#include <vector>

class RadixSort {
public:
  ci::gl::GlslProgRef scanProg, scanFirstProg, resolveProg, reorderProg;

  ci::gl::SsboRef sortedBuffer, flagsBuffer;
  std::vector<ci::gl::SsboRef> scanBuffers, sumBuffers;

  uint32_t elemCount, blockSize, scanLevelCount;

  void sortBits(GLuint inputBufId, GLuint outputBufId, int bitOffset, const ci::vec3 &axis,
                float zMin, float zMax);

public:
  RadixSort(uint32_t elemCount, uint32_t blockSize);

  void sort(GLuint inputBufId, GLuint outputBufId, const ci::vec3 &axis, float zMin, float zMax);
};
