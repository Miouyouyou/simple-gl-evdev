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

#include <libevdev/libevdev.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <myy.h>
#include <helpers/log.h>

#include <ftw.h>

#include <unistd.h>

#include "myy_evdev.h"

/* parse horizontal relative move events */
static void plus_x(int const code, int const value)
{
	myy_abs_mouse_move(value, 0);
}

/* parse vertical relative move events */
static void plus_y(int const code, int const value) {
	myy_abs_mouse_move(0, value);
}

/* parse mouse wheel like events */
static void plus_wheel(int const code, int const value) {
	myy_mouse_action(myy_mouse_wheel_action, value);
}

/* parse unknown events */
static void plus_wut(int code, int value) {
	LOG("??? : %d\n", value);
}

static void (*plus_rels[])(int code, int value) = {
	[REL_X] = plus_x,
	[REL_Y] = plus_y,
	[REL_WHEEL] = plus_wheel,
	plus_wut
};

/* Parse an input data. */
static void parse_event
(struct input_event * __restrict const event) {
	/* If it's a relative move event, we'll parse it as a mouse move
	 * event */
	if (event->type == EV_REL)
		plus_rels[event->code](event->code, event->value);
}

/* Recatch all dropped input data. That WILL happen, no matter what.
	 Just move the mouse quickly left and right and you WILL have dropped
	 events, even if your program only do event reading.
	 A quick way to trigger this mechanism, if you're not doing DRM work,
	 is to :
	 - hit the 'Scroll Lock' key,
	 - move the mouse,
	 - release 'Scroll Lock',
	 - move it again.
 */
static void parse_dropped_events
(struct input_event * __restrict const event,
 struct libevdev * __restrict const dev) 
{
	int rc;
	//LOG("Resyncing !! ------------------------\n");
	do {
		parse_event(event);
		rc = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_SYNC, event);
	}
	while (rc == LIBEVDEV_READ_STATUS_SYNC);
	//LOG("Resync Complete ! ++++++++++++++++++\n");
}

/* When everything's fine, we'll just parse the last event.
 * If we have some catching to do, we'll read ALL previous input
 * data */
void (*handlers[])() = {
	[LIBEVDEV_READ_STATUS_SUCCESS] = parse_event,
	[LIBEVDEV_READ_STATUS_SYNC] = parse_dropped_events,
};

/* Duck typing : If it returns relative coordinates and has a left
 * button, we consider the device to be a mouse ! */
static inline int is_a_valid_mouse
(struct libevdev const * const dev) {
  return (libevdev_has_event_type(dev, EV_REL) &&
          libevdev_has_event_code(dev, EV_KEY, BTN_LEFT));
}

/* Check if the provided file path is a mouse Evdev input node */
static int mouse_check
(char const * __restrict const file_path, 
 struct stat const * __restrict const file_stats,
 int const filetype)
{
	struct libevdev *dev = NULL;
	int fd = open(file_path, O_RDONLY|O_NONBLOCK);
	int rc = libevdev_new_from_fd(fd, &dev);
	// nftw() continue if 0 is returned. 
	// Since we want to return the fd, we need to avoid 0 values
	int ret = fd + 1;

	if (rc < 0 || !is_a_valid_mouse(dev))
	{
		if (rc >= 0) close(fd);
		ret = 0;
	}
	libevdev_free(dev);
	return ret;
}

/* Scan all /dev/input nodes for a mouse */
static int acquire_mouse() {
	int incremented_fd = 
		ftw("/dev/input", mouse_check, 1);
	int ret = -1;
	if (incremented_fd) ret = incremented_fd - 1;
	return ret;
}

/* Read input from the provided mouse device */
unsigned int myy_evdev_read_input
(struct myy_evdev_data const * const mouse) 
{
	int rc = 0;
	int ret = 1;
	struct libevdev const * const dev = mouse->dev;
	
	/* Read all available inputs data.
	 * rc == -EAGAIN when no input data can be read is available now. */
	do {
		struct input_event ev;
		rc = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL, &ev);
		if (rc >= 0) handlers[rc](&ev, dev);
	}
	while (rc == LIBEVDEV_READ_STATUS_SUCCESS
	    || rc == LIBEVDEV_READ_STATUS_SYNC);

	return 1;
}

/* Scan for mouses and use them. Return 0 if no mouse found */
static unsigned int init_mouse
(struct myy_evdev_data * const mouse) {

	int ret = 0;
	int fd = acquire_mouse();
	if (fd >= 0) {
		libevdev_new_from_fd(fd, &(mouse->dev));
		mouse->fd = fd;
		ret = 1;
	}
	return ret;
}

/* We currently only read mouse input */
unsigned int myy_init_input_devices
(struct myy_evdev_data * const mouse,
 unsigned int const n_devices) 
{
	return init_mouse(mouse);
}

/* Release and stop reading data from the previously acquired devices */
unsigned int myy_free_input_devices
(struct myy_evdev_data * const mouse,
 unsigned int const n_devices)
{
	libevdev_free(mouse->dev);
	close(mouse->fd);
}

// LIBEVDEV_READ_STATUS_SUCCESS 0
// LIBEVDEV_READ_STATUS_SYNC    1
