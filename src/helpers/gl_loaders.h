#ifndef MYY_SRC_HELPERS_OPENGL_LOADERS
#define MYY_SRC_HELPERS_OPENGL_LOADERS 1

#include <current/opengl.h>
#include <stdint.h>

/**
 * Compile a simple program, set the provided attributes locations
 * sequentially and link the program.
 *
 * PARAMS :
 * @param vsh_filename The Vertex Shader file's path
 * @param fsh_filename The Fragment Shader file's path
 * @param n_attributes The number of attributes locations to set
 * @param attributes_names The attributes names to set the location of,
 *                         sequentially, starting from 0
 *
 * RETURNS :
 * @return A non-0 program ID if all the steps were successful.
 *         0 otherwise.
 */
GLuint glhSetupProgram
(char const * __restrict const vsh_filename,
 char const * __restrict const fsh_filename,
 uint8_t const n_attributes,
 char const * __restrict const attributes_names);

/**
 * A combination of glhSetupProgram and glUseProgram.
 * See glhSetupProgram */
GLuint glhSetupAndUse
(char const * __restrict const vsh_filename,
 char const * __restrict const fsh_filename,
 uint8_t n_attributes,
 char const * __restrict const attributes_names);

struct myy_raw_texture_content {
	/* The texture width */
	uint32_t const width;
	/* The texture height */
	uint32_t const height;
	/* myy_target = gl_target */
	uint32_t const myy_target;
	/* myy_format = gl_format */
	uint32_t const myy_format;
	/* myy_type   = gl_type   */
	uint32_t const myy_type;
	/* Used for glPixelStorei */
	uint32_t const alignment;
	/* The texture raw data */
	uint32_t const data[];
};

/**
 *  Create n textures buffers and upload the content of
 *  each \0 separated filename in "textures_names" into these buffers.
 * 
 * The raw files are supposed to follow a specific format, described by
 * struct myy_raw_texture_content .
 *
 * Example :
 * GLuint textures_id[2];
 * glhUploadMyyRawTextures("tex/first_tex.raw\0tex/second_tex.raw\0", 2,
 *                         textures_id);
 *
 * CAUTION :
 * - This will replace the current active texture binding by a binding
 *   to the last texture uploaded.
 *
 * PARAMS :
 * @param textures_names The filepaths of the textures to upload
 *                       This is implementation specific.
 *                       For example, tex/first_tex.raw will be read
 *                       from the current Asset archive on Android.
 *
 * @param n              The number of textures to upload
 *
 * @param texid          The buffer receiving the generated textures id
 *
 * ADVICE :
 *   Once the textures uploaded, use glhActiveTextures to enable
 * multi-texturing.
 */
void glhUploadMyyRawTextures
(char const * __restrict const textures_names, int const n,
 GLuint * __restrict const texid);

/**
 * Activate and bind the provided textures, in order.
 * The first texture will be activated and bound to GL_TEXTURE0
 * The following will be activated and bound to GL_TEXTURE0+n
 *
 * parameters :
 *   @param texids The OpenGL textures identifiers to activate
 *   @param n_textures The number of textures to activate
 *
 * Warnings :
 *   Does not check if texids is a valid pointer
 *   Does not check if texids contains n_textures
 *   You'll have to upload the textures to the GPU first
 *   before using this procedure.
 */
void glhActiveTextures
(GLuint const * const texids, int const n_textures);

#endif
