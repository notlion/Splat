#version 430 core

#include "utils/noise.glsl"

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

layout(r32ui, binding = 2) readonly uniform uimage3D volumeDensity;
layout(rgba16f, binding = 3) readonly uniform image3D volumeDensityGrad;


uniform float time;
uniform uint frameId;
uniform vec3 eyePos, viewDir;

uniform vec3 volumeBoundsMin, volumeBoundsMax;
uniform uvec3 volumeRes;
uniform mat4 worldToUnitVolumeMtx;

uniform sampler3D densityGradTex;


vec3 worldToVolumeTexcoord(in vec3 pos) {
  return vec3(worldToUnitVolumeMtx * vec4(pos, 1.0));
}


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

float ramp(float e1, float e2, float x) {
  return x = clamp((x - e1) / (e2 - e1), 0.0, 1.0);
}


void main() {
  uint id = gl_GlobalInvocationID.x;

  float t = float(id) / float(PARTICLE_COUNT);

  vec3 pos = particle[id].position;
  vec3 vel = pos - particlePrev[id].position;
  particlePrev[id] = particle[id];

  vec3 dg = texture(densityGradTex, worldToVolumeTexcoord(pos)).xyz;

  const float cycleDuration = 500.0;
  int age = int(mod(t * cycleDuration - float(frameId), cycleDuration));
  age = int(frameId) % 100;

  if (age == 0) {
    vec3 p = randVec3(t) * 0.75;
    //p *= mix(0.9, 1.0, hash11(t));
    //p.x += sin(time) * 0.1;
    particle[id].position = particlePrev[id].position = p;
  } else {
    vec3 scale = vec3(2.0, 2.01, 2.05);
    vec3 q = scale * pos + kHashScale3 + vec3(time) * vec3(0.05, 0.07, 0.09);
    vec3 dN = SimplexPerlin3D_Deriv(q).xyz + 0.7 * SimplexPerlin3D_Deriv(q * 5.01).xyz;
    dN += -normalize(pos) * t * 0.0005;
    vec3 v1 = dN;
    // vec3 v1 = dN + vec3(dN.y - dN.z, dN.z - dN.x, dN.x - dN.y);

    //vel += v1 * 0.0001;

    vel -= dg * 0.0001;
    //vel += normalize(pos).yxx * vec3(1.0, -1.0, 0.0) * 0.001;

    // vel += -normalize(pos) * t * 0.0005;

    particle[id].position = pos + vel * 0.5;//mix(0.5, 0.8, t);
  }

  // float wave = (sin(time + particle[id].position.z * kTwoPi * 0.5) + 1.0) * 0.5;
  // particle[id].color.rgb = pal(t, vec3(0.5, 0.5, 0.5), vec3(0.5, 0.5, 0.5), vec3(1.0, 1.0,
  // 0.5), vec3(0.8, 0.90, 0.30));
  vec4 c = vec4(1.0);
  // float viewDist = distance(eyePos, particle[id].position);
  // float d = ramp(5.0, 2.0, viewDist);
  // c.rgb = pal(d, vec3(1.118,0.680,0.750), vec3(1.008,0.600,0.600), vec3(0.318,0.330,0.310),
  // vec3(-0.440,-0.447,-0.500));

  c.rgb = dg * 0.05;
  // float a = time * 0.1;
  // c.rgb = vec3(clamp(dot(normalize(dg), vec3(sin(a), cos(a), 0.0)), 0.0, 1.0));
  c.a = 1.0;

  c *= 0.25f;

  particle[id].color = c;
  particle[id].scale = 2.0;//fract(time / 5.0) > 0.5 ? 2.0 : 0.0;
}
