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

#include <config.h>

#include <check.h>
#include <errno.h>
#include <fcntl.h>
#include <libinput.h>
#include <unistd.h>

#include "libinput-util.h"
#include "litest.h"

START_TEST(proximity_in_out)
{
	struct litest_device *dev = litest_current_device();
	struct libinput *li = dev->libinput;
	struct libinput_event_pointer *pointer_event;
	struct libinput_event *event;
	int have_tool_update = 0;
	struct axis_replacement axes[] = {
		{ ABS_DISTANCE, 10 },
		{ -1, -1 }
	};

	litest_drain_events(dev->libinput);

	litest_tablet_proximity_in(dev, 10, 10, axes);
	libinput_dispatch(li);

	while ((event = libinput_get_event(li))) {
		if (libinput_event_get_type(event) == LIBINPUT_EVENT_POINTER_TOOL_UPDATE) {
			have_tool_update++;
			pointer_event = libinput_event_get_pointer_event(event);
			ck_assert_int_eq(libinput_tool_get_type(libinput_event_pointer_get_tool(pointer_event)), LIBINPUT_TOOL_PEN);
		}
		libinput_event_destroy(event);
	}
	ck_assert_int_eq(have_tool_update, 1);

	litest_tablet_proximity_out(dev);
	libinput_dispatch(li);

	while ((event = libinput_get_event(li))) {
		if (libinput_event_get_type(event) == LIBINPUT_EVENT_POINTER_TOOL_UPDATE) {
			have_tool_update++;
			pointer_event = libinput_event_get_pointer_event(event);
			ck_assert_int_eq(libinput_tool_get_type(libinput_event_pointer_get_tool(pointer_event)), LIBINPUT_TOOL_NONE);
		}
		libinput_event_destroy(event);
	}
	ck_assert_int_eq(have_tool_update, 2);
}
END_TEST

START_TEST(motion)
{
	struct litest_device *dev = litest_current_device();
	struct libinput *li = dev->libinput;
	struct libinput_event_pointer *pointer_event;
	struct libinput_event *event;
	int have_motion = 0;
	struct axis_replacement axes[] = {
		{ ABS_DISTANCE, 10 },
		{ -1, -1 }
	};

	litest_drain_events(dev->libinput);

	litest_tablet_proximity_in(dev, 10, 10, axes);
	libinput_dispatch(li);

	while ((event = libinput_get_event(li))) {
		if (libinput_event_get_type(event) == LIBINPUT_EVENT_POINTER_MOTION_ABSOLUTE) {
			have_motion++;
		}
		libinput_event_destroy(event);
	}
	ck_assert_int_eq(have_motion, 1);

	litest_tablet_motion(dev, 20, 10, axes);
	libinput_dispatch(li);

	while ((event = libinput_get_event(li))) {
		if (libinput_event_get_type(event) == LIBINPUT_EVENT_POINTER_MOTION_ABSOLUTE) {
			have_motion++;
		}
		libinput_event_destroy(event);
	}
	ck_assert_int_eq(have_motion, 2);

	/* Proximity out must not emit motion events */
	litest_tablet_proximity_out(dev);
	libinput_dispatch(li);

	while ((event = libinput_get_event(li))) {
		ck_assert(libinput_event_get_type(event) != LIBINPUT_EVENT_POINTER_MOTION_ABSOLUTE);
		libinput_event_destroy(event);
	}
}
END_TEST

int
main(int argc, char **argv)
{
	litest_add("tablet:proximity-in-out", proximity_in_out, LITEST_TABLET, LITEST_ANY);
	litest_add("tablet:motion", motion, LITEST_TABLET, LITEST_ANY);

	return litest_run(argc, argv);
}
