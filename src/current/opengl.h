#ifndef MYY_CURRENT_OPENGL
#define MYY_CURRENT_OPENGL 1

/* You need to define a macro to get function prototypes...
	 Everything's fine... */
#define GL_GLEXT_PROTOTYPES 1

#include <GLES2/gl2.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

#ifdef EGL_OPENGL_ES3_BIT
#define MYY_GLES3_BIT EGL_OPENGL_ES3_BIT
#else
#define MYY_GLES3_BIT EGL_OPENGL_ES3_BIT_KHR
#endif

#define MYY_EGL_COMMON_PC_ATTRIBS \
	EGL_SURFACE_TYPE, EGL_WINDOW_BIT,   \
	EGL_CONFORMANT, EGL_OPENGL_ES2_BIT, \
	EGL_SAMPLES,         4, \
	EGL_RED_SIZE,        5, \
	EGL_GREEN_SIZE,      6, \
	EGL_BLUE_SIZE,       5, \
	EGL_ALPHA_SIZE,      8, \
	EGL_DEPTH_SIZE,     16

#define MYY_EGL_COMMON_MOBILE_ATTRIBS MYY_EGL_COMMON_PC_ATTRIBS

#define MYY_CURRENT_GL_CONTEXT \
	EGL_CONTEXT_MAJOR_VERSION, 2, EGL_NONE, EGL_NONE
#endif
