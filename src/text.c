#include <helpers/gl_loaders.h>
#include <helpers/log.h>
#include <helpers/matrices.h>
#include <helpers/strings.h>
#include "machin.h"
#include "myy.h"

#include <stddef.h>

#include "text.h"

struct gl_char_point {
	/* Signed integers, since people might write outside the screen */
	int16_t x, y;
	float s, t;
};

struct gl_char {
	/* 2 triangles with 3 points each */
	struct gl_char_point points[6];
};
typedef struct gl_char gl_char_t;
myy_vector_template(gl_char, gl_char_t)


struct {
	GLuint proj;
	GLuint z;
	GLuint u_texture;
	GLuint u_color;
	GLuint u_buffer;
	GLuint u_gamma;
} glsl_text_uniforms = {0};

static void glsl_text_store_to_buffer(
	struct gl_text_buffer * __restrict const buffer,
	size_t const n_chars,
	size_t const size,
	void * __restrict const glchars)
{
	glUseProgram(glsl_programs[glsl_text_program]);
	glBindBuffer(GL_ARRAY_BUFFER, buffer->id);
	glBufferData(GL_ARRAY_BUFFER, size, gl_chars, GL_DYNAMIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glUseProgram(0);

	/* 2 triangles = 2 * 3 points = 6 points per character */
	buffer->n_points = n_chars * 6;
}


static void gl_char_point_dump(
	struct gl_char_point const * __restrict const point,
	char const * __restrict const name)
{
	LOG("Point %12s : [ %03d, %03d, %2.5f, %2.5f ]\n",
		name, point->x, point->y, point->s, point->t);
}

static void gl_char_dump(gl_char_t const * __restrict const glc)
{
	LOG("===\n");
	gl_char_point_dump(glc->points,   "left_top");
	gl_char_point_dump(glc->points+1, "left_bottom");
	gl_char_point_dump(glc->points+2, "right_bottom");
	LOG("---\n");
	gl_char_point_dump(glc->points+3, "right_top");
	gl_char_point_dump(glc->points+4, "left_top");
	gl_char_point_dump(glc->points+5, "right_bottom");
	LOG("===\n");
}

static GLint text_tex_id;
static GLint text_buffer_id;
enum glsl_text_program_attribs { glsl_text_attr_pos, glsl_text_attr_st };

struct gl_char first_char;

void glsl_text_proj_changed(
    unsigned int const width,
    unsigned int const height)
{
    myy_4x4_matrix_t projection;
    myy_matrix_4x4_ortho_layered_window_coords(
        &projection,
        width,
        height,
        16 /* layers */);
    glUseProgram(glsl_programs[glsl_text_program]);
    glUniformMatrix4fv(
        glsl_text_uniforms.proj,
        1,
        GL_FALSE,
        projection.raw_data);
    glUseProgram(0);
}

static void glfont_glyph_to_gl_char(
	texture_glyph_t const * __restrict const glyph,
	struct gl_char * __restrict const glc,
	int16_t x, int16_t y)
{
	float const tex_left   = glyph->s0;
	float const tex_right  = glyph->s1;
	float const tex_bottom = glyph->t1;
	float const tex_top    = glyph->t0;

	int16_t const left     = x + glyph->offset_x;
	int16_t const right    = x + glyph->offset_x + glyph->width;
	int16_t const top      = y + (-glyph->height + glyph->offset_y);
	int16_t const bottom   = y + glyph->offset_y;

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

static texture_glyph_t const * glfont_get_glyph_for_codepoint(
	texture_font_t const * __restrict const glfont,
	uint32_t const codepoint)
{
	texture_glyph_t const * __restrict const glyphs =
		glfont->glyphs;
	size_t const n_glyphs = glfont->glyphs_count;
	
	/* The very first glyph is for unknown characters */
	for (size_t i = 1; i < n_glyphs; i++)
	{
		texture_glyph_t const * __restrict const glyph = glyphs+i;

		if (glyph->codepoint == codepoint) return glyph;
	}
	
	return glyphs+0;
}

struct pos { int16_t x, y; };


static size_t glfont_utf8_char_to_gl_char(
	texture_font_t const * __restrict const glfont,
	uint8_t const * __restrict const c,
	struct gl_char * __restrict const glc,
	struct pos * __restrict const p)
{
	struct utf8_codepoint codepoint = utf8_codepoint_and_size(c);
	texture_glyph_t const * __restrict const glyph =
		glfont_get_glyph_for_codepoint(glfont, codepoint.value);
	glfont_glyph_to_gl_char(glyph, glc, p->x, p->y);
	p->x += (int16_t) (glyph->advance_x);
	return codepoint.size;
}

myy_vector_template(gl_char, struct gl_char)

void gl_text_buffer_string_at(
	struct gl_text_buffer * __restrict const buffer,
	void * __restrict const glfont_v,
	uint8_t const * __restrict string,
	int16_t x, int16_t y)
{
  texture_font_t const * __restrict font = (texture_font_t const *)  glfont_v;
	myy_vector_gl_char chars = myy_vector_gl_char_init(32);
	struct pos p = {x, y};
	
	while(*string) {
		struct gl_char current_char = {0};
		size_t utf8_char_size +=
			glfont_utf8_char_to_glchar(glfont, string, &current_char, &p);
		string += utf8_char_size;
		myy_vector_gl_char_add(&chars, 1, &current_char);
	}

	gl_text_buffer_store(
		buffer,
		myy_vector_gl_chars_length(&chars),
		myy_vector_gl_char_allocated_used(&chars),
		myy_vector_gl_char_data(&chars));

	myy_vector_gl_char_free_content(chars);
}

void glsl_text_init()
{
	GLuint const text_program = glhSetupAndUse(
		"shaders/sdf.vert", "shaders/sdf.frag",
		/* n_attributes */     2,
		/* attributes_names */ "pos\0st");

	LOG("glsl_programs : %p\n", glsl_programs);
	glsl_programs[glsl_text_program] = text_program;
	glUseProgram(text_program);

	glEnableVertexAttribArray(glsl_text_attr_pos);
	glEnableVertexAttribArray(glsl_text_attr_st);

	glsl_text_uniforms.proj =
		glGetUniformLocation(text_program, "proj");
	glsl_text_uniforms.z =
		glGetUniformLocation(text_program, "z");
	glsl_text_uniforms.u_texture =
		glGetUniformLocation(text_program, "u_texture");
	glsl_text_uniforms.u_color =
		glGetUniformLocation(text_program, "u_color");
	glsl_text_uniforms.u_buffer =
		glGetUniformLocation(text_program, "u_buffer");
	glsl_text_uniforms.u_gamma =
		glGetUniformLocation(text_program, "u_gamma");

	glGenTextures(1, &text_tex_id);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, text_tex_id);
	glTexImage2D(
		/* target */         GL_TEXTURE_2D,
		/* level */          0,
		/* internalformat */ GL_ALPHA,
		/* width, height */  roboto.tex_width, roboto.tex_height,
		/* border */         0,
		/* format */         GL_ALPHA,
		/* type */           GL_UNSIGNED_BYTE,
		/* data */           roboto.tex_data);
	glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	/* 3 means Texture Unit 3 */
	glUniform1i(glsl_text_uniforms.u_texture, 3);
	glUniform4f(glsl_text_uniforms.u_color, 0, 0, 0, 1.0f);
	glUniform1f(glsl_text_uniforms.u_buffer, 0.5f);
	glUniform1f(glsl_text_uniforms.u_gamma, 0.1f);

	glfont_utf8_char_to_gl_char(&roboto, "A", &first_char);

	glGenBuffers(1, &text_buffer_id);
	glBindBuffer(GL_ARRAY_BUFFER, text_buffer_id);
	glBufferData(GL_ARRAY_BUFFER, sizeof(first_char), &first_char, GL_STATIC_DRAW);
	glUseProgram(0);
}

void glsl_text_draw(struct gl_text_buffer * __restrict const buffer)
{
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

	glUseProgram(glsl_programs[glsl_text_program]);

	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, text_tex_id);
	glBindBuffer(GL_ARRAY_BUFFER, buffer->id);
	glVertexAttribPointer(
		glsl_text_attr_pos,
		2, GL_SHORT, GL_FALSE, sizeof(struct gl_char_point),
		(void *) (offsetof(struct gl_char_point, x)));
	glVertexAttribPointer(
		glsl_text_attr_st,
		2, GL_FLOAT, GL_FALSE, sizeof(struct gl_char_point),
		(void *) (offsetof(struct gl_char_point, s)));

	glDrawArrays(
		/* How to connect the dots */  GL_TRIANGLES,
		/* Start from */               0,
		/* Dots (vertices) to draws */ buffer->n_points);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glUseProgram(0);

	/* Blending disable a lot of performances optimizations
	 * on mobile chips, so keep it disabled when possible.
	 */
	glDisable(GL_BLEND);
}


gl_simple_text_atlas_t simple_text_atlas;
struct myy_fh_map_handle simple_text_atlas_file_handle = {0};

bool gl_text_area_simple_init_atlas()
{
	struct myy_sampler_properties sampler = {
		GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE,
		GL_LINEAR, GL_LINEAR
	};
	return myy_packed_fonts_load(
		"data/fonts_simple.pack",
		&simple_text_atlas,
		&simple_text_atlas_file_handle,
		&sampler);
}

void gl_text_area_simple_shaders_setup()
{
	gl_text_area_simple_use_program();
	myy_4x4_matrix_t texture_projection;
	myy_matrix_4x4_ortho_layered_window_coords(
		&texture_projection,
		simple_text_atlas.tex_width, simple_text_atlas.tex_height,
		1);
	glUseProgram(0);
}

void gl_text_area_simple_draw(
	struct gl_text_area const * __restrict const area)
{
	gl_text_area_simple_use_program();

	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, simple_text_atlas.tex_id);
	glUniform1i(gl_simple_text_unifs.fonts_texture, 1);

	glUniform3f(gl_simple_text_unifs.rgb,
		area->color.r, area->color.g, area->color.b);
	glUniform2f(gl_simple_text_unifs.absolute_offset,
		area->pos.x, area->pos.y);
	glUniform2f(gl_simple_text_unifs.relative_text_offset,
		0, 0);

	gl_text_area_bind_buffers(area);
	glVertexAttribPointer(
		gl_simple_text_attr_relative_xy,
		2, GL_SHORT, GL_FALSE, sizeof(struct myy_gl_char_point),
		(void *) (offsetof(struct gl_char_point, x)));
	glVertexAttribPointer(
		gl_simple_text_attr_in_st,
		2, GL_UNSIGNED_SHORT, GL_FALSE, sizeof(struct myy_gl_char_point),
		(void *) (offsetof(struct gl_char_point, s)));
	glDrawArrays(
		/* How to connect the dots */  GL_TRIANGLES,
		/* Start from */               0,
		/* Dots (vertices) to draws */ area->n_points);

	/* Cleaning up */
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDisable(GL_BLEND);
	glUseProgram(0);


}
