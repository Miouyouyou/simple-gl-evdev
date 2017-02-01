#ifndef MYY_SRC_GENERATED_DATAINFOS
#define MYY_SRC_GENERATED_DATAINFOS 1

#include <myy/current/opengl.h>


/* 1) Énumérer tous les programmes disponibles
 * 2) Énumérer toutes les variables uniformes utilisées
 * 3) Énumérer les attributs de chaque programme (Non essentiel)
 *
 * Création du tableau contenant tous les ids de programme :
 * GLuint programs_ids[glsl_programs_count];
 * Création du tableau contenant tout les ids des valeurs uniformes :
 * GLuint programs_uniforms_ids[glsl_programs_uniforms_count];
 *
 */

struct glsl_program {
	GLuint id;
	GLuint uni[];
};

enum glsl_shader_name {
	text_vsh,
	text_fsh,
	cursor_vsh,
	cursor_fsh,
	glsl_shaders_count
};

enum glsl_program_name {
	glsl_text_program,
	glsl_cursor_program,
	glsl_programs_count
};

struct glsl_shader {
	GLuint type;
	uint32_t str_pos;
};

struct glsl_programs_shared_data {
	GLuint programs[glsl_programs_count];
	struct glsl_shader shaders[glsl_shaders_count];
	GLchar strings[512];
};

#endif
