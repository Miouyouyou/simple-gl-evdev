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

#define MYYT_SIGNATURE 0x5459594d
struct myy_raw_texture_header {
	uint32_t const signature; /* Must be 0x5459594d */
	uint32_t const width;     /* The texture width */
	uint32_t const height;    /* The texture height */
	uint32_t const gl_target; /* myy_target = gl_target */
	uint32_t const gl_format; /* myy_format = gl_format */
	uint32_t const gl_type;   /* myy_type   = gl_type   */
	uint32_t const alignment; /* Used for glPixelStorei */
	uint32_t const reserved;  /* Reserved */
	uint8_t const data[];  /* See struct myy_raw_texture_content */
};
typedef struct myy_raw_texture_header myy_raw_tex_header_t;

struct myy_sampler_properties {
	GLint wrap_s;     /* GL_CLAMP_TO_EDGE, ... */
	GLint wrap_t;     /* GL_CLAMP_TO_EDGE, ... */
	GLint min_filter; /* GL_NEAREST, GL_LINEAR */
	GLint max_filter; /* GL_{MIPMAP,}_{NEAREST,LINEAR} */
};
typedef struct myy_sampler_properties myy_sampler_props_t;

__attribute__((unused))
static inline void setupTexture(struct myy_sampler_properties const props)
{
	glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, props.wrap_s);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, props.wrap_t);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, props.max_filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, props.min_filter);
}

__attribute__((unused))
static inline myy_sampler_props_t myy_sampler_properties_default()
{
	myy_sampler_props_t props = {
		.wrap_s = GL_CLAMP_TO_EDGE,
		.wrap_t = GL_CLAMP_TO_EDGE,
		.min_filter = GL_LINEAR,
		.max_filter = GL_LINEAR
	};
	return props;
}


/**
 *  Create n textures buffers and upload the content of
 *  each \0 separated filename in "textures_names" into these buffers.
 * 
 * The raw files are supposed to follow a specific format, described by
 * struct myy_raw_texture_header .
 *
 * Example :
 * GLuint textures_id[2];
 * glhUploadMyyRawTextures("tex/first_tex.raw\0tex/second_tex.raw\0", 2,
 *                         textures_id);
 *
 * CAUTION :
 * - This will rebind the current active texture binding. If you want to
 *   save your current bindings, use glActiveTexture beforehand.
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
 * @param sampler_props  The sampler setup to use with these textures
 *
 */
void glhUploadMyyRawTextures(
	char const * __restrict const textures_names,
	int const n,
	GLuint * __restrict const texid,
	struct myy_sampler_properties const * __restrict sampler_props);

/**
 * Bind texture_id to the current texture unit, upload the provided
 * data through that texture unit, setup the texture sampler for that
 * texture and generate mipmaps for that texture.
 * 
 * This basically just call glBindTexture, glTexImage2D,
 * glTexParameteri and glGenerateMipmap. If you're not using a specific
 * format, use these calls directly.
 * 
 * The data is supposed myy_raw_tex_header_t format.
 *
 * PARAMS :
 * 
 * @param data 
 * The data to upload. Should follow myy_raw_tex_header_t format.
 *
 * @param texture_id 
 * The texture id identifying the new texture.
 * 
 * @param sampler_props 
 * The texture sampler properties.
 *
 */
void glhUploadMyyRawTextureData(
	uint8_t const * __restrict const data,
	GLuint const texture_id,
	struct myy_sampler_properties const * __restrict const sampler_properties);

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
