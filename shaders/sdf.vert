precision highp float;

attribute vec4 pos;
attribute vec2 st;

uniform mat4 proj;
uniform float z;

varying vec2 v_texcoord;

void main() {
    gl_Position = proj * vec4(pos.xy * 10.0 + vec2(200.0,200.0), 8.0, 1.0);
    v_texcoord = st;
}