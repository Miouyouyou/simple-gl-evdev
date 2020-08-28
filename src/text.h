#ifndef MYY_TEXT_H
#define MYY_TEXT_H 1

#include <stdint.h>
#include <stdarg.h>


#include <myy.h>
#include <helpers/file/file.h>
#include <helpers/myy_vector/vector.h>
#include <helpers/strings.h>
#include <current/opengl.h>
#include <helpers/matrices.h>
#include <helpers/packed_fonts_parser.h>

/* TODO
 * Next steps :
 * - v Provide the tools and headers to parse the data
 * - v Prepare the rights GLSL
 * - On each draw call
 * 	- (Done once) Initialize the right matrices (Setup the texture ortho)
 * 	- v Initialize the uniforms
 * 	- v Send the draw data
 * 
 * - Check that all uniforms are setup correctly
 * - Call the appropriate setup functions
 * - Test the whole thing
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

typedef struct myy_packed_fonts_glyphdata myy_glyph_data_t;

struct myy_gl_char_point {
	int16_t x, y;       /* Position, in pixels */
	uint16_t s, t;      /* Atlas texture coordinates, in pixels */
};
typedef struct myy_gl_char_point myy_gl_char_point_t;
struct myy_gl_char {
	/* 2 triangles with 3 points each */
	struct myy_gl_char_point points[6];
};
typedef struct myy_gl_char myy_gl_char_t;
myy_vector_template(mgl_char, myy_gl_char_t)

struct myy_gl_double_buffers {
	GLuint id[2];
	uint8_t current_id;
};

struct gl_text_area {
	struct myy_gl_double_buffers gl_buffers;
	struct {
		int16_t x, y;
	} pos;
	struct {
		uint8_t r, g, b, a;
	} color;
	uint32_t n_points;
	myy_vector_u8 string;
	myy_vector_mgl_char cpu_buffer;
};
typedef struct gl_text_area gl_text_area_t;

struct {
	GLuint projection;
	GLuint texture_projection;
	GLuint relative_text_offset;
	GLuint absolute_offset;
	GLuint fonts_texture;
	GLuint rgb;
} gl_simple_text_unifs;

enum {
	gl_simple_text_attr_relative_xy,
	gl_simple_text_attr_in_st
};

__attribute__((unused))
static inline void myy_double_buffers_init(
	struct myy_gl_double_buffers * __restrict const buffers)
{
	glGenBuffers(2, buffers->id);
	buffers->current_id = 0;
}

__attribute__((unused))
static inline void myy_double_buffers_bind(
	struct myy_gl_double_buffers const * __restrict const buffers)
{
	glBindBuffer(GL_ARRAY_BUFFER, buffers->id[buffers->current_id]);
}

__attribute__((unused))
static inline void myy_double_buffers_bind_next(
	struct myy_gl_double_buffers * __restrict const buffers)
{
	buffers->current_id ^= 1;
	myy_double_buffers_bind(buffers);
}

__attribute__((unused))
static inline void myy_double_buffers_store(
	struct myy_gl_double_buffers * __restrict const buffers,
	void const * __restrict const data,
	size_t data_size)
{
	myy_double_buffers_bind_next(buffers);
	glBufferData(GL_ARRAY_BUFFER, data_size, data, GL_DYNAMIC_DRAW);
}

__attribute__((unused))
static inline void myy_double_buffers_free(
	struct myy_gl_double_buffers * __restrict const buffers)
{
	glDeleteBuffers(2, buffers->id);
}




__attribute__((unused))
static inline void glsl_text_use_program() {
	glUseProgram(glsl_programs[glsl_text_program]);
}

__attribute__((unused))
static inline GLuint gl_text_area_simple_use_program() {
	GLuint const program_id = glsl_programs[glsl_simple_text_program];
	glUseProgram(program_id);
	return program_id;
}

__attribute__((unused))
static inline bool gl_text_area_init(
	struct gl_text_area * __restrict const area)
{
	myy_double_buffers_init(&area->gl_buffers);
	area->n_points = 0;
	area->pos.x = 0;
	area->pos.y = 0;
	area->color.r = 0;
	area->color.g = 0;
	area->color.b = 0;
	area->color.a = 255;
	area->string = myy_vector_u8_init(64);
	area->cpu_buffer = myy_vector_mgl_char_init(32);
	return
		myy_vector_u8_is_valid(&area->string) &&
		myy_vector_mgl_char_is_valid(&area->cpu_buffer);
}

static inline void gl_text_area_bind_buffers(
	struct gl_text_area const * __restrict const area)
{
	myy_double_buffers_bind(&area->gl_buffers);
}

__attribute__((unused))
static inline void gl_text_area_set_color(
	struct gl_text_area * __restrict const area,
	uint8_t const r, uint8_t const g, uint8_t const b, uint8_t const a)
{
	area->color.r = r;
	area->color.g = g;
	area->color.b = b;
	area->color.a = a;
}

__attribute__((unused))
static inline void gl_text_area_set_global_position(
	struct gl_text_area * __restrict const area,
	int16_t const x, int16_t const y)
{
	area->pos.x = x;
	area->pos.y = y;
}

#define store_point(storage, ...) \
	{\
		myy_gl_char_point_t const point = __VA_ARGS__; \
		storage = point; \
	}

__attribute__((unused))
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

	store_point(
		glc->points[gl_char_tri1_upper_left],
		{ left, top, tex_left, tex_top })
	store_point(
		glc->points[gl_char_tri1_bottom_left],
		{ left, bottom, tex_left, tex_bottom })
	store_point(
		glc->points[gl_char_tri1_bottom_right],
		{ right, bottom, tex_right, tex_bottom })
	store_point(
		glc->points[gl_char_tri2_upper_right],
		{ right, top, tex_right, tex_top })
	store_point(
		glc->points[gl_char_tri2_upper_left],
		{ left, top, tex_left, tex_top })
	store_point(
		glc->points[gl_char_tri2_bottom_right],
		{ right, bottom, tex_right, tex_bottom })
	/*glc->points[gl_char_tri1_upper_left].x = left;
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
	glc->points[gl_char_tri2_bottom_right].t = tex_bottom;*/

}

__attribute__((unused))
static myy_glyph_data_t const * myy_atlas_get_glyph_data(
	gl_simple_text_atlas_t const * __restrict const atlas,
	uint32_t const codepoint)
{
	uint32_t const n_codepoints = atlas->stored_codepoints;
	uint32_t const * __restrict const codepoints = atlas->codepoints_addr;
	myy_glyph_data_t const * __restrict const glyphs_data =
		atlas->glyphdata_addr;

	uint32_t i = 0;
	while (i < n_codepoints && codepoints[i] != codepoint) i++;

	/* glyphs_data+0 contain the placeholder glyph data used
	 * for missing glyphs.
	 * If i == n_codepoints, it means we never found the codepoint.
	 */
	if (i == n_codepoints) i = 0;

	return glyphs_data+i;
}

__attribute__((unused))
static inline bool gl_text_area_simple_save_to_gpu(
	struct gl_text_area * __restrict const area,
	gl_simple_text_atlas_t const * __restrict const atlas)
{
	/* We allocate everything at once.
	 * That way, if we don't have enough memory, we'll know
	 * it soon enough.
	 */
	size_t const string_length = myy_vector_u8_length(&area->string);
	myy_vector_mgl_char * __restrict const geometry_cpu_buffer = &area->cpu_buffer;
	bool const enough_memory = myy_vector_mgl_char_ensure_enough_space_for(
		geometry_cpu_buffer,
		string_length);

	/* TODO Treat each pack independently */
	if (enough_memory) {
		int16_t const start_x = area->pos.x;
		int16_t write_x = start_x;
		int16_t write_y = area->pos.y;
		size_t const expected_points =
			string_length * 6 /* points per character quad (2 triangles) */;

		/* Consider all the characters written */
		myy_vector_mgl_char_force_length_to(
			geometry_cpu_buffer, string_length);

		myy_gl_char_t * __restrict current_gl_char =
			myy_vector_mgl_char_data(geometry_cpu_buffer);
		uint8_t const * __restrict string =
			myy_vector_u8_data(&area->string);
		uint8_t const * __restrict const string_end =
			myy_vector_u8_tail_ptr(&area->string);

		while(string < string_end) {
			struct utf8_codepoint codepoint =
				utf8_codepoint_and_size(string);
			string += codepoint.size;
			
			myy_glyph_data_t const * __restrict const glyph_data =
				myy_atlas_get_glyph_data(atlas, codepoint.value);
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

		LOG("Sending %zu chars to the GPU.\n", expected_points);
		myy_double_buffers_bind_next(&area->gl_buffers);
		myy_double_buffers_store(&area->gl_buffers,
			myy_vector_mgl_char_data(geometry_cpu_buffer),
			myy_vector_mgl_char_allocated_used(geometry_cpu_buffer));
		area->n_points = expected_points;

	}
	else {
		LOG("Not enough memory for %zu chars !?\n", string_length);
	}

	return enough_memory;
}

__attribute__((unused))
static inline void gl_text_area_remove_text(
	struct gl_text_area * __restrict const area,
	char const * __restrict const utf8_text)
{
	myy_vector_u8 * __restrict const string = &area->string;
	myy_vector_u8_reset(string);
}

__attribute__((unused))
static inline void gl_text_area_append_text(
	struct gl_text_area * __restrict const area,
	char const * __restrict const utf8_text)
{
	myy_vector_u8 * __restrict const string = &area->string;
	myy_vector_u8_add(string, strlen(utf8_text), (uint8_t *) utf8_text);
}

__attribute__((unused))
__attribute__((format (printf, 2, 3)))
static inline void gl_text_area_append_text_format(
	struct gl_text_area * __restrict const area,
	char const * __restrict const utf8_format,
	...)
	
{
	va_list args;
	va_start(args, utf8_format);

	int n_chars = vsnprintf(NULL, 0, utf8_format, args);

	myy_vector_u8 * __restrict const string = &area->string;
	myy_vector_u8_ensure_enough_space_for(string, n_chars);

	uint8_t * __restrict const current_string_end =
		myy_vector_u8_tail_ptr(string);

	vsnprintf((char *) current_string_end, n_chars, utf8_format, args);

	va_end(args);
}

__attribute__((unused))
static inline void gl_text_area_simple_send_to_gpu(
	struct gl_text_area * __restrict const area,
	gl_simple_text_atlas_t const * __restrict const atlas)
{
	gl_text_area_simple_save_to_gpu(area, atlas);
}

__attribute__((unused))
static inline void gl_text_area_proj_changed(
	unsigned int const width,
	unsigned int const height)
{
	myy_4x4_matrix_t projection;
	myy_matrix_4x4_ortho_layered_window_coords(
		&projection,
		width,
		height,
		16 /* layers */);
	gl_text_area_simple_use_program();
	glUniformMatrix4fv(
		gl_simple_text_unifs.projection,
		1,
		GL_FALSE,
		projection.raw_data);
	glUseProgram(0);
}

__attribute__((unused))
static void gl_simple_text_shaders_setup(
	gl_simple_text_atlas_t const * __restrict const atlas)
{
	GLuint program = gl_text_area_simple_use_program();
	gl_simple_text_unifs.projection =
		glGetUniformLocation(program, "projection");
	gl_simple_text_unifs.texture_projection =
		glGetUniformLocation(program, "texture_projection");
	gl_simple_text_unifs.relative_text_offset =
		glGetUniformLocation(program, "relative_text_offset");
	gl_simple_text_unifs.absolute_offset =
		glGetUniformLocation(program, "absolute_offset");
	gl_simple_text_unifs.fonts_texture =
		glGetUniformLocation(program, "fonts_texture");
	gl_simple_text_unifs.rgb =
		glGetUniformLocation(program, "rgb");
	glUniform2f(
		gl_simple_text_unifs.texture_projection,
		1.0f/atlas->tex_width,
		1.0f/atlas->tex_height);
	glUseProgram(0);
}



__attribute__((unused))
static inline bool gl_simple_text_init_atlas(
	gl_simple_text_atlas_t * __restrict const atlas_to_init,
	char const * __restrict const font_pack_filepath,
	struct myy_fh_map_handle * __restrict const initialized_atlas_filehandle)
{
	struct myy_sampler_properties sampler = {
		GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE,
		GL_LINEAR, GL_LINEAR
	};
	return myy_packed_fonts_load(
		font_pack_filepath,
		atlas_to_init,
		initialized_atlas_filehandle,
		&sampler);
}

__attribute__((unused))
static void gl_text_area_simple_draw_prepare_for_batch(
	struct gl_text_area const * __restrict const area,
	gl_simple_text_atlas_t const * __restrict const atlas)
{
	gl_text_area_simple_use_program();
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, atlas->tex_id);
	glUniform1i(gl_simple_text_unifs.fonts_texture, 1);
}

__attribute__((unused))
static void gl_text_area_simple_draw(
	struct gl_text_area const * __restrict const area)
{

	glUniform3f(gl_simple_text_unifs.rgb,
		area->color.r, area->color.g, area->color.b);
	glUniform4f(gl_simple_text_unifs.absolute_offset,
		area->pos.x, area->pos.y, 1.0f, 0.0f);
	glUniform4f(gl_simple_text_unifs.relative_text_offset,
		0.0f, 0.0f, 0.0f, 0.0f);

	gl_text_area_bind_buffers(area);
	glVertexAttribPointer(
		gl_simple_text_attr_relative_xy,
		2, GL_SHORT, GL_FALSE, sizeof(struct myy_gl_char_point),
		(void *) (offsetof(struct myy_gl_char_point, x)));
	glVertexAttribPointer(
		gl_simple_text_attr_in_st,
		2, GL_UNSIGNED_SHORT, GL_FALSE, sizeof(struct myy_gl_char_point),
		(void *) (offsetof(struct myy_gl_char_point, s)));
	glDrawArrays(
		/* How to connect the dots */  GL_TRIANGLES,
		/* Start from */               0,
		/* Dots (vertices) to draws */ area->n_points);
}

__attribute__((unused))
static void gl_text_area_simple_draw_cleanup_after_batch(
	struct gl_text_area const * __restrict const area)
{
	/* Cleaning up */
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDisable(GL_BLEND);
	glUseProgram(0);
}

void glsl_text_draw(struct gl_text_area * __restrict const area);


#endif
