#include <src/helpers/opengl/loaders.h>
#include <src/helpers/opengl/quads_structures.h>
#include <myy/helpers/struct.h>
#include <myy/helpers/log.h>

#include <myy.h>

#include <stddef.h>

#include <packed_fonts_parser.h>

struct glsl_programs_shared_data glsl_shared_data = {
	.strings = {
	  "shaders/standard.vert\0shaders/standard.frag\0"
	  "shaders/cursor_shader.vsh\0shaders/cursor_shader.fsh\0"
	},
	.shaders = {
	  [text_vsh] = {
	    .type = GL_VERTEX_SHADER,
	    .str_pos = 0,
	  },
	  [text_fsh] = {
	    .type = GL_FRAGMENT_SHADER,
	    .str_pos = 22,
	  },
	  [cursor_vsh] = {
	    .type = GL_VERTEX_SHADER,
	    .str_pos = 44,
	  },
	  [cursor_fsh] = {
	    .type = GL_FRAGMENT_SHADER,
	    .str_pos = 70
	  }
	}
};

#define MANAGED_CODEPOINTS 512
struct myy_packed_fonts_codepoints codepoints[MANAGED_CODEPOINTS] = {0};
struct myy_packed_fonts_glyphdata   glyphdata[MANAGED_CODEPOINTS] = {0};

struct glyph_infos myy_glyph_infos = {
	.stored_codepoints = 0,
	.codepoints_addr = codepoints,
	.glyphdata_addr  = glyphdata
};

enum attributes { attr_xyz, attr_st, n_attrs };
enum uniforms { unif_st, unif_offset, n_uniforms };
enum myy_current_textures_id {
	myy_characters_tex,
	myy_cursor_tex,
	n_textures_id
};
GLuint textures_id[n_textures_id];
GLuint offset_uniform;

void myy_init() {}

void myy_display_initialised(unsigned int width, unsigned int height) {}

void myy_generate_new_state() {}

static unsigned int find_codepoint_in
(struct myy_packed_fonts_codepoints const * __restrict const codepoints,
 uint32_t searched_codepoint) {
	unsigned int i = 1;
	while (codepoints[i].codepoint &&
	       (codepoints[i].codepoint != searched_codepoint))
		i++;
	unsigned int found_index =
	  i * (codepoints[i].codepoint == searched_codepoint);
	return found_index;
}

struct US_normalised_xy {
	int x, y;
};

struct US_normalised_xy normalise_coordinates
(int const width_px, int const height_px) {
	struct US_normalised_xy normalised_dimensions = {
		.x  = (width_px/960.0f)*32767,
		.y = (height_px/540.0f)*32767
	};

	return normalised_dimensions;
}

int16_t myy_copy_glyph
(struct glyph_infos const * __restrict const glyph_infos,
 uint32_t const codepoint,
 US_two_tris_quad_3D * __restrict const quad,
 int16_t x_offset_px) {

	struct myy_packed_fonts_codepoints const * __restrict const codepoints =
	  glyph_infos->codepoints_addr;

	//LOG("[myy_copy_glyph]\n");
	unsigned int codepoint_index =
	  find_codepoint_in(codepoints, codepoint);
	if (codepoint_index) {
		struct myy_packed_fonts_glyphdata const * __restrict const glyphdata =
		  glyph_infos->glyphdata_addr+codepoint_index;

		int16_t glyph_x_offset_px = glyphdata->offset_x_px + x_offset_px;
		int16_t glyph_y_offset_px = glyphdata->offset_y_px;
		int16_t right_px = glyphdata->width_px  + glyph_x_offset_px;
		int16_t up_px    = glyphdata->height_px + glyph_y_offset_px;
		int16_t advance_x = x_offset_px + glyphdata->advance_x_px;

		struct US_normalised_xy normalised_offsets =
		  normalise_coordinates(glyph_x_offset_px, glyph_y_offset_px);
		struct US_normalised_xy right_up =
		  normalise_coordinates(right_px, up_px);

		uint16_t const
		  left  = normalised_offsets.x,
		  right = right_up.x,
		  down  = normalised_offsets.y,
		  up    = right_up.y,
		  layer = 20000,
		  tex_left  = glyphdata->tex_left,
		  tex_right = glyphdata->tex_right,
		  tex_up    = glyphdata->tex_top,
		  tex_down  = glyphdata->tex_bottom;

		/*LOG("  %d↓\n"
		    " Tex: left: %d, right : %d, bottom: %d, top: %d\n",
		    codepoint, tex_left, tex_right, tex_down, tex_up);*/

		quad->points[upleft_corner].s = tex_left;
		quad->points[upleft_corner].t = tex_up;
		quad->points[upleft_corner].x = left;
		quad->points[upleft_corner].y = up;
		quad->points[upleft_corner].z = layer;

		quad->points[downleft_corner].s = tex_left;
		quad->points[downleft_corner].t = tex_down;
		quad->points[downleft_corner].x = left;
		quad->points[downleft_corner].y = down;
		quad->points[downleft_corner].z = layer;

		quad->points[upright_corner].s	= tex_right;
		quad->points[upright_corner].t	= tex_up;
		quad->points[upright_corner].x	= right;
		quad->points[upright_corner].y	= up;
		quad->points[upright_corner].z	= layer;

		quad->points[downright_corner].s = tex_right;
		quad->points[downright_corner].t = tex_down;
		quad->points[downright_corner].x = right;
		quad->points[downright_corner].y = down;
		quad->points[downright_corner].z = layer;

		quad->points[repeated_upright_corner].s = tex_right;
		quad->points[repeated_upright_corner].t = tex_up;
		quad->points[repeated_upright_corner].x = right;
		quad->points[repeated_upright_corner].y = up;
		quad->points[repeated_upright_corner].z = layer;

		quad->points[repeated_downleft_corner].s = tex_left;
		quad->points[repeated_downleft_corner].t = tex_down;
		quad->points[repeated_downleft_corner].x = left;
		quad->points[repeated_downleft_corner].y = down;
		quad->points[repeated_downleft_corner].z = layer;
		return advance_x;
	}
	return x_offset_px;
}

static void prepare_string
(struct glyph_infos const * __restrict const glyph_infos,
 uint32_t const * __restrict const string,
 unsigned int const n_characters,
 US_two_tris_quad_3D * __restrict const quads) {

	int16_t x_offset = 0;
	for (unsigned int i = 0; i < n_characters; i++)
		x_offset = myy_copy_glyph(glyph_infos, string[i], quads+i, x_offset);

}

int string[] = L"Mon super hamster fait du Taekwondo ! Sérieux !";
uint32_t string_size = sizeof(string)/sizeof(uint32_t);
US_two_tris_quad_3D quads[90];

void myy_init_drawing() {
	glhBuildAndSaveSimpleProgram(
	  &glsl_shared_data, text_vsh, text_fsh, glsl_text_program
	);
	glhUploadMyyRawTextures(
	  "textures/super_bitmap.raw\0"
	  "textures/cursor.raw",
	  n_textures_id, textures_id
	);
	glhActiveTextures(textures_id, 2);
	glEnableVertexAttribArray(attr_xyz);
	glEnableVertexAttribArray(attr_st);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	myy_parse_packed_fonts(&myy_glyph_infos, "data/codepoints.dat");
	/*myy_copy_glyph(&myy_glyph_infos, L'a', quads, 0);
	myy_copy_glyph(&myy_glyph_infos, L'p', quads+1, 12*51);*/
	prepare_string(&myy_glyph_infos, string, string_size, quads);

}

static struct mouse_cursor_position { 
	int x, y;
} 
cursor = {200,200};

struct textured_point_3D cursor_icon[4] = {
  {.x = 0.025f, .y = 0, .z = 0.2f, .s = 1.0f, .t = 1.0f},
	{.x = 0, .y = 0, .z = 0.2f, .s = 0.0f, .t = 1.0f},
	{.x = 0, .y = -0.04444f, .z = 0.2f, .s = 0.0f, .t = 0.0f},
	{.x = 0.025f, .y = -0.04444f, .z = 0.2f, .s = 1.0f, .t = 0.0f}
};

struct normalised_float_coords {
	GLfloat x, y;
};

struct normalised_float_coords normalise_float_coords
(int const x, int const y)
{
	/* 960.0f and 540.0f are half 1080p dimensions... Still, random
	 * values coming from nowhere should be removed soon enough. */
	struct normalised_float_coords result = {
		.x = (x / 960.0f) - 1.0f,
		.y = (y / 540.0f) - 1.0f
	};
	return result;
}

void myy_draw() {

	glClear( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT );
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	GLuint const * const programs = glsl_shared_data.programs;
	glUseProgram(programs[glsl_text_program]);
	glUniform2f(unif_offset, 0.0f, 0.0f);
	glUniform1i(unif_st, myy_characters_tex);
	glVertexAttribPointer(attr_xyz, 3, GL_SHORT, GL_TRUE,
	                      sizeof(struct US_textured_point_3D),
	                      (uint8_t *)
	                      (quads)+offsetof(struct US_textured_point_3D, x));
	glVertexAttribPointer(attr_st, 2, GL_UNSIGNED_SHORT, GL_TRUE,
	                      sizeof(struct US_textured_point_3D),
	                      (uint8_t *)
	                      (quads)+offsetof(struct US_textured_point_3D, s));
	glDrawArrays(GL_TRIANGLES, 0, 6 * string_size);
	struct normalised_float_coords cursor_coords =
	  normalise_float_coords(cursor.x, cursor.y);
	glUniform2f(unif_offset, cursor_coords.x, cursor_coords.y);
	glUniform1i(unif_st, myy_cursor_tex);
	glVertexAttribPointer(attr_xyz, 3, GL_FLOAT, GL_FALSE,
	                      sizeof(struct textured_point_3D),
	                      (uint8_t *)
	                      (cursor_icon)+offsetof(struct textured_point_3D, x));
	glVertexAttribPointer(attr_st, 2, GL_FLOAT, GL_FALSE,
	                      sizeof(struct textured_point_3D),
	                      (uint8_t *)
	                      (cursor_icon)+offsetof(struct textured_point_3D, s));
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

void myy_abs_mouse_move(int x, int y) {
	int 
	  current_x = cursor.x,
		new_x = current_x + x;

	new_x >>= ((new_x < 0) << 5);
	new_x = (new_x < 1920 ? new_x : 1920);

	int
		current_y = cursor.y,
		new_y = current_y + y;

	new_y >>= ((new_y < 0) << 5);
	new_y = (new_y < 1080 ? new_y : 1080);

	cursor.x = new_x;
	cursor.y = new_y;

}

void myy_mouse_action(enum mouse_action_type type, int value) {
	LOG("Wheely ! : %d\n", value);
}

void myy_save_state(struct myy_game_state * const state) {}

void myy_resume_state(struct myy_game_state * const state) {}

void myy_cleanup_drawing() {}

void myy_stop() {}

void myy_click(int x, int y, unsigned int button) {}
void myy_doubleclick(int x, int y, unsigned int button) {}
void myy_move(int x, int y) {}
void myy_hover(int x, int y) {}
void myy_key(unsigned int keycode) {}
