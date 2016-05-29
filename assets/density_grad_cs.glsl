#version 430 core

layout(local_size_x = WORK_GROUP_SIZE_XYZ, local_size_y = WORK_GROUP_SIZE_XYZ,
       local_size_z = WORK_GROUP_SIZE_XYZ) in;

layout(r32ui, binding = 0) readonly uniform uimage3D densityImg;
layout(rgba16f, binding = 1) writeonly uniform image3D gradientImg;

const uint kMaxDensityPerParticle = 1024;

void main() {
  uvec3 id = gl_GlobalInvocationID;
  // vec4 grad = vec4(imageLo, 0.0)
  // imageStore(gradientImg, id, grad);
}
