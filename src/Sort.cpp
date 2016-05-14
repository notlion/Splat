// NOTE(ryan): This is mostly cribbed from ARM's ComputeParticles sample
// http://malideveloper.arm.com/resources/sample-code/particle-flow-simulation-compute-shaders/

#include "Sort.hpp"

#include "cinder/app/App.h"
#include "cinder/gl/gl.h"

using namespace ci;

RadixSort::RadixSort(uint32_t elemCount, uint32_t blockSize)
: elemCount(elemCount), blockSize(blockSize) {
  auto fmt = gl::GlslProg::Format();

  scanProg = gl::GlslProg::create(fmt.compute(app::loadAsset("scan_cs.glsl")));
  scanFirstProg = gl::GlslProg::create(fmt.compute(app::loadAsset("scan_first_cs.glsl")));
  resolveProg = gl::GlslProg::create(fmt.compute(app::loadAsset("scan_resolve_cs.glsl")));
  reorderProg = gl::GlslProg::create(fmt.compute(app::loadAsset("scan_reorder_cs.glsl")));

  {
    scanLevelCount = 0;

    auto size = elemCount;
    while (size > 1) {
      scanLevelCount++;
      size = (size + blockSize - 1) / blockSize;
    }
  }

  flagsBuffer = gl::BufferObj::create(GL_SHADER_STORAGE_BUFFER, elemCount * sizeof(GLuint), nullptr,
                                      GL_DYNAMIC_COPY);

  {
    scanBuffers.reserve(scanLevelCount);
    sumBuffers.reserve(scanLevelCount);

    uint32_t blockCount = elemCount / blockSize;
    for (uint32_t i = 0; i < scanLevelCount; ++i) {
      scanBuffers.push_back(gl::BufferObj::create(GL_SHADER_STORAGE_BUFFER,
                                                  blockCount * blockSize * 4 * sizeof(GLuint),
                                                  nullptr, GL_DYNAMIC_COPY));
      blockCount = (blockCount + blockSize - 1) / blockSize;
      sumBuffers.push_back(gl::BufferObj::create(GL_SHADER_STORAGE_BUFFER,
                                                 blockCount * blockSize * 4 * sizeof(GLuint),
                                                 nullptr, GL_DYNAMIC_COPY));
    }
  }
}

void RadixSort::sortBits(GLuint inputBufId, GLuint outputBufId, int bitOffset, const vec3 &axis,
                         float zMin, float zMax) {
  // Keep track of which dispatch sizes we used to make the resolve steps simpler.
  std::vector<uint32_t> dispatchSizes(scanLevelCount);

  uint32_t blockCount = elemCount / blockSize;

  {
    // First pass. Compute 16-bit unsigned depth and apply first pass of scan algorithm.

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, inputBufId);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, scanBuffers.front()->getId());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, sumBuffers.front()->getId());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, flagsBuffer->getId());

    scanFirstProg->bind();
    scanFirstProg->uniform("bitOffset", bitOffset);
    scanFirstProg->uniform("axis", axis);
    scanFirstProg->uniform("zMin", zMin);
    scanFirstProg->uniform("zMax", zMax);

    dispatchSizes[0] = blockCount;
    glDispatchCompute(blockCount, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
  }

  {
    // If we processed more than one work group of data, we're not done, so scan buf_sums[0] and
    // keep scanning recursively like this until buf_sums[N] becomes a single value.

    scanProg->bind();

    for (uint32_t i = 1; i < scanLevelCount; ++i) {
      blockCount = (blockCount + blockSize - 1) / blockSize;
      dispatchSizes[i] = blockCount;

      glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, sumBuffers[i - 1]->getId());
      // If we only do one work group we don't need to resolve it later,
      // and we can update the scan buffer inplace.
      if (blockCount <= 1) {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, sumBuffers[i - 1]->getId());
      } else {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, scanBuffers[i]->getId());
      }
      glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, sumBuffers[i]->getId());

      glDispatchCompute(blockCount, 1, 1);
      glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }
  }

  {
    // Go backwards, we want to end up with a buf_sums[0] which has been properly scanned.
    // Once we have buf_scan[0] and buf_sums[0], we can do the reordering step.

    resolveProg->bind();

    for (uint32_t i = scanLevelCount - 1; i > 0; --i) {
      if (dispatchSizes[i] <= 1) { // No need to resolve, buf_sums[i - 1] is already correct
        continue;
      }

      glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, scanBuffers[i]->getId());
      glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, sumBuffers[i]->getId());
      glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, sumBuffers[i - 1]->getId());

      glDispatchCompute(dispatchSizes[i], 1, 1);
      glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }
  }

  {
    // We can now reorder our input properly.

    reorderProg->bind();

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, inputBufId);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, scanBuffers[0]->getId());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, sumBuffers[0]->getId());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, outputBufId);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, flagsBuffer->getId());

    glDispatchCompute(elemCount / blockSize, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
  }
}

void RadixSort::sort(GLuint inputBufId, GLuint outputBufId, const vec3 &axis, float zMin,
                     float zMax) {
  for (uint32_t i = 0; i < 8; ++i) {
    sortBits(inputBufId, outputBufId, i * 2, axis, zMin, zMax);

    // Swap for the next digit stage
    // The <buf_input> buffer will in the end hold the latest sorted data
    std::swap(inputBufId, outputBufId);
  }

  // We use the position data to draw the particles afterwards
  // Thus we need to ensure that the data is up to date
  glMemoryBarrier(GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);
}
