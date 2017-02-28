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

#ifndef MYY_DRM_H
#define MYY_DRM_H 1

#include <xf86drm.h>
#include <xf86drmMode.h>
#include <gbm.h>

#include <current/opengl.h>

struct egl_infos {
	EGLDisplay display;
	EGLConfig config;
	EGLContext context;
	EGLSurface surface;
};

struct gbm_infos {
	struct gbm_device *dev;
	struct gbm_surface *surface;
};

struct drm_infos {
	int fd;
	drmModeModeInfo *mode;
	uint32_t crtc_id;
	uint32_t connector_id;
};

struct drm_fb {
	struct gbm_bo *bo;
	struct drm_infos *drm;
	uint32_t fb_id;
};

int init_drm
(struct drm_infos * const drm_infos);

int init_gbm
(struct drm_infos * __restrict const drm_infos,
 struct gbm_infos * __restrict const gbm_infos);

int add_gl_context
(struct egl_infos * const egl_infos,
 struct gbm_infos * const gbm_infos);

struct drm_fb * drm_fb_get_from_bo
(struct gbm_bo * __restrict const bo, 
 struct drm_infos * __restrict const drm_infos);

void drm_fb_destroy_callback
(struct gbm_bo * __restrict const bo,
 void * const data);

#endif
