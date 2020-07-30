precision highp float;

attribute vec4 xy;
attribute vec2 in_st;

uniform mat4 projection;
uniform vec2 texture_projection;
/* Used for offseting text when displaying drop-down shadows */
uniform vec4 text_offset;
/* Used to move all the text elements */
uniform vec4 global_offset;

varying vec2 st;

void main() {
	gl_Position = projection * (xy+text_offset+global_offset);
	st = in_st * texture_projection;
}

