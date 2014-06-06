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
#include "config.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include "evdev-tablet.h"

#define tablet_set_status(tablet,s) (tablet->status |= (s));
#define tablet_unset_status(tablet,s) (tablet->status &= ~(s));
#define tablet_has_status(tablet,s) (!!(tablet->status & s))

static void
tablet_process_absolute(struct tablet_dispatch *tablet,
			struct evdev_device *device,
			struct input_event *e,
			uint32_t time)
{
	switch (e->code) {
	case ABS_X:
	case ABS_Y:
		set_bit(tablet->changed_axes, e->code);
		tablet_set_status(tablet, TABLET_AXES_UPDATED);
		break;
	default:
		log_info("Unhandled ABS event code 0x%x\n", e->code);
		break;
	}
}

static inline enum libinput_tablet_axis
evcode_to_axis(const uint32_t evcode)
{
	enum libinput_tablet_axis axis;

	switch (evcode) {
	case ABS_X:
		axis = LIBINPUT_TABLET_AXIS_X;
		break;
	case ABS_Y:
		axis = LIBINPUT_TABLET_AXIS_Y;
		break;
	default:
		axis = -1;
		break;
	}

	return axis;
}

static void
tablet_update_tool(struct tablet_dispatch *tablet,
		   int32_t tool,
		   bool enabled)
{
	assert(tool != LIBINPUT_TOOL_NONE);

	if (enabled && tool != tablet->state.tool_type)
		tablet->state.tool_type = tool;
	else if (!enabled && tool == tablet->state.tool_type)
		tablet->state.tool_type = LIBINPUT_TOOL_NONE;
}

static void
tablet_notify_axes(struct tablet_dispatch *tablet,
		   struct evdev_device *device,
		   uint32_t time)
{
	struct libinput_device *base = &device->base;
	unsigned char changed_axes[NCHARS(LIBINPUT_TABLET_AXIS_CNT + 1)] = { 0 };
	bool axis_update_needed = false;
	
	/* A lot of the ABS axes don't apply to tablets, so we loop through the
	 * values in here so we don't waste time checking axes that will never
	 * update
	 */
	uint32_t check_axes[] = {
		ABS_X,
		ABS_Y,
	};

	uint32_t *evcode;
	ARRAY_FOR_EACH(check_axes, evcode) {
		const struct input_absinfo *absinfo;
		enum libinput_tablet_axis axis;

		if (!bit_is_set(tablet->changed_axes, *evcode))
			continue;

		absinfo = libevdev_get_abs_info(device->evdev, *evcode);
		axis = evcode_to_axis(*evcode);

		switch (*evcode) {
		case ABS_X:
		case ABS_Y:
			tablet->axes[axis] = absinfo->value;
			break;
		default:
			log_bug_libinput("Unhandled axis update with evcode %x\n",
					 *evcode);
		}

		absinfo = libevdev_get_abs_info(device->evdev, *evcode);

		set_bit(changed_axes, axis);
		clear_bit(tablet->changed_axes, *evcode);
		axis_update_needed = true;
	}

	if (axis_update_needed)
		tablet_notify_axis(base, time, changed_axes, tablet->axes);
}

static void
tablet_process_key(struct tablet_dispatch *tablet,
		   struct evdev_device *device,
		   struct input_event *e,
		   uint32_t time)
{
	switch (e->code) {
	case BTN_TOOL_PEN:
	case BTN_TOOL_RUBBER:
	case BTN_TOOL_BRUSH:
	case BTN_TOOL_PENCIL:
	case BTN_TOOL_AIRBRUSH:
	case BTN_TOOL_FINGER:
	case BTN_TOOL_MOUSE:
	case BTN_TOOL_LENS:
		/* These codes have an equivalent libinput_tool value */
		tablet_update_tool(tablet, e->code, e->value);
		break;
	default:
		break;
	}
}

static void
tablet_process_misc(struct tablet_dispatch *tablet,
		    struct evdev_device *device,
		    struct input_event *e,
		    uint32_t time)
{
	switch (e->code) {
	case MSC_SERIAL:
		tablet->state.tool_serial = e->value;
		break;
	default:
		log_info("Unhandled MSC event code 0x%x\n", e->code);
		break;
	}
}

static void
tablet_check_notify_tool(struct tablet_dispatch *tablet,
			 struct evdev_device *device,
			 uint32_t time,
			 bool post_check)
{
	struct libinput_device *base = &device->base;
	struct libinput_tool *tool;
	struct libinput_tool *new_tool = NULL;

	if (tablet->state.tool_type == tablet->prev_state.tool_type)
		return;

	if (tablet->state.tool_type == LIBINPUT_TOOL_NONE) {
		/* Wait for post-check */
		if (post_check)
			return;
	} else if (post_check) {
		/* Already handled in pre-check */
		return;
	}

	/* Check if we already have the tool in our list of tools */
	list_for_each(tool, &tablet->tool_list, link) {
		if (tablet->state.tool_type == tool->type &&
		    tablet->state.tool_serial == tool->serial) {
			libinput_tool_ref(tool);
			new_tool = tool;
			break;
		}
	}

	/* If we didn't already have the tool in our list of tools, add it */
	if (new_tool == NULL) {
		new_tool = zalloc(sizeof *new_tool);
		*new_tool = (struct libinput_tool) {
			.type = tablet->state.tool_type,
			.serial = tablet->state.tool_serial,
			.refcount = 2,
		};

		list_insert(&tablet->tool_list, &new_tool->link);
	}
	
	tablet_notify_tool_update(base, time, new_tool);
}

static void
tablet_flush(struct tablet_dispatch *tablet,
	     struct evdev_device *device,
	     uint32_t time)
{
	/* pre-update notifications */
	tablet_check_notify_tool(tablet, device, time, false);

	if (tablet_has_status(tablet, TABLET_AXES_UPDATED)) {
		tablet_notify_axes(tablet, device, time);

		tablet_unset_status(tablet, TABLET_AXES_UPDATED);
	}

	/* post-update notifications */
	tablet_check_notify_tool(tablet, device, time, true);

	/* replace previous state */
	tablet->prev_state = tablet->state;
}

static void
tablet_process(struct evdev_dispatch *dispatch,
	       struct evdev_device *device,
	       struct input_event *e,
	       uint64_t time)
{
	struct tablet_dispatch *tablet =
		(struct tablet_dispatch *)dispatch;

	switch (e->type) {
	case EV_ABS:
		tablet_process_absolute(tablet, device, e, time);
		break;
	case EV_KEY:
		tablet_process_key(tablet, device, e, time);
		break;
	case EV_MSC:
		tablet_process_misc(tablet, device, e, time);
		break;
	case EV_SYN:
		tablet_flush(tablet, device, time);
		break;
	default:
		log_error("Unexpected event type 0x%x\n", e->type);
		break;
	}
}

static void
tablet_destroy(struct evdev_dispatch *dispatch)
{
	struct tablet_dispatch *tablet =
		(struct tablet_dispatch*)dispatch;
	struct libinput_tool *tool, *tmp;

	list_for_each_safe(tool, tmp, &tablet->tool_list, link)
		libinput_tool_unref(tool);
	
	free(tablet);
}

static struct evdev_dispatch_interface tablet_interface = {
	tablet_process,
	tablet_destroy
};

static int
tablet_init(struct tablet_dispatch *tablet,
	    struct evdev_device *device)
{
	tablet->base.interface = &tablet_interface;
	tablet->device = device;
	tablet->status = TABLET_NONE;
	tablet->state.tool_type = LIBINPUT_TOOL_NONE;

	list_init(&tablet->tool_list);

	return 0;
}

struct evdev_dispatch *
evdev_tablet_create(struct evdev_device *device)
{
	struct tablet_dispatch *tablet;

	tablet = zalloc(sizeof *tablet);
	if (!tablet)
		return NULL;

	if (tablet_init(tablet, device) != 0) {
		tablet_destroy(&tablet->base);
		return NULL;
	}

	return  &tablet->base;
}
