#version 300 es

precision mediump float;

in vec4 xyst;

out vec2 st;

void main() {
  gl_Position = vec4(xyst.xy, 0.5, 1.0);
  st = xyst.zw;
}
