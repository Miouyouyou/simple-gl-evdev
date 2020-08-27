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

void glsl_text_proj_changed(
	unsigned int const width,
	unsigned int const height);
void glsl_text_init();
void glsl_text_draw();

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
	glsl_text_proj_changed(width, height);
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

void myy_glsl_text_area_init()
{
	GLuint simple_text_program = glhSetupAndUse(
		"shaders/text.vert", "shaders/text.frag",
		2, "relative_xy\0in_st");
	
	if (!simple_text_program) {
		LOG("Could not compile the simple text displayer shader :C\n");
		exit(1);
	}

	glsl_programs[glsl_simple_text_program] = simple_text_program;
	
	gl_text_area_init(&printed_string);
	gl_text_area_set_color(&printed_string, 0, 0, 0, 255);
	gl_text_area_set_global_position(&printed_string, 300, 300);
	gl_text_area_append_text_format(
		&printed_string, "Wonderful pointer at : %p\n", &printed_string);
	gl_text_area_send_to_gpu(&printed_string);
}

void myy_init_drawing() { 
	init_cursor_program();
	glsl_text_init();
	myy_glsl_text_area_init();
}

void myy_draw() {

	static int i = 0;

	/* Clear the screen with a nice blueish color */
	glClear( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT );
	//            RED GREEN  BLUE ALPHA
	glClearColor(0.2f, 0.5f, 0.7f, 1.0f);

	/** Note : Rebinding the same buffer, re-enabling the same vertex 
	           attributes and resetting the texture sampler ID every time
	           is redundant here, as we only have one GLSL program.
	           OpenGL remembers what was set a few seconds ago. 
	           Still, ONLY ONE GLSL program is quite rare, though, so 
	           we'll do it the common way.*/
	
	/* Enable the cursor program */
	GLuint cursor_program = glsl_programs[glsl_cursor_program];
	glUseProgram(cursor_program);
	
	/* Make the cursor texture unit active, and rebind the
	 * texture just in case.
	 * Note : Rebinding is not required here, since the cursor
	 * texture is the only one bound to GL_TEXTURE0+0.
	 * I'm letting this here, just in case you want to develop
	 * something more complex.
	 */
	glActiveTexture(GL_TEXTURE0+glsl_cursor_texture);
	glBindTexture(GL_TEXTURE_2D, glsl_textures[glsl_cursor_texture]);

	/* Blending is required for transparency. Else the alpha channel will
	   be completely ignored and we'll have a black rectangle around the
	   cursor icon */
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
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
	   a copy the current GPU buffer adddress is made and glDrawArrays 
	   will use this address as a base for glVertexAttribPointer
	   offsets.
	   Else, these "offsets" will be considered to be CPU addresses.
	   Therefore, IT IS ESSENTIAL TO BIND THE BUFFER NOW ! */
	glBindBuffer(GL_ARRAY_BUFFER, glsl_buffers[glsl_cursor_buffer]);
	/* The 0 offset is relative to the currently bound buffer's memory.
		Avoid using NULL instead of 0 as NULL is not assured to be 0. */
	glVertexAttribPointer(
		glsl_cursor_attr_xyst, 4, GL_FLOAT, GL_FALSE, 0, (uint8_t *) 0
	);
	/* Draw the cursor. This is it for the CPU part ! */
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	/* This will move the text horizontally between 200 and 711 px */
	gl_text_area_set_global_position(&printed_string, 200 + (i & 511), 200);
	gl_text_area_draw(&printed_string);

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
}
