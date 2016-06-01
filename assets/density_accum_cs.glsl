#version 430 core

struct Particle {
  vec3 position;
  float scale;
  vec4 color;
};

layout(local_size_x = WORK_GROUP_SIZE_X) in;

layout(std140, binding = 0) buffer ParticleBuffer {
  Particle particle[];
};
layout(r32ui, binding = 1) uniform uimage3D volumeDensity;

uniform vec3 boundsMin, oneOverBoundsSize;
uniform uvec3 volumeRes;
uniform float oneOverCelScale;

const uint kMaxDensityPerParticle = 1024;

void main() {
  uint id = gl_GlobalInvocationID.x;

  if (id >= volumeRes.x * volumeRes.y * volumeRes.z) {
    ivec3 pos = ivec3((particle[id].position - boundsMin) * oneOverBoundsSize * vec3(volumeRes));
    uint density = 1;//uint(particle[id].scale * oneOverCelScale * kMaxDensityPerParticle);
    imageAtomicAdd(volumeDensity, pos, density);
  }
}
