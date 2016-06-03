#version 430 core

#include "utils/particle.glsl"

layout(local_size_x = WORK_GROUP_SIZE_X) in;

layout(std140, binding = 0) buffer ParticleBuffer {
  Particle particle[];
};
layout(r32ui, binding = 1) uniform uimage3D volumeDensity;

uniform mat4 worldToVolumeMtx;

const uint kMaxDensityPerParticle = 1024;

ivec3 worldToVolumeCoord(in vec3 pos) {
  return ivec3(worldToVolumeMtx * vec4(pos, 1.0));
}

void main() {
  uint id = gl_GlobalInvocationID.x;
  ivec3 coord = worldToVolumeCoord(particle[id].position);
  imageAtomicAdd(volumeDensity, coord, 1);
}
