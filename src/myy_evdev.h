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

#ifndef MYY_EVDEV
#define MYY_EVDEV 1

#include <libevdev/libevdev.h>

struct myy_evdev_data {
	/* The opened device */
	struct libevdev *dev;
	int fd;
};

unsigned int myy_init_input_devices
(struct myy_evdev_data * const mouse,
 unsigned int const n_devices);

unsigned int myy_free_input_devices
(struct myy_evdev_data * const mouse,
 unsigned int const n_devices);

unsigned int myy_evdev_read_input
(struct myy_evdev_data const * const mouse);

#endif /* MYY_EVDEV */
