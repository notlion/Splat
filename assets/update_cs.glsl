#version 430 core

#include "utils/noise.glsl"
#include "utils/particle.glsl"


layout(local_size_x = WORK_GROUP_SIZE_X) in;

layout(std140, binding = 0) buffer ParticleBuffer {
  Particle particle[];
};
layout(std140, binding = 1) buffer ParticlePrevBuffer {
  Particle particlePrev[];
};

uniform float time;
uniform uint frameId;
uniform vec3 eyePos, eyeVel, viewDir;

uniform bool init, compile;

uniform vec3 volumeBoundsMin, volumeBoundsMax;
uniform uvec3 volumeRes;
uniform mat4 worldToUnitVolumeMtx;

uniform usampler3D densityTex;
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
  vel *= 0.7;
  particlePrev[id] = particle[id];

  vec3 texcoord = worldToVolumeTexcoord(pos);
  vec3 dg = texture(densityGradTex, texcoord).xyz;
  float d = float(texture(densityTex, texcoord).r);

  float h11t = hash11(t);

  const float cycleDuration = 2000.0;
  int age = int(mod(t * cycleDuration - float(frameId), cycleDuration));
  // age = int(frameId) % 100;

  bool flotsam = t < 0.025;
  bool spawn =
      init || age == 0 ||
      (!flotsam && (any(greaterThan(texcoord, vec3(1.0))) || any(lessThan(texcoord, vec3(0.0)))));

  if (spawn) {
    vec3 p;
    if (flotsam) {
      p = randVec3(t) * sqrt(h11t) * 4.0;
    } else {
      p = randVec3(t) * mix(0.5, 0.8, h11t);
    }
    particle[id].position = p;
    particlePrev[id].position = p; // - vec3(0.1, 0.0, 0.0);
  } else {
    vec3 scale = vec3(2.0, 2.01, 2.05);
    vec3 q = scale * pos + kHashScale3 + vec3(time) * vec3(0.05, 0.07, 0.09);
    vec3 dN = SimplexPerlin3D_Deriv(q).xyz + 0.7 * SimplexPerlin3D_Deriv(q * 5.01).xyz;
    dN += -normalize(pos) * t * 0.0005;
    // vec3 v1 = dN;
    vec3 v1 = dN + vec3(dN.y - dN.z, dN.z - dN.x, dN.x - dN.y);

    vel += v1 * 0.00001;

    vec3 eyeDir = pos - eyePos;
    float eyePow = smoothstep(0.4, 0.0, length(eyeDir));
    eyeDir = normalize(eyeDir);
    vel += max(0.0, dot(eyeDir, eyeVel)) * eyeDir * eyePow;

    // vel -= dg * 0.00001;
    // vel += normalize(pos).yxx * vec3(1.0, -1.0, 0.0) * 0.001;

    // vel += -normalize(pos) * t * 0.0005;

    particle[id].position = pos + vel; // mix(0.5, 0.8, t);
  }

  // float wave = (sin(time + particle[id].position.z * kTwoPi * 0.5) + 1.0) * 0.5;
  // particle[id].color.rgb = pal(t, vec3(0.5, 0.5, 0.5), vec3(0.5, 0.5, 0.5), vec3(1.0, 1.0,
  // 0.5), vec3(0.8, 0.90, 0.30));
  vec4 c = vec4(1.0);
  // float viewDist = distance(eyePos, particle[id].position);
  // float d = ramp(5.0, 2.0, viewDist);
  // c.rgb = pal(d, vec3(1.118,0.680,0.750), vec3(1.008,0.600,0.600), vec3(0.318,0.330,0.310),
  // vec3(-0.440,-0.447,-0.500));

  c.rgb = dg * 0.005;
  // float a = time * 0.1;
  // c.rgb = vec3(clamp(dot(normalize(dg), vec3(sin(a), cos(a), 0.0)), 0.0, 1.0));
  c.a = 1.0;

  c *= 0.25f;
  c.g *= 0.25;

  float s;
  if (flotsam) {
    c = vec4(0.1);
    s = mix(1.0, 2.0, h11t);
  } else {
    s = mix(1.0, 4.0, h11t);
  }

  particle[id].color = spawn ? vec4(0.0) : c;
  particle[id].scale = s; // fract(time / 5.0) > 0.5 ? 2.0 : 0.0;
}
