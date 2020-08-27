precision highp float;

attribute vec4 relative_xy;
attribute vec2 in_st;

uniform mat4 projection;
uniform vec2 texture_projection;
/* Used for offseting text when displaying drop-down shadows.
   When displaying "shadows", we just draw behind the text,
   with a 'black' tone, similar to text-shadow in CSS. */
uniform vec4 relative_text_offset;
/* Used to move all the text elements */
uniform vec4 absolute_offset;

varying vec2 st;

void main() {
	gl_Position = projection * (relative_xy+absolute_offset+relative_text_offset);
	st = in_st * texture_projection;
}

