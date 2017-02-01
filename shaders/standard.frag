#version 310 es

precision highp float;

layout(location = 0) uniform sampler2D sampler;

out vec4 fragmentColor;

in vec2 st;

void main() {
	fragmentColor = vec4(1.0,1.0,1.0,texture(sampler, st).a);
}
