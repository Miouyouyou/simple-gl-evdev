#version 310 es

precision mediump float;

layout(location = 0) in vec3 xyz;
layout(location = 1) in vec2 in_st;

out vec2 st;

layout(location = 1) uniform vec2 offset_uniform;

const vec2 pixel = vec2((2.0/1280.0), (2.0/720.0));
const vec2 half_pixel = vec2((1.0/1280.0), (1.0/720.0));

void main() {
	/*vec2 random_position = xyz.xy;
	vec2 reordered_position = random_position - mod(random_position, pixel);*/
	gl_Position = vec4(xyz.xy + offset_uniform, xyz.z, 1.0);
	st = in_st;
}
