#version 430 core

#include "utils/particle.glsl"

uniform mat4 ciModelViewProjection;
uniform float pointSize;

layout(location = 0) in uint particleId;
layout(std140, binding = 0) buffer Particles {
  Particle particle[];
};

out vec4 color;

void main() {
  color = particle[particleId].color;
  gl_Position = ciModelViewProjection * vec4(particle[particleId].position, 1.0);
  gl_PointSize = (pointSize * particle[particleId].scale) / gl_Position.w;
}
