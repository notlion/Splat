uniform sampler2D texture;

in float depth;

void main() {
  gl_FragColor = vec4(1.0 - depth * 0.2);
  gl_FragColor *= texture2D(texture, gl_PointCoord).r;
}
