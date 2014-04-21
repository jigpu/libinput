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
		device->abs.x = e->value;
		tablet_set_status(tablet, TABLET_UPDATED);
		break;
	case ABS_Y:
		device->abs.y = e->value;
		tablet_set_status(tablet, TABLET_UPDATED);
		break;
	default:
		log_info("Unhandled ABS event code 0x%x\n", e->code);
		break;
	}
}

static void
tablet_update_tool(struct tablet_dispatch *tablet,
		   uint32_t tool,
		   uint32_t enabled)
{
	assert(tool != LIBINPUT_TOOL_NONE);

	if (enabled && tool != tablet->state.tool) {
		tablet->state.tool = tool;
		tablet_set_status(tablet, TABLET_INTERACTED);
	} else if (!enabled && tool == tablet->state.tool) {
		tablet->state.tool = LIBINPUT_TOOL_NONE;
		tablet_unset_status(tablet, TABLET_INTERACTED);
	}
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
			 int post_check)
{
	struct libinput_device *base = &device->base;

	if (tablet->state.tool == tablet->prev_state.tool)
		return;

	if (tablet->state.tool == LIBINPUT_TOOL_NONE) {
		/* Wait for post-check */
		if (post_check)
			return;
	} else if (post_check) {
		/* Already handled in pre-check */
		return;
	}

	pointer_notify_tool_update(
		base, time, tablet->state.tool, tablet->state.tool_serial);
}

static void
tablet_flush(struct tablet_dispatch *tablet,
	     struct evdev_device *device,
	     uint32_t time)
{
	struct libinput_device *base = &device->base;
	li_fixed_t x, y;

	/* pre-update notifications */
	tablet_check_notify_tool(tablet, device, time, 0);

	if (tablet->state.tool != LIBINPUT_TOOL_NONE) {
		if (tablet_has_status(tablet, TABLET_UPDATED)) {
			/* FIXME: apply hysteresis, calibration */
			x = li_fixed_from_int(device->abs.x);
			y = li_fixed_from_int(device->abs.y);

			pointer_notify_motion_absolute(base, time, x, y);
			tablet_unset_status(tablet, TABLET_UPDATED);
		}
	}

	/* post-update notifications */
	tablet_check_notify_tool(tablet, device, time, 1);

	/* replace previous state */
	tablet->prev_state = tablet->state;
}

static void
tablet_process(struct evdev_dispatch *dispatch,
	       struct evdev_device *device,
	       struct input_event *e,
	       uint32_t time)
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
	tablet->state.tool = LIBINPUT_TOOL_NONE;

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
