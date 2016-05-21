uniform sampler2D texture;

in vec4 color;

out vec4 fragColor;

void main() {
  fragColor = color * texture2D(texture, gl_PointCoord).r;
}
