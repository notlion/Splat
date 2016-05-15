uniform mat4 ciModelView, ciProjectionMatrix;
uniform float time, pointSize;

in vec4 ciPosition;

const float scale = 0.85;

out vec3 color;

void main() {
  vec4 viewPos = ciModelView * vec4(ciPosition.xyz, 1.0);
  float viewDist = length(viewPos.xyz);

  gl_PointSize = pointSize / viewDist;
  gl_Position = ciProjectionMatrix * viewPos;

  color = vec3((sin(time + ciPosition.z * 6.0) + 1.0) * 0.5);
  color *= 1.0 - gl_Position.z * 0.2;
}
