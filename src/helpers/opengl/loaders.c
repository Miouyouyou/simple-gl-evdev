/* This depends on a generated file */

#include <stdint.h>
#include <src/helpers/opengl/loaders.h>
#include <myy/helpers/file.h>
#include <myy/helpers/log.h>
#include <myy/helpers/string.h>

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

int glhLoadShader
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

unsigned int glhCompileProgram
(struct glsl_programs_shared_data const * __restrict const metadata,
 unsigned int const n_shaders,
 enum glsl_shader_name const * __restrict const shaders)
{
	GLuint p = glCreateProgram();
	GLuint ok = 1;
	for (unsigned int s = 0; s < n_shaders; s++) {
		enum glsl_shader_name shader_index = shaders[s];
		struct glsl_shader const shader = metadata->shaders[shader_index];

		GLchar const * const shader_filepath =
			(GLchar const *) metadata->strings+shader.str_pos;
		ok &= (glhLoadShader(shader.type, shader_filepath, p) != 0);
		LOG("Status after loading %s --- %d\n",
		    shader_filepath, ok);
	}

	GLuint ret = 0;
	if (ok) ret = p;
	else LOG("A problem occured during the creation of the program...\n");

	return ret;
}

unsigned int glhLinkAndSaveProgram
(struct glsl_programs_shared_data * __restrict const metadata,
 enum glsl_program_name const program_index,
 GLuint const p)
{
	unsigned int ret = 0;
	if (p) {
		glLinkProgram(p);
		if (check_if_ok(p, GL_PROGRAM_PROBLEMS)) {
			metadata->programs[program_index] = p;
			ret = 1;
		}
		else LOG("An error occured during the linking phase...\n");
	}
	else LOG("Invalid program !\n");
	return ret;
}

unsigned int glhBuildAndSaveProgram
(struct glsl_programs_shared_data * __restrict const metadata,
 unsigned int const n_shaders,
 enum glsl_shader_name const * __restrict const shaders,
 enum glsl_program_name const program_index)
{
	GLuint program = glhCompileProgram(metadata, n_shaders, shaders);
	glhLinkAndSaveProgram(metadata, program_index, program);
	return program;
}

unsigned int glhBuildAndSaveSimpleProgram
(struct glsl_programs_shared_data * __restrict const metadata,
 enum glsl_shader_name vertex_shader,
 enum glsl_shader_name fragment_shader,
 enum glsl_program_name const program_index)
{
	enum glsl_shader_name shaders[2] = {vertex_shader, fragment_shader};
	return glhBuildAndSaveProgram(metadata, 2, shaders, program_index);
}

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



/* TODO : This must be customised */
static void setupTexture()
{
  glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
}

/**
 *  Create n textures buffers and upload the content of
 *  each \0 separated filename in "textures_names" into these buffers.
 * 
 * The raw files are supposed to follow a specific format, described by
 * the 
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
 GLuint * __restrict const texid)
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
		struct myy_fh_map_handle mapped_file_infos =
			fh_MapFileToMemory(current_name);
		if (mapped_file_infos.ok) {

			struct myy_raw_texture_content const * const tex = 
				(struct myy_raw_texture_content const *)
				mapped_file_infos.address;

			glBindTexture(tex->myy_target, texid[i]);
			glPixelStorei(GL_UNPACK_ALIGNMENT, tex->alignment);
			LOG(
			  "glPixelStorei(%d)\n"
			  "glTexImage2D(%d, %d, %d, %d, %d, %d, %d, %d, %p)\n",
			  tex->alignment,
			  tex->myy_target, 0, tex->myy_format,
			  tex->width, tex->height, 0,
			  tex->myy_format, tex->myy_type, tex->data
			);
			glTexImage2D(
			  tex->myy_target, 0, tex->myy_format,
			  tex->width, tex->height, 0,
			  tex->myy_format, tex->myy_type, tex->data
			);
			setupTexture();

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
