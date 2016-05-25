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
layout(std140, binding = 1) buffer ParticlePrevBuffer {
  Particle particlePrev[];
};

// void main() {}
