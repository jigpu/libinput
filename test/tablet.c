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
#include <stdbool.h>

#include "libinput-util.h"
#include "litest.h"

START_TEST(proximity_in_out)
{
	struct litest_device *dev = litest_current_device();
	struct libinput *li = dev->libinput;
	struct libinput_event_tablet *tablet_event;
	struct libinput_event *event;
	bool have_tool_update = false,
	     have_proximity_out = false,
	     proximity_out_clears_buttons = true;

	struct axis_replacement axes[] = {
		{ ABS_DISTANCE, 10 },
		{ -1, -1 }
	};

	litest_drain_events(dev->libinput);

	litest_tablet_proximity_in(dev, 10, 10, axes);
	libinput_dispatch(li);

	while ((event = libinput_get_event(li))) {
		if (libinput_event_get_type(event) == LIBINPUT_EVENT_TABLET_TOOL_UPDATE) {
			struct libinput_tool * tool;

			have_tool_update++;
			tablet_event = libinput_event_get_tablet_event(event);
			tool = libinput_event_tablet_get_tool(tablet_event);
			ck_assert_int_eq(libinput_tool_get_type(tool), LIBINPUT_TOOL_PEN);
		}
		libinput_event_destroy(event);
	}
	ck_assert(have_tool_update);

	litest_tablet_proximity_out(dev);
	libinput_dispatch(li);

	while ((event = libinput_get_event(li))) {
		if (libinput_event_get_type(event) == LIBINPUT_EVENT_TABLET_PROXIMITY_OUT)
			have_proximity_out = true;

		libinput_event_destroy(event);
	}
	ck_assert(have_proximity_out);

	litest_tablet_proximity_out_release_buttons(dev, 10, 10, axes);
	libinput_dispatch(li);

	while ((event = libinput_get_event(li))) {
		enum libinput_event_type type = libinput_event_get_type(event);
		libinput_event_destroy(event);

		/* Skip all events until we get a proximity out event */
		if (type == LIBINPUT_EVENT_TABLET_PROXIMITY_OUT)
			break;
	}

	while ((event = libinput_get_event(li))) {
		if (libinput_event_get_type(event) == LIBINPUT_EVENT_TABLET_BUTTON)
			proximity_out_clears_buttons = false;

		libinput_event_destroy(event);
	}
	ck_assert(proximity_out_clears_buttons);

	litest_tablet_proximity_out_clear_axes(dev, 10, 10, axes);
	libinput_dispatch(li);

	while ((event = libinput_get_event(li))) {
		enum libinput_event_type type = libinput_event_get_type(event);
		libinput_event_destroy(event);

		/* Skip all events until we get a proximity out event */
		if (type == LIBINPUT_EVENT_TABLET_PROXIMITY_OUT)
			break;
	}

	while ((event = libinput_get_event(li))) {
		if (libinput_event_get_type(event) == LIBINPUT_EVENT_TABLET_AXIS) {
			struct libinput_event_tablet *tablet_event =
				(struct libinput_event_tablet*)event;

			for (unsigned int a = 0; a <= LIBINPUT_TABLET_AXIS_CNT; a++) {
				/* the x axis is the only axis that actually
				 * updated, so don't bother checking it */
				if (a == LIBINPUT_TABLET_AXIS_X)
					continue;

				ck_assert(!libinput_event_tablet_axis_has_changed(tablet_event, a));
				ck_assert_int_eq(libinput_event_tablet_get_axis_value(tablet_event, a), 0);
			}
		}

		libinput_event_destroy(event);
	}
}
END_TEST

int
main(int argc, char **argv)
{
	litest_add("tablet:proximity-in-out", proximity_in_out, LITEST_TABLET, LITEST_ANY);

	return litest_run(argc, argv);
}
