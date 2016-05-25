#version 430 core

uniform mat4 ciModelView, ciProjectionMatrix;
uniform float pointSize;

struct Particle {
  vec3 position;
  float scale;
  vec4 color;
};

layout(location = 0) in uint particleId;
layout(std140, binding = 0) buffer Particles {
  Particle particle[];
};

out vec4 color;

void main() {
  vec4 viewPos = ciModelView * vec4(particle[particleId].position, 1.0);
  float viewDist = length(viewPos.xyz);

  gl_PointSize = (pointSize / viewDist) * particle[particleId].scale;
  gl_Position = ciProjectionMatrix * viewPos;

  color = particle[particleId].color;

  // TEMP: Slight fog
  // color.rgb *= 1.0 - gl_Position.z * 0.2;
}
