#version 300 es

precision highp float;

uniform sampler2D sampler;

out vec4 fragmentColor;

in vec2 st;

void main() {
	fragmentColor = vec4(1.0,1.0,1.0,texture(sampler, st).a);
}
