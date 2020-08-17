#ifndef MYY_TEXT_H
#define MYY_TEXT_H 1

#include <stdint.h>
#include <helpers/myy_vector/vector.h>
#include <current/opengl.h>

myy_vector_template(u8, uint8_t)

struct myy_gl_double_buffers {
	GLuint id[2];
	uint8_t current_id;
};

static inline void myy_double_buffers_init(
	struct myy_gl_double_buffers * __restrict const buffers)
{
	glGenBuffers(buffers->id, 2);
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
	glDeletebuffers(buffers->id);
}

struct gl_text_area {
	struct myy_gl_double_buffers gl;
	uint32_t n_points;
	myy_vector_u8 * string;
	struct {
		int16_t x, y;
	} pos;
};


static inline void glsl_text_use_program() {
	glUseProgram(glsl_programs[glsl_text_program]);
}

static inline bool gl_text_area_init(
	struct gl_text_area * __restrict const area)
{
	/* I won't go the double buffer route with that
         * little time
         */
	myy_double_buffers_init(&buffer->gl);
	area->n_points = 0;
	area->string = myy_vector_u8_init(64);
	area->pos.x = 0;
	area->pos.y = 0;
}

static inline void gl_text_area_save_for_glsl_text(
	struct gl_text_area * __restrict const area)
{
	/* TODO Use the generator + callback to generate
	 * the geometry and store it, according to 'string'
	 * data.
	 */
}

static inline void gl_text_area_set_text(
	struct gl_text_area * __restrict const area,
	char const * __restrict const utf8_text)
{
	myy_vector_u8 * __restrict const string = &area->string;
	myy_vector_u8_clear(string);
	myy_vector_u8_add(string, (uint8_t *) utf8_text, strlen(utf8_text));
	gl_text_area_save_for_glsl_text();
}

void gl_text_area_string_at(
	struct gl_text_area * __restrict const area,
	void * __restrict const glfont,
	uint8_t const * __restrict string,
	int16_t x, int16_t y);

void glsl_text_draw(struct gl_text_area * __restrict const area);


#endif
