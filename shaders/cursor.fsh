precision highp float;

uniform sampler2D sampler;

varying vec2 out_st;
void main() { gl_FragColor = texture2D(sampler, out_st); }
