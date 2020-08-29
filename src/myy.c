/*
	Copyright (c) 2020 Miouyouyou <Myy>

	Permission is hereby granted, free of charge, to any person obtaining
	a copy of this software and associated documentation files
	(the "Software"), to deal in the Software without restriction,
	including without limitation the rights to use, copy, modify, merge,
	publish, distribute, sublicense, and/or sell copies of the Software,
	and to permit persons to whom the Software is furnished to do so,
	subject to the following conditions:

	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
	MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
	IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
	CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
	TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
	SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <helpers/gl_loaders.h>
#include <helpers/log.h>
#include "text.h"
#include <myy.h>

#include <stddef.h>

// ------ GLSL Data

GLuint glsl_programs[n_glsl_programs] = {0};
GLuint glsl_textures[n_glsl_textures] = {0};
GLuint glsl_buffers[n_glsl_buffers]   = {0};
GLuint glsl_cursor_uniforms[n_glsl_cursor_uniforms] = {0};

static struct mouse_cursor_position { int x, y; } cursor = {200, 200};

struct screen_props { unsigned int width, height; }
	screen_size = { 1920, 1080 };

// Text functions

// ----- Code

void myy_generate_new_state() {}


void myy_display_initialised(
	unsigned int const width,
	unsigned int const height)
{
	{
		/* Inverse multiplications tend to be slightly faster than
			divisions in GLSL. */
		float
			half_width  = width/2.0f,
			half_height = height/2.0f,
			inv_half_width = 1/half_width,
			inv_half_height = 1/half_height,
			recenter_width  = -1,
			recenter_height = -1;

		/* This expects that the cursor program has been linked prior to this
		call ! */
		GLuint cursor_program = glsl_programs[glsl_cursor_program];
		glUseProgram(cursor_program);

		glUniform4f(
			glsl_cursor_uniforms[glsl_cursor_unif_norm],
			inv_half_width, inv_half_height,
			recenter_width, recenter_height
		);
		glUseProgram(0);
	}
	gl_text_area_proj_changed(width, height);


}

static void init_cursor_program() {
	/* Link and use our cursor shader */
	GLuint cursor_program = glhSetupAndUse(
		"shaders/cursor.vsh", "shaders/cursor.fsh",
		1, "xyst"
	);
	glsl_programs[glsl_cursor_program] = cursor_program;

	glUseProgram(cursor_program);

	glEnableVertexAttribArray(glsl_cursor_attr_xyst);

	/* Get our uniforms locations as with GLSL 2.x the uniforms locations
	   cannot be set directly (GLSL 3.1 feature) */
	glsl_cursor_uniforms[glsl_cursor_unif_tex] =
		glGetUniformLocation(cursor_program, "sampler");
	glsl_cursor_uniforms[glsl_cursor_unif_norm] =
		glGetUniformLocation(cursor_program, "px_to_norm");
	glsl_cursor_uniforms[glsl_cursor_unif_position] =
		glGetUniformLocation(cursor_program, "cursor_pos");

	/* Upload and activate the cursor texture */
	glhUploadMyyRawTextures(
		"textures/cursor.raw",
		n_glsl_textures, glsl_textures,
		NULL
	);
	glhActiveTextures(glsl_textures, 1);

	/* Store the cursor quad in the GPU memory */
	glGenBuffers(n_glsl_buffers, glsl_buffers);
	GLuint cursor_buffer = glsl_buffers[glsl_cursor_buffer];
	/* Most buffers operations are done on the currently bound buffer */
	glBindBuffer(GL_ARRAY_BUFFER, cursor_buffer);

	/* x, y are expressed in pixels.
	 * s is the normalised coordinate of the texture. 0 : left - 1 : right
	 * t is the normalised coordinate of the texture. 0 : down - 1 : up
	 */
	float left = 0, right = 24, up = 0, down = -24;
	float cursor_icon[16] = {
		/*  x     y  s  t */
		right,   up, 1, 1,
		left,    up, 0, 1,
		left,  down, 0, 0,
		right, down, 1, 0
	};

	glBufferData(
		GL_ARRAY_BUFFER, 16*sizeof(float), cursor_icon, GL_STATIC_DRAW
	);

	glUseProgram(0);
}

gl_text_area_t printed_string;
gl_simple_text_atlas_t myy_font_atlas;
struct myy_fh_map_handle myy_font_atlas_filehandle;

void myy_glsl_text_area_init()
{
	gl_simple_text_atlas_t * __restrict const simple_atlas = &myy_font_atlas;
	/* FIXME Move this somewhere else.
	 * However, don't forget that initializing the atlas requires
	 * sending textures to the GPU using OpenGL, so OpenGL
	 * must be initialized.
	 */
	{
		bool const atlas_initialized = gl_simple_text_init_atlas(
			simple_atlas,
			"data/font_pack_meta.dat",
			&myy_font_atlas_filehandle);
		if (!atlas_initialized) {
			fh_UnmapFileFromMemory(myy_font_atlas_filehandle);
			LOG("Could not find data/fonts_test.dat");
			exit(1);
		}
	}
	{
		GLuint simple_text_program = glhSetupAndUse(
			"shaders/text.vert", "shaders/text.frag",
			2, "relative_xy\0in_st");
		
		if (!simple_text_program) {
			LOG("Could not compile the simple text displayer shader :C\n");
			exit(1);
		}

		glsl_programs[glsl_simple_text_program] = simple_text_program;

		gl_simple_text_shaders_setup(simple_atlas);
	}

	gl_text_area_init(&printed_string);
	gl_text_area_set_color(&printed_string, 0, 0, 0, 255);
	gl_text_area_set_global_position(&printed_string, 300, 300);
	gl_text_area_append_text_format(
		&printed_string, "Wonderful pointer : %p\nIsn't it ?", &printed_string);

	gl_text_area_simple_send_to_gpu(&printed_string, simple_atlas);
}

void myy_init_drawing() { 
	init_cursor_program();
	myy_glsl_text_area_init();

	/* Depth testing is enabled, in order to be able to setup a 'depth'
	 * to our cursor (inside the shader) and have it displayed on top of
	 * other objects, like a moving text for example */
	glEnable(GL_DEPTH_TEST);
}

void myy_draw() {

	/* This is a semi-global variable.
	 * This variable is initialized ONCE, then it keeps its
	 * value between invocations of the same function.
	 * This is why the increment at the bottom of the function
	 * works.
	 */
	static int i = 0;

	/* Clear the screen with a nice blueish color */
	glClear( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT );
	//            RED GREEN  BLUE ALPHA
	glClearColor(0.2f, 0.5f, 0.7f, 1.0f);

	/* When displaying transparent objects, the order is KEY.
	 * Remember that 'transparency' is a fallacy in 3D rendering.
	 * The foreground pixel color is just mixed with (or copied from)
	 * the current background pixel color to give that "transparent"
	 * illusion.
	 * So we NEED to draw back to front, in order to ensure that each
	 * front object 'transparent' pixel takes into account the background
	 * objects pixel colors, else things will start to look VERY ugly.
	 * Note that drawing back to front KILLS a lot of performance and
	 * optimization on embedded devices. That's why you might want to
	 * look for more "DRM" ways of display cursors on the screen, with
	 * compositing and such, in order to get real performances.
	 */

	{
		gl_text_area_simple_draw_prepare_for_batch(
			&printed_string, &myy_font_atlas);

		/* Simple moving text effects */
		/* This will move the text horizontally between 200 and 711 px 
		 * 
		 * Boolean magic :
		 * ((i >> 9) & 1) will take the value of the 9th bit only.
		 * The bit will be 0 if 'i' is below 512 * an odd number
		 * For example : 0-511, 1024-1535, ...
		 * The bit will be 1 if 'i' is below 512 * an even number
		 * For example : 512-1023, 1536-2047, ...
		 * 
		 * ^ 1 will invert this bit (so 0 will become 1 and 1 will become
		 * 0)
		 * 
		 * That way, we move the text 512 pixels to the right, then
		 * 512 pixels to the left...
		 */
		unsigned int const move_forward = ((i >> 9) & 1) ^1;
		if (move_forward) {
			gl_text_area_set_global_position(
				&printed_string, 200 + (i & 511), 200);
		}
		else {
			gl_text_area_set_global_position(
				&printed_string, 711 - (i & 511), 200);
		}
		gl_text_area_simple_draw(&printed_string);

		gl_text_area_simple_draw_cleanup_after_batch(&printed_string);
	}

	/* Cursor drawing */
	/* Enable the cursor program */
	GLuint cursor_program = glsl_programs[glsl_cursor_program];
	glUseProgram(cursor_program);

	/* Blending is required for transparency. Else the alpha channel will
	   be completely ignored and we'll have a black rectangle around the
	   cursor icon */
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	/* Make the cursor texture unit active, and rebind the
	 * texture just in case.
	 * Note : Rebinding is not required here, since the cursor
	 * texture is the only one bound to GL_TEXTURE0+0.
	 * I'm letting this here, just in case you want to develop
	 * something more complex.
	 */
	glActiveTexture(GL_TEXTURE0+glsl_cursor_texture);
	glBindTexture(GL_TEXTURE_2D, glsl_textures[glsl_cursor_texture]);

	/* Enable the cursor texture. */
	glUniform1i(
		glsl_cursor_uniforms[glsl_cursor_unif_tex],
		glsl_cursor_texture);
	/* Set the current cursor position. This is ESSENTIAL */

	glUniform2f(
		glsl_cursor_uniforms[glsl_cursor_unif_position], 
		(float) cursor.x,
		(float) screen_size.height - cursor.y);

	/* -Get ready to send data.- */
	/* When calling glVertexAttribPointer, if an ARRAY_BUFFER is bound,
	   the last argument is an offset, inside this buffer, from which
	   glDrawArrays should start picking attributes information from.
	   If no ARRAY_BUFFER is bound, then the latest address is considered
	   to be a simple CPU memory address (pointer).
	   Since we sent the attributes data to the GPU beforehand, we bind
	   the buffer and then pass '0' as the last argument, meaning that
	   glDrawArrays should start picking vertices information for the
	   attribute "xyst" from (the bound buffer address + 0).
	   In this context, 0 is NOT a null pointer. It's an offset.
	   Don't pass NULL or nullptr, hoping for the same results. */
	glBindBuffer(GL_ARRAY_BUFFER, glsl_buffers[glsl_cursor_buffer]);
	/* The 0 offset is relative to the currently bound buffer's memory.
	   Avoid using NULL instead of 0 as NULL is not assured to be 0. */
	glVertexAttribPointer(
		glsl_cursor_attr_xyst, 4, GL_FLOAT, GL_FALSE, 0, (uint8_t *) 0);
	/* Draw the cursor. This is it for the CPU part ! */
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	// Increment the frame number
	i++;
}

void myy_cleanup_drawing() {
	glFinish();
	glDeleteProgram(glsl_programs[glsl_cursor_program]);
	glDeleteBuffers(n_glsl_buffers, glsl_buffers);
}

/* Evdev will send ABSOLUTE movement offsets like -4, +2, +9, -1
 * By default, Evdev send :
 * - +X for right, -X for left
 * - -Y for up,    +Y for down
 * 
 * We'll associate these offsets to pixels movement. So something like
 * (0, -9) will be considered as move the cursor 9 pixels up, and
 * (-4, 0) will be considered as move the cursor 4 pixels left.
 * This is completely arbitrary though.
 * 
 * We also keep the cursor inside the screen by clamping :
 * - x between [0, width]
 * - y between [0, height]
 * 
 * Note that instead of int32 we could use uint32 and do a "% width", 
 * "% height" to have connected edges (i.e. sending the cursor outside
 * the left edge would teleport the cursor near the right edge, ...).
*/
void myy_abs_mouse_move(int x, int y) {
	unsigned int 
		width  = screen_size.width,
		height = screen_size.height;
	
	int32_t
		current_x = cursor.x,
		new_x = current_x + x;

	new_x = (new_x >= 0) ? new_x : 0;
	new_x = (new_x < width ? new_x : width);

	int32_t
		current_y = cursor.y,
		new_y = current_y + y;

	new_y = (new_y >= 0) ? new_y : 0;
	new_y = (new_y < height ? new_y : height);

	cursor.x = new_x;
	cursor.y = new_y;
}

/* Invoked when the mouse wheel is used, but this isn't useful here */
void myy_mouse_action(enum mouse_action_type action, int value) {
  // Doing a printf here would be of no use as we cannot see the
  // terminal until the application shutdown
	exit(1);
}
