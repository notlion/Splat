#version 430 core

layout(local_size_x = WORK_GROUP_SIZE_X) in;
layout(std140, binding = 0) buffer PositionBuffer {
  vec4 position[];
};

uniform float time;

const float kTwoPi = 6.2831853;
const vec3 kHashScale3 = vec3(443.897, 441.423, 437.195);

vec3 hash31(float p) {
  vec3 p3 = fract(vec3(p) * kHashScale3);
  p3 += dot(p3, p3.yzx + 19.19);
  return fract(vec3((p3.x + p3.y) * p3.z, (p3.x + p3.z) * p3.y, (p3.y + p3.z) * p3.x));
}

void main() {
  uint index = gl_GlobalInvocationID.x;
  float t = float(index) / PARTICLE_COUNT;

  position[index].xyz = hash31(t);
}
