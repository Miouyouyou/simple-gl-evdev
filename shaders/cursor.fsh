precision highp float;

uniform sampler2D sampler;

varying vec2 out_st;
/* FIXME Texture generator b0rked out and is converting
   to ARGB, instead of RGBA */
void main() { gl_FragColor = texture2D(sampler, out_st).argb; }
