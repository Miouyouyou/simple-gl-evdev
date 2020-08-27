/*
	Copyright (c) 2017 Miouyouyou <Myy>

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

#include <stdint.h>
#include <helpers/gl_loaders.h>
#include <helpers/file/file.h>
#include <helpers/log.h>
#include <helpers/string.h>

/* TODO : Replace exit by an "implementation-defined" panic function */
/* exit */
#include <stdlib.h>
#include <unistd.h>

struct gleanup {
	void (*check)(GLuint, GLenum, GLint * );
	int verif;
	void (*log)(GLuint, GLsizei, GLsizei*, GLchar*);
};

#define GL_SHADER_PROBLEMS 0
#define GL_PROGRAM_PROBLEMS 1

static const struct gleanup cleanupMethods[] = {
	[GL_SHADER_PROBLEMS] = {
	  .check = glGetShaderiv,
	  .verif = GL_COMPILE_STATUS,
	  .log   = glGetShaderInfoLog
	},
	[GL_PROGRAM_PROBLEMS] = {
	  .check = glGetProgramiv,
	  .verif = GL_LINK_STATUS,
	  .log   = glGetProgramInfoLog
	}
};

static int check_if_ok
(GLuint const element, GLuint const method_id)
{
	struct gleanup gheckup = cleanupMethods[method_id];

	GLint ok = GL_FALSE;
	gheckup.check(element, gheckup.verif, &ok);

	if (ok == GL_TRUE) return ok;

	int written = 0;
  /* "1KB should be enough for every log" */
	GLchar log_data[1024] = {0};
	gheckup.log(element, 1022, &written, (GLchar *) log_data);
	log_data[written] = 0;
	LOG("Problem was : %s\n", log_data);

	return GL_FALSE;
}

static int glhLoadShader
(GLenum const shaderType,
 char const * __restrict const pathname,
 GLuint const program)
{

	LOG("Shader : %s - Type : %d\n",
	    pathname, shaderType);
  GLuint shader = glCreateShader(shaderType);
	LOG("Loading shader : %s - glCreateShader : %d\n",
	    pathname, shader);
  GLuint ok = 0;

  if (shader) {
		LOG("Shader %s seems ok...\n", pathname);
    struct myy_fh_map_handle mapped_file_infos =
		  fh_MapFileToMemory(pathname);
    if (mapped_file_infos.ok) {
			GLchar const * const * __restrict const shader_code =
			  (GLchar const * const *) &mapped_file_infos.address;

			glShaderSource(shader, 1, shader_code, &mapped_file_infos.length);
      glCompileShader(shader);
      ok = check_if_ok(shader, GL_SHADER_PROBLEMS);
      if (ok) glAttachShader(program, shader);
      glDeleteShader(shader);
      fh_UnmapFileFromMemory(mapped_file_infos);
    }
  }
	LOG("Shader %s -> Status : %d\n", pathname, program);
  return ok;
}

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
 char const * __restrict const attributes_names)
{
	GLuint p = glCreateProgram();

	/* Shaders */
	if (glhLoadShader(GL_VERTEX_SHADER,   vsh_filename, p) &&
	    glhLoadShader(GL_FRAGMENT_SHADER, fsh_filename, p)) {

		LOG("Shaders loaded\n");
		// Flash quiz : Why bound_attribute_name can be updated ?
		char const * bound_attribute_name = attributes_names;
		for (uint32_t i = 0; i < n_attributes; i++) {
			glBindAttribLocation(p, i, bound_attribute_name);
			LOG("Attrib : %s - Location : %d\n", bound_attribute_name, i);
			sh_pointToNextString(bound_attribute_name);
		}
		glLinkProgram(p);
		if (check_if_ok(p, GL_PROGRAM_PROBLEMS)) return p;
	}
	LOG("A problem occured during the creation of the program\n");
	return 0;

}

/**
 * A combination of glhSetupProgram and glUseProgram.
 * See glhSetupProgram */
GLuint glhSetupAndUse
(char const * __restrict const vsh_filename,
 char const * __restrict const fsh_filename,
 uint8_t n_attributes,
 char const * __restrict const attributes_names)
{
	GLuint p =
		glhSetupProgram(vsh_filename, fsh_filename, n_attributes, attributes_names);
	glUseProgram(p);
	return p;
}

void glhUploadMyyRawTextureData(
	uint8_t const * __restrict const data,
	GLuint const texture_id,
	struct myy_sampler_properties const * __restrict const sampler_properties)
{
	myy_raw_tex_header_t const * __restrict const header =
		(myy_raw_tex_header_t const *) data;
	uint8_t const * __restrict const content = header->data;

	glBindTexture(header->gl_target, texture_id);
	glPixelStorei(GL_UNPACK_ALIGNMENT, header->alignment);
	LOG(
		"glPixelStorei(%d)\n"
		"glTexImage2D(%d, %d, %d, %d, %d, %d, %d, %d, %p)\n",
		header->alignment,
		header->gl_target, 0, header->gl_format,
		header->width, header->height, 0,
		header->gl_format, header->gl_type, content);
	glTexImage2D(
		header->gl_target,             /* GL_TEXTURE_2D */     
		0,                             /* Level */             
		header->gl_format,             /* Texture format */    
		header->width, header->height, /* Dimensions */        
		0,                             /* Unused */            
		header->gl_format,             /* Output format */     
		header->gl_type,               /* Texture data type */ 
		content);
	glGenerateMipmap(header->gl_target);
	setupTexture(*sampler_properties);
}

void glhUploadMyyRawTextures(
	char const * __restrict const textures_names,
	int const n,
	GLuint * __restrict const texid,
	struct myy_sampler_properties const * __restrict sample_props)
{
	/* OpenGL 2.x way to load textures is certainly NOT intuitive !
	 * From what I understand :
	 * - The current activated texture unit is changed through
	 *   glActiveTexture.
	 * - glGenTextures will generate names for textures *storage* units.
	 * - glBindTexture will bind the current *storage* unit to the current
	 *   activated texture unit and, on the first time, will define the
	 *   current *storage* unit parameters.
	 *   Example : This storage unit must store GL_TEXTURE_2D textures.
	 * - glTexImage2D will upload the provided data in the texture *storage* unit
	 *   bound to the current texture unit.
	 */

	glGenTextures(n, texid);

	const char *current_name = textures_names;

	for (int i = 0; i < n; i++) {
		/* glTexImage2D
		   Specifies a two-dimensional or cube-map texture for the current
		   texture unit, specified with glActiveTexture. */

		LOG("Loading texture : %s\n", current_name);

		struct myy_sampler_properties const props =
			sample_props ? sample_props[i] : myy_sampler_properties_default();
		struct myy_fh_map_handle mapped_file_infos =
			fh_MapFileToMemory(current_name);

		if (mapped_file_infos.ok) {

			glhUploadMyyRawTextureData(mapped_file_infos.address, texid[i], &props);

			fh_UnmapFileFromMemory(mapped_file_infos);
			sh_pointToNextString(current_name);
		}
		else {
			LOG("You're sure about that file : %s ?\n", current_name);
			exit(1);
		}
	}
}

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
(GLuint const * const texids, int const n_textures)
{
	for (unsigned int i = 0; i < n_textures; i++) {
		glActiveTexture(GL_TEXTURE0+i);
		glBindTexture(GL_TEXTURE_2D, texids[i]);
	}
}
