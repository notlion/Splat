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

  particle[index].position = hash31(t) * 2.0 - 1.0;

  float wave = (sin(time + particle[index].position.z * kTwoPi * 2.0) + 1.0) * 0.5;
  particle[index].color.rgb = vec3(wave);
  particle[index].scale = wave * 1.5 + 0.5;
}
