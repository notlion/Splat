in float depth;

void main() {
  gl_FragColor = vec4(1.0 - depth * 0.2);
}