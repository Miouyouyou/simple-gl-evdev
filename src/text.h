#ifndef MYY_TEXT_H
#define MYY_TEXT_H 1

#include <stdint.h>
#include <current/opengl.h>

struct gl_text_buffer {
	GLuint id[1];
	uint32_t n_points;
};

static inline void glsl_text_use_program() {
	glUseProgram(glsl_programs[glsl_text_program]);
}

static inline void gl_text_buffer_init(
	struct gl_text_buffer * __restrict const buffer)
{
	/* I won't go the double buffer route with that
         * little time
         */
	glGenBuffers(1, &buffer->id);
	buffer->n_points = 0;
}


void gl_text_buffer_string_at(
	struct gl_text_buffer * __restrict const buffer,
	void * __restrict const glfont,
	uint8_t const * __restrict string,
	int16_t x, int16_t y);

void glsl_text_draw(struct gl_text_buffer * __restrict const buffer);


#endif
