#include "utils/noise.glsl"

uniform float time;
uniform uint frameId;

const float kPi = 3.1415927;
const float kTwoPi = 6.2831853;

const float kHashScale1 = 443.8975;
const vec3 kHashScale3 = vec3(443.897, 441.423, 437.195);

float hash11(float p) {
  vec3 p3 = fract(vec3(p) * kHashScale1);
  p3 += dot(p3, p3.yzx + 19.19);
  return fract((p3.x + p3.y) * p3.z);
}

vec2 hash21(float p) {
  vec3 p3 = fract(vec3(p) * kHashScale3);
  p3 += dot(p3, p3.yzx + 19.19);
  return fract(vec2((p3.x + p3.y) * p3.z, (p3.x + p3.z) * p3.y));
}

vec3 hash31(float p) {
  vec3 p3 = fract(vec3(p) * kHashScale3);
  p3 += dot(p3, p3.yzx + 19.19);
  return fract(vec3((p3.x + p3.y) * p3.z, (p3.x + p3.z) * p3.y, (p3.y + p3.z) * p3.x));
}

vec3 pal(in float t, in vec3 a, in vec3 b, in vec3 c, in vec3 d) {
  return a + b * cos(kTwoPi * (c * t + d));
}

vec3 randVec3(float p) {
  vec2 rnd = hash21(p);
  float phi = rnd.x * kTwoPi;
  float costheta = mix(-1.0, 1.0, rnd.y);
  float rho = sqrt(1.0 - costheta * costheta);
  return vec3(rho * cos(phi), rho * sin(phi), costheta);
}

void main() {
  uint id = gl_GlobalInvocationID.x;
  float t = float(id) / float(PARTICLE_COUNT);

  vec3 pos = particle[id].position;
  vec3 vel = (pos - particlePrev[id].position);
  particlePrev[id] = particle[id];

  if (frameId % 2000 == 0) {
    pos = randVec3(t) - 0.5;
  }

  vec3 scale = vec3(2.0, 2.01, 2.05);
  vec3 q = scale * pos + kHashScale3 + vec3(time) * vec3(0.05, 0.07, 0.09);
  vec3 dN = SimplexPerlin3D_Deriv(q).xyz + 0.7 * SimplexPerlin3D_Deriv(q * 5.01).xyz;
  vec3 v1 = vec3(dN.y - dN.z, dN.z - dN.x, dN.x - dN.y);

  vel = v1 * 0.0002;
  particle[id].position = pos + vel;

  // float wave = (sin(time + particle[id].position.z * kTwoPi * 0.5) + 1.0) * 0.5;
  // particle[id].color.rgb = pal(t, vec3(0.5, 0.5, 0.5), vec3(0.5, 0.5, 0.5), vec3(1.0, 1.0, 0.5), vec3(0.8, 0.90, 0.30));
  particle[id].color = vec4(0.05);
  particle[id].scale = 2.0;
}
