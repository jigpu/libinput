/*
 * Copyright © 2014 Red Hat, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and
 * its documentation for any purpose is hereby granted without fee, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of the copyright holders not be used in
 * advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission.  The copyright holders make
 * no representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
 * CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */


#ifndef EVDEV_TABLET_H
#define EVDEV_TABLET_H

#include "evdev.h"

/* Completely unscientific, but there should be plenty of room most commonly */
#define MAX_AXES 20

enum tablet_status {
	TABLET_NONE = 0,
	TABLET_UPDATED = 1 << 0,
	TABLET_INTERACTED = 1 << 1,
	TABLET_STYLUS_IN_CONTACT = 1 << 2,
};

struct axis_info {
	int32_t code;
	int32_t axis;
	int32_t updated;
	struct input_absinfo abs;
};

struct device_state {
	uint32_t pad_buttons; /* bitmask of evcode - BTN_MISC */
	uint32_t stylus_buttons; /* bitmask of evcode - BTN_TOUCH */
	enum libinput_tool tool;
	int32_t tool_serial;
};

struct tablet_dispatch {
	struct evdev_dispatch base;
	struct evdev_device *device;

	struct device_state state;
	struct device_state prev_state;

	struct axis_info axes[MAX_AXES];
	enum tablet_status status;
	int n_axes;
};

#endif
