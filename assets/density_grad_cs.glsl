#version 430 core

layout(local_size_x = WORK_GROUP_SIZE_XYZ, local_size_y = WORK_GROUP_SIZE_XYZ,
       local_size_z = WORK_GROUP_SIZE_XYZ) in;

layout(r32ui, binding = 0) readonly uniform uimage3D densityImg;
layout(rgba16f, binding = 1) writeonly uniform image3D gradientImg;

uniform uvec3 volumeRes;

const uint kMaxDensityPerParticle = 1024;

ivec3 clampToVol(in ivec3 c) {
  return clamp(c + ivec3(1, 0, 0), ivec3(0), ivec3(volumeRes - 1));
}

void main() {
  ivec3 c = ivec3(gl_GlobalInvocationID);

  float px = float(imageLoad(densityImg, clampToVol(c + ivec3(1, 0, 0))).r);
  float py = float(imageLoad(densityImg, clampToVol(c + ivec3(0, 1, 0))).r);
  float pz = float(imageLoad(densityImg, clampToVol(c + ivec3(0, 0, 1))).r);
  float nx = float(imageLoad(densityImg, clampToVol(c - ivec3(1, 0, 0))).r);
  float ny = float(imageLoad(densityImg, clampToVol(c - ivec3(0, 1, 0))).r);
  float nz = float(imageLoad(densityImg, clampToVol(c - ivec3(0, 0, 1))).r);

  float v = float(imageLoad(densityImg, c).r);

  vec3 grad = vec3(px - nx, py - ny, pz - nz);
  // vec3 grad = vec3((px - v) + (v - nx), (py - v) + (v - ny), (pz - v) + (v - nz));

  imageStore(gradientImg, c, vec4(grad, 0.0));
}
