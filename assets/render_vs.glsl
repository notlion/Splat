uniform mat4 ciModelView, ciProjectionMatrix;
uniform float time;

in vec4 ciPosition;

const float scale = 0.85;

out float depth;

void main() {
  vec3 pos = ciPosition.xyz;
  vec3 np = pos * 4.0f;
  pos += vec3(sin(np.x + time * 0.423),
  			  sin(np.y + time * 0.437),
  			  sin(np.z + time * 0.453)) * 0.25;

  vec4 viewPos = ciModelView * vec4(pos.xyz, 1.0);
  float viewDist = length(viewPos.xyz);

  gl_PointSize = scale * (20.0 - 6.0 * (viewDist - 1.0) / (3.0 - 1.0));
  gl_Position = ciProjectionMatrix * viewPos;

  depth = gl_Position.z;
}
