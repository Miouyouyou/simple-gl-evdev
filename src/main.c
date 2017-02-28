/*
 * Copyright (c) 2012 Arvin Schnell <arvin.schnell@gmail.com>
 * Copyright (c) 2012 Rob Clark <rob@ti.com>
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

/* Based on a egl cube test app originally written by Arvin Schnell */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <assert.h>

#include <myy.h>
#include <myy_drm.h>
#include <myy_evdev.h>
#include <helpers/log.h>

#include <unistd.h>

static void page_flip_handler
(int fd, unsigned int frame,
 unsigned int sec, unsigned int usec,
 void * data)
{
	int *waiting_for_flip = data;
	*waiting_for_flip = 0;
}

int old_drm() {
	struct egl_infos egl;
	struct gbm_infos gbm;
	struct drm_infos drm;
	fd_set fds;
	drmEventContext evctx = {
	  .version = DRM_EVENT_CONTEXT_VERSION,
	  .page_flip_handler = page_flip_handler,
	};
	struct gbm_bo *bo;
	struct drm_fb *fb;
	uint32_t i = 0;
	int ret;

	/* Prepare to read input from Evdev */
	struct myy_evdev_data evdev_data;
	ret = myy_init_input_devices(&evdev_data, 1);
	if (!ret) {
		LOG("Meow ? Where's the mouse ?\n"
		    "Mouse input device not found.\n"
		    "Check that a mouse is plugged and you have sufficent privileges\n");
		goto no_mouse;
	}

	/* Start to use the DRI device */
	ret = init_drm(&drm);
	if (ret) {
		LOG("failed to initialize DRM\n");
		goto program_end;
	}

	FD_ZERO(&fds);
	FD_SET(0, &fds);
	FD_SET(drm.fd, &fds);

	/* Generate a Generic Buffer */
	ret = init_gbm(&drm, &gbm);
	if (ret) {
		LOG("failed to initialize GBM\n");
		goto program_end;
	}

	/* Add an OpenGL ES context to it, using EGL */
	ret = add_gl_context(&egl, &gbm);
	if (ret) {
		LOG("failed to initialize EGL\n");
		goto program_end;
	}

	/* clear the color buffer */
	glClearColor(0.5, 0.5, 0.5, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);
	eglSwapBuffers(egl.display, egl.surface);
	bo = gbm_surface_lock_front_buffer(gbm.surface);
	fb = drm_fb_get_from_bo(bo, &drm);

	/* Save the current CRTC configuration */
	drmModeCrtcPtr prev_crtc = drmModeGetCrtc(drm.fd, drm.crtc_id);
	/* set mode: */
	ret = drmModeSetCrtc(drm.fd, drm.crtc_id, fb->fb_id, 0, 0,
	    &drm.connector_id, 1, drm.mode);
	if (ret) {
		LOG("failed to set mode: %s\n", strerror(errno));
		goto program_end;
	}

	/* Initialise our 'engine' */
	myy_generate_new_state();
	myy_init_drawing();
	myy_display_initialised(
	  gbm_bo_get_width(bo),
	  gbm_bo_get_height(bo)
	);

	while (1) {
		struct gbm_bo *next_bo;
		int waiting_for_flip = 1;

		/* Draw ! */
		myy_draw();

		/* Show ! */
		eglSwapBuffers(egl.display, egl.surface);

		/* Wait until the next VBlank */
		next_bo = gbm_surface_lock_front_buffer(gbm.surface);
		fb = drm_fb_get_from_bo(next_bo, &drm);

		/*
		 * Here you could also update drm plane layers if you want
		 * hw composition
		 */

		ret = drmModePageFlip(
		  drm.fd, drm.crtc_id, fb->fb_id,
		  DRM_MODE_PAGE_FLIP_EVENT, &waiting_for_flip
		);
		if (ret) {
			LOG("failed to queue page flip: %s\n", strerror(errno));
			ret = -1;
			goto program_end;
		}

		while (waiting_for_flip) {

			/* Read input when waiting for Vblank
			 * Given that waiting_for_flip is set to 1 before entering this
			 * loop, this could be rewritten like this :
			 *   do { ... } while (waiting_for_flip)
			*/
			myy_evdev_read_input(&evdev_data);
			ret = select(drm.fd + 1, &fds, NULL, NULL, NULL);
			if (ret < 0) {
				LOG("select err: %s\n", strerror(errno));
				goto program_end;
			} else if (ret == 0) {
				LOG("select timeout!\n");
				ret = -1;
				goto program_end;
			} else if (FD_ISSET(0, &fds)) {
				LOG("user interrupted!\n");
				break;
			}

			/* THIS is the the part that might set waiting_for_flip to 0 */
			/* Now, it might be better to draw directly from inside the
			 * handler, instead of polling here... */
			drmHandleEvent(drm.fd, &evctx);
		}

		/* Prepare for the next draw */
		/* release last buffer to render on again: */
		gbm_surface_release_buffer(gbm.surface, bo);
		bo = next_bo;
	}

	/* Try to restore the previous CRTC */
	drmModeSetCrtc(
	  drm.fd, prev_crtc->crtc_id, prev_crtc->buffer_id,
	  prev_crtc->x, prev_crtc->y,
	  &drm.connector_id, 1, &prev_crtc->mode
	);

program_end:
	myy_free_input_devices(&evdev_data, 1);
no_mouse:
	return ret;
}

/*
 * The whole process, as I understand, tends to be :
 * - Initialise the DRM drivers and get a nice framebuffer
 * - Initialise GBM libraries
 * - Add an OpenGL context to the current GBM managed framebuffer
 * - Run post OpenGL/GBM initialisation preparations
 * - Draw
 * - VSync
 * - Loop to Draw
 */
int main(int argc, char *argv[])
{
	return old_drm();
}
