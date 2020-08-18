#ifndef MYY_TEXT_H
#define MYY_TEXT_H 1

#include <stdint.h>
#include <helpers/myy_vector/vector.h>
#include <helpers/strings.h>
#include <current/opengl.h>

/* TODO
 * Next steps :
 * - Provide the tools and headers to parse the data
 * - Prepare the rights GLSL
 * - On each draw call
 * 	- Initialize the right matrices (Setup the texture ortho)
 * 	- Initialize the uniforms
 * 	- Send the draw data
 */
 

extern GLuint glsl_programs[];
enum gl_char_points {
	gl_char_tri1_upper_left,
	gl_char_tri1_bottom_left,
	gl_char_tri1_bottom_right,
	gl_char_tri2_upper_right,
	gl_char_tri2_upper_left,
	gl_char_tri2_bottom_right
};

myy_vector_template(u8, uint8_t)

struct myy_gl_double_buffers {
	GLuint id[2];
	uint8_t current_id;
};

static inline void myy_double_buffers_init(
	struct myy_gl_double_buffers * __restrict const buffers)
{
	glGenBuffers(2, buffers->id);
	buffers->current_id = 0;
}

static inline void myy_double_buffers_bind(
	struct myy_gl_double_buffers * __restrict const buffers)
{
	glBindBuffer(GL_ARRAY_BUFFER, buffers->id[buffers->current_id]);
}

static inline void myy_double_buffers_bind_next(
	struct myy_gl_double_buffers * __restrict const buffers)
{
	buffers->current_id ^= 1;
	myy_double_buffers_bind(buffers);
}

static inline void myy_double_buffers_store(
	struct myy_gl_double_buffers * __restrict const buffers,
	void const * __restrict const data,
	size_t data_size)
{
	myy_double_buffers_bind_next(buffers);
	glBufferData(GL_ARRAY_BUFFER, data_size, data, GL_DYNAMIC_DRAW);
}

static inline void myy_double_buffers_free(
	struct myy_gl_double_buffers * __restrict const buffers)
{
	glDeleteBuffers(2, buffers->id);
}

struct gl_atlas_glyph_data {
	uint16_t tex_left, tex_right, tex_bottom, tex_top;
	int16_t offset_x_px, offset_y_px;
	int16_t advance_x_px, advance_y_px;
	uint16_t width_px, height_px;
};
typedef struct gl_atlas_glyph_data myy_glyph_data_t;

struct myy_gl_char_point { int16_t x, y, s, t; };
struct myy_gl_char {
	/* 2 triangles with 3 points each */
	struct myy_gl_char_point points[6];
};
typedef struct myy_gl_char myy_gl_char_t;
myy_vector_template(mgl_char, myy_gl_char_t)

struct gl_text_area {
	struct myy_gl_double_buffers gl_buffers;
	struct {
		int16_t x, y;
	} pos;
	uint32_t n_points;
	myy_vector_u8 string;
	myy_vector_mgl_char cpu_buffer;
};

static inline void glsl_text_use_program() {
	glUseProgram(glsl_programs[glsl_text_program]);
}

static inline bool gl_text_area_init(
	struct gl_text_area * __restrict const area)
{
	myy_double_buffers_init(&area->gl_buffers);
	area->n_points = 0;
	area->pos.x = 0;
	area->pos.y = 0;
	area->string = myy_vector_u8_init(64);
	area->cpu_buffer = myy_vector_mgl_char_init(32);
	return
		myy_vector_u8_is_valid(&area->string) &&
		myy_vector_mgl_char_is_valid(&area->cpu_buffer);
}

static void glfont_glyph_to_gl_char(
	myy_glyph_data_t const * __restrict const glyph,
	myy_gl_char_t * __restrict const glc,
	int16_t write_pos_x, int16_t write_pos_y)
{
	float const tex_left   = glyph->tex_left;
	float const tex_right  = glyph->tex_right;
	float const tex_bottom = glyph->tex_bottom;
	float const tex_top    = glyph->tex_top;

	int16_t const left     = write_pos_x + glyph->offset_x_px;
	int16_t const right    = write_pos_x + glyph->offset_x_px + glyph->width_px;
	int16_t const top      = write_pos_y + (-glyph->height_px + glyph->offset_y_px);
	int16_t const bottom   = write_pos_y + glyph->offset_y_px;

	glc->points[gl_char_tri1_upper_left].x = left;
	glc->points[gl_char_tri1_upper_left].y = top;
	glc->points[gl_char_tri1_upper_left].s = tex_left;
	glc->points[gl_char_tri1_upper_left].t = tex_top;

	glc->points[gl_char_tri1_bottom_left].x = left;
	glc->points[gl_char_tri1_bottom_left].y = bottom;
	glc->points[gl_char_tri1_bottom_left].s = tex_left;
	glc->points[gl_char_tri1_bottom_left].t = tex_bottom;

	glc->points[gl_char_tri1_bottom_right].x = right;
	glc->points[gl_char_tri1_bottom_right].y = bottom;
	glc->points[gl_char_tri1_bottom_right].s = tex_right;
	glc->points[gl_char_tri1_bottom_right].t = tex_bottom;

	glc->points[gl_char_tri2_upper_right].x = right;
	glc->points[gl_char_tri2_upper_right].y = top;
	glc->points[gl_char_tri2_upper_right].s = tex_right;
	glc->points[gl_char_tri2_upper_right].t = tex_top;

	glc->points[gl_char_tri2_upper_left].x = left;
	glc->points[gl_char_tri2_upper_left].y = top;
	glc->points[gl_char_tri2_upper_left].s = tex_left;
	glc->points[gl_char_tri2_upper_left].t = tex_top;

	glc->points[gl_char_tri2_bottom_right].x = right;
	glc->points[gl_char_tri2_bottom_right].y = bottom;
	glc->points[gl_char_tri2_bottom_right].s = tex_right;
	glc->points[gl_char_tri2_bottom_right].t = tex_bottom;

}


static myy_glyph_data_t const * myy_atlas_get_glyph_data(
	void const * __restrict const atlas,
	uint32_t const codepoint)
{
	static myy_glyph_data_t glyph = { 0 };
	return &glyph;
}

static inline bool gl_text_area_save_for_glsl_text(
	struct gl_text_area * __restrict const area)
{
	/* We allocate everything at once.
	 * That way, if we don't have enough memory, we'll know
	 * it soon enough.
	 */
	size_t const string_length = myy_vector_u8_length(&area->string);
	bool const enough_memory = myy_vector_mgl_char_ensure_enough_space_for(
		&area->cpu_buffer,
		string_length);

	if (enough_memory) {
		int16_t const start_x = area->pos.x;
		int16_t write_x = start_x;
		int16_t write_y = area->pos.y;

		/* Consider all the characters written */
		myy_vector_mgl_char_force_length_to(&area->cpu_buffer, string_length);

		myy_gl_char_t * __restrict current_gl_char =
			myy_vector_mgl_char_data(&area->cpu_buffer);
		uint8_t const * __restrict string =
			myy_vector_u8_data(&area->string);
		uint8_t const * __restrict const string_end =
			myy_vector_u8_tail_ptr(&area->string);

		while(string < string_end) {
			struct utf8_codepoint codepoint =
				utf8_codepoint_and_size(string);
			string += codepoint.size;
			
			myy_glyph_data_t const * __restrict const glyph_data =
				myy_atlas_get_glyph_data(NULL, codepoint.value);
			glfont_glyph_to_gl_char(
				glyph_data, current_gl_char++, write_x, write_y);

			if (codepoint.value != 0xa) {
				write_x += glyph_data->advance_x_px;
			}
			else {
				write_x = start_x;
				write_y += 16 /* pixels. FIXME : Make this customisable */;
			}
		}
	}

	return enough_memory;
}

static inline void gl_text_area_set_text(
	struct gl_text_area * __restrict const area,
	char const * __restrict const utf8_text)
{
	myy_vector_u8 * __restrict const string = &area->string;
	myy_vector_u8_reset(string);
	myy_vector_u8_add(string, strlen(utf8_text), (uint8_t *) utf8_text);
	gl_text_area_save_for_glsl_text(area);
}

void gl_text_area_string_at(
	struct gl_text_area * __restrict const area,
	void * __restrict const glfont,
	uint8_t const * __restrict string,
	int16_t x, int16_t y);

void glsl_text_draw(struct gl_text_area * __restrict const area);


#endif
