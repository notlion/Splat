uniform sampler2D texture;

in vec3 color;

out vec4 fragColor;

void main() {
  fragColor = vec4(color, 1.0);
  fragColor *= texture2D(texture, gl_PointCoord).r;
}
