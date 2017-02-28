/*
 * Copyright (c) 2012 Arvin Schnell <arvin.schnell@gmail.com>
 * Copyright (c) 2012 Rob Clark <rob@ti.com>
 * Copyright (c) 2017 Miouyouyou <myy@miouyouyou.fr>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <myy_drm.h>
#include <myy.h>

#include <helpers/log.h>

// open, read, close
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

// assert
#include <assert.h>

// free, calloc
#include <stdlib.h>

char * connector_states[] = {
  [DRM_MODE_CONNECTED] = "Connected",
  [DRM_MODE_DISCONNECTED] = "Disconnected",
  [DRM_MODE_UNKNOWNCONNECTION] = "???",
};

static void print_connector_infos
(drmModeConnector * __restrict const c, int const c_n)
{
	LOG("Connector %d\n"
	    "Connection state  : %s\n"
	    "Connector id      : %d\n"
	    "Connector type    : %d\n"
	    "Connector type id : %d\n"
	    "Encoders          : %u\n"
	    "Modes             : %u\n"
	    "Properties        : %u\n"
	    "Encoder ID        : %d\n"
	    "Height in mm      : %d\n"
	    "Width in mm       : %d\n",
	    c_n,
	    connector_states[c->connection], 
	    c->connector_id, c->connector_type, c->connector_type_id,
	    c->count_encoders, c->count_modes, c->count_props,
	    c->encoder_id, c->mmHeight, c->mmWidth);
	unsigned int n_modes = c->count_modes;
	for (unsigned int m = 0; m < n_modes; m++) {
		drmModeModeInfoPtr mode = c->modes+m;
		LOG("Connector %d - Mode %d/%d\n"
		    "Clock            : %d\n"
		    "Flags            : %d\n"
		    "Width            : %d\n"
		    "Width Skew       : %d\n"
		    "HSync End        : %d\n"
		    "HSync Start      : %d\n"
		    "Horizontal Total : %d\n"
		    "Name             : %s\n"
		    "Type             : %d\n"
		    "Height           : %d\n"
		    "Vertical Refresh : %d\n"
		    "VScan            : %d\n"
		    "VSync End        : %d\n"
		    "VSync Start      : %d\n"
		    "Vertical Total   : %d\n",
		    c_n, m, n_modes,
		    mode->clock, mode->flags,
		    mode->hdisplay, mode->hskew,
		    mode->hsync_end, mode->hsync_start, mode->htotal,
		    mode->name, mode->type,
		    mode->vdisplay, mode->vrefresh, mode->vscan,
		    mode->vsync_end, mode->vsync_start, mode->vtotal);
	}
}

static uint32_t find_crtc_for_encoder
(drmModeRes const * __restrict resources,
 drmModeEncoder const * __restrict encoder)
{
	int i;

	for (i = 0; i < resources->count_crtcs; i++) {
		/* possible_crtcs is a bitmask as described here:
		 * https://dvdhrm.wordpress.com/2012/09/13/linux-drm-mode-setting-api
		 */
		const uint32_t crtc_mask = 1 << i;
		const uint32_t crtc_id = resources->crtcs[i];
		if (encoder->possible_crtcs & crtc_mask) {
			return crtc_id;
		}
	}

	/* no match found */
	return -1;
}

static uint32_t find_crtc_for_connector
(drmModeRes const * __restrict const resources,
 drmModeConnector const * __restrict const connector,
 struct drm_infos const * __restrict const drm_infos)
{
	int i;

	for (i = 0; i < connector->count_encoders; i++) {
		const uint32_t encoder_id = connector->encoders[i];
		drmModeEncoder *encoder =
		  drmModeGetEncoder(drm_infos->fd, encoder_id);

		if (encoder) {
			const uint32_t crtc_id =
			  find_crtc_for_encoder(resources, encoder);

			drmModeFreeEncoder(encoder);
			if (crtc_id != 0) {
				return crtc_id;
			}
		}
	}

	/* no match found */
	return -1;
}

int init_drm
(struct drm_infos * const drm_infos)
{
	drmModeRes *resources;
	drmModeConnector *connector = NULL;
	drmModeEncoder *encoder = NULL;
	drmModeModeInfo *mode;
	int i, area, refresh_rate, fd;
	uint32_t crtc_id;

	fd = open("/dev/dri/card0", O_RDWR);

	if (fd < 0) {
		LOG("could not open drm device\n");
		return -1;
	}

	resources = drmModeGetResources(fd);
	if (!resources) {
		LOG("drmModeGetResources failed: %s\n", strerror(errno));
		return -1;
	}

	/* find a connected connector: */
	LOG("Connectors : %d\n", resources->count_connectors);
	for (i = 0; i < resources->count_connectors; i++) {
		connector = drmModeGetConnector(
		  fd, resources->connectors[i]
		);
		//print_connector_infos(connector);
		if (connector != NULL &&
		    connector->connection == DRM_MODE_CONNECTED &&
			  connector->count_modes > 0) {
			/* it's connected, let's use this! */
			break;
		}
		drmModeFreeConnector(connector);
		connector = NULL;
	}

	if (!connector || resources->count_connectors == 0) {
		LOG("no connected connector!\n");
		return -1;
	}

	/* find prefered mode or the highest resolution mode: */
	for (i = 0, area = 0, refresh_rate = 0;
	     i < connector->count_modes; i++) {
		drmModeModeInfo *current_mode = &connector->modes[i];

		int current_area =
			current_mode->hdisplay * current_mode->vdisplay;
		int current_vrefresh = current_mode->vrefresh;
		if (current_area >= area && current_vrefresh > refresh_rate) {
			mode = current_mode;
			area = current_area;
			refresh_rate = current_vrefresh;
		}
	}

	if (!mode) {
		LOG("could not find mode!\n");
		return -1;
	}

	/* Find the encoder of the current connector...
	   Isn't there a better way to get it ? */
	for (i = 0; i < resources->count_encoders; i++) {
		encoder = drmModeGetEncoder(fd, resources->encoders[i]);
		if (encoder->encoder_id == connector->encoder_id)
			break;
		drmModeFreeEncoder(encoder);
		encoder = NULL;
	}

	if (encoder) {
		crtc_id = encoder->crtc_id;
	} else {
		LOG(
			"No encoder found !?\n"
			"Searching for a CRTC manually...\n"
		);
		uint32_t crtc_id = find_crtc_for_connector(
		  resources, connector, drm_infos
		);
		if (crtc_id == 0) {
			LOG("no crtc found!\n");
			return -1;
		}

		crtc_id = crtc_id;
	}

	drm_infos->fd = fd;
	drm_infos->mode = mode;
	drm_infos->connector_id = connector->connector_id;
	drm_infos->crtc_id = crtc_id;
	return 0;
}

int init_gbm
(struct drm_infos * __restrict const drm_infos,
 struct gbm_infos * __restrict const gbm_infos)
{
	gbm_infos->dev = gbm_create_device(drm_infos->fd);

	gbm_infos->surface = gbm_surface_create(
	  gbm_infos->dev,
	  drm_infos->mode->hdisplay, drm_infos->mode->vdisplay,
	  GBM_FORMAT_ARGB8888, 
	  GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING
	);

	int ret = 0;
	if (!gbm_infos->surface) {
		LOG("failed to create gbm surface\n");
		ret = -1;
	}

	return ret;
}

int add_gl_context
(struct egl_infos * const egl_infos,
 struct gbm_infos * const gbm_infos)
{
	EGLint major, minor, n;
	GLuint vertex_shader, fragment_shader;
	GLint ret;

	EGLDisplay display;
	EGLConfig config;
	EGLContext context;
	EGLSurface surface;

	static const EGLint context_attribs[] = {
		MYY_CURRENT_GL_CONTEXT,
		EGL_NONE, EGL_NONE
	};

	static const EGLint config_attribs[] = {
		MYY_EGL_COMMON_PC_ATTRIBS,
		EGL_NONE, EGL_NONE
	};

	PFNEGLGETPLATFORMDISPLAYEXTPROC get_platform_display = NULL;
	get_platform_display =
		(void *) eglGetProcAddress("eglGetPlatformDisplayEXT");
	assert(get_platform_display != NULL);

	display =
	  get_platform_display(EGL_PLATFORM_GBM_KHR, gbm_infos->dev, NULL);

	if (!eglInitialize(display, &major, &minor)) {
		LOG("failed to initialize\n");
		return -1;
	}

	LOG("Using display %p with EGL version %d.%d\n",
			display, major, minor);

	LOG("EGL Version \"%s\"\n", eglQueryString(display, EGL_VERSION));
	LOG("EGL Vendor \"%s\"\n", eglQueryString(display, EGL_VENDOR));
	LOG("EGL Extensions \"%s\"\n", eglQueryString(display, EGL_EXTENSIONS));

	if (!eglBindAPI(EGL_OPENGL_ES_API)) {
		LOG("failed to bind api EGL_OPENGL_ES_API\n");
		return -1;
	}

	if (!eglChooseConfig(display, config_attribs, &config, 1, &n) || n != 1) {
		LOG("failed to choose config: %d\n", n);
		return -1;
	}

	context = eglCreateContext(
	  display, config, EGL_NO_CONTEXT, context_attribs
	);
	if (context == NULL) {
		LOG("failed to create context\n");
		return -1;
	}

	surface = eglCreateWindowSurface(
	  display, config, gbm_infos->surface, NULL
	);
	if (surface == EGL_NO_SURFACE) {
		LOG("failed to create egl surface\n");
		return -1;
	}

	/* connect the context to the surface */
	eglMakeCurrent(display, surface, surface, context);

	LOG("GL Extensions: \"%s\"\n", glGetString(GL_EXTENSIONS));
	egl_infos->display = display;
	egl_infos->config  = config;
	egl_infos->surface = surface;
	egl_infos->context = context;

	return 0;
}

/* DRM cleanup */
void drm_fb_destroy_callback
(struct gbm_bo * __restrict const bo,
 void * const data)
{
	myy_cleanup_drawing();
	struct drm_fb *fb = data;
	struct gbm_device *gbm = gbm_bo_get_device(bo);

	if (fb->fb_id)
		drmModeRmFB(fb->drm->fd, fb->fb_id);

	free(fb);
}

struct drm_fb * drm_fb_get_from_bo
(struct gbm_bo * __restrict const bo, 
 struct drm_infos * __restrict const drm_infos)
{
	struct drm_fb *fb = gbm_bo_get_user_data(bo);
	uint32_t width, height, stride, handle;
	int ret;

	if (fb)
		return fb;

	fb = calloc(1, sizeof *fb);
	fb->bo = bo;
	fb->drm = drm_infos;

	width = gbm_bo_get_width(bo);
	height = gbm_bo_get_height(bo);
	stride = gbm_bo_get_stride(bo);
	handle = gbm_bo_get_handle(bo).u32;

	ret = drmModeAddFB(
	  drm_infos->fd,
	  width, height,
	  24, 32,
	  stride, handle, &fb->fb_id
	);
	if (ret) {
		LOG("failed to create fb: %s\n", strerror(errno));
		free(fb);
		return NULL;
	}

	gbm_bo_set_user_data(bo, fb, drm_fb_destroy_callback);

	return fb;
}
