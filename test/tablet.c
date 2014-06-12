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
#include "evdev-tablet.h"
#include "litest.h"

START_TEST(proximity_in_out)
{
	struct litest_device *dev = litest_current_device();
	struct libinput *li = dev->libinput;
	struct libinput_event_tablet *tablet_event;
	struct libinput_event *event;
	bool have_tool_update = false,
	     have_proximity_out = false;
	unsigned int button;
	uint32_t *axis;

	struct axis_replacement axes[] = {
		{ ABS_DISTANCE, 10 },
		{ -1, -1 }
	};

	uint32_t test_axes[] = {
		ABS_X,
		ABS_Y,
		ABS_DISTANCE,
		ABS_PRESSURE,
		ABS_TILT_X,
		ABS_TILT_Y
	};

	litest_drain_events(dev->libinput);

	litest_tablet_proximity_in(dev, 10, 10, axes);
	libinput_dispatch(li);

	while ((event = libinput_get_event(li))) {
		if (libinput_event_get_type(event) ==
		    LIBINPUT_EVENT_TABLET_TOOL_UPDATE) {
			struct libinput_tool * tool;

			have_tool_update++;
			tablet_event = libinput_event_get_tablet_event(event);
			tool = libinput_event_tablet_get_tool(tablet_event);
			ck_assert_int_eq(libinput_tool_get_type(tool),
					 LIBINPUT_TOOL_PEN);
		}
		libinput_event_destroy(event);
	}
	ck_assert(have_tool_update);

	litest_tablet_proximity_out(dev);
	libinput_dispatch(li);

	while ((event = libinput_get_event(li))) {
		if (libinput_event_get_type(event) ==
		    LIBINPUT_EVENT_TABLET_PROXIMITY_OUT)
			have_proximity_out = true;

		libinput_event_destroy(event);
	}
	ck_assert(have_proximity_out);

	/* Test that proximity out events send button releases for any currently
	 * pressed stylus buttons
	 */
	for (button = BTN_TOUCH; button <= BTN_STYLUS2; button++) {
		bool button_released = false;

		litest_tablet_proximity_in(dev, 10, 10, axes);
		litest_event(dev, EV_KEY, button, 1);
		litest_event(dev, EV_SYN, SYN_REPORT, 0);
		litest_tablet_proximity_out(dev);

		libinput_dispatch(li);

		while ((event = libinput_get_event(li))) {
			tablet_event = libinput_event_get_tablet_event(event);

			if (libinput_event_get_type(event) ==
			    LIBINPUT_EVENT_TABLET_BUTTON) {
				unsigned int event_button;
				enum libinput_button_state state;

				event_button = libinput_event_tablet_get_button(
				    tablet_event);
				state = libinput_event_tablet_get_button_state(
				    tablet_event);

				if (event_button == button &&
				    state == LIBINPUT_BUTTON_STATE_RELEASED)
					button_released = true;
			}

			libinput_event_destroy(event);
		}

		ck_assert(button_released);
	}

	/* Make sure every axis gets cleared during a proximity out event */
	ARRAY_FOR_EACH(test_axes, axis) {
		uint32_t *abs_code;
		enum libinput_tablet_axis li_axis;
		enum libinput_event_type type;
		int a;

		litest_tablet_proximity_in(dev, 10, 10, axes);
		ARRAY_FOR_EACH(test_axes, abs_code) {
			litest_event(dev, EV_ABS, *abs_code, 10);
		}
		litest_tablet_proximity_out(dev);

		/* We need to trigger at least one axis event, otherwise we
		 * can't check the value of the other axes
		 */
		litest_event(dev, EV_ABS, *axis, 10);
		litest_event(dev, EV_SYN, SYN_REPORT, 0);

		libinput_dispatch(li);

		/* Skip to after the proximity out event */
		do {
			event = libinput_get_event(li);
			type = libinput_event_get_type(event);

			libinput_event_destroy(event);
		} while (type != LIBINPUT_EVENT_TABLET_PROXIMITY_OUT);

		/* Check to make sure all of the other axes were cleared */
		li_axis = evcode_to_axis(*axis);
		while ((event = libinput_get_event(li))) {
			if (libinput_event_get_type(event) !=
			    LIBINPUT_EVENT_TABLET_AXIS) {
				libinput_event_destroy(event);
				continue;
			}

			tablet_event = libinput_event_get_tablet_event(event);

			for (a = 0; a < LIBINPUT_TABLET_AXIS_CNT; a++) {
				int has_changed;
				double value;

				has_changed = libinput_event_tablet_axis_has_changed(
				    tablet_event, a);
				value = libinput_event_tablet_get_axis_value(
				    tablet_event, a);

				if (a == li_axis) {
					ck_assert(has_changed);
					litest_assert_double_ne(value, 0);
				} else {
					ck_assert(!has_changed);
					litest_assert_double_eq(value, 0);
				}
			}

			libinput_event_destroy(event);
		}
	}

	/* Proximity out must not emit axis events */
	litest_tablet_proximity_out(dev);
	libinput_dispatch(li);

	while ((event = libinput_get_event(li))) {
		tablet_event = libinput_event_get_tablet_event(event);
		enum libinput_event_type type = libinput_event_get_type(event);

		ck_assert(type != LIBINPUT_EVENT_TABLET_AXIS);

		libinput_event_destroy(event);
	}
}
END_TEST

START_TEST(motion)
{
	struct litest_device *dev = litest_current_device();
	struct libinput *li = dev->libinput;
	struct libinput_event_tablet *tablet_event;
	struct libinput_event *event;
	int have_motion = 0,
	    axis_has_changed = 0;
	enum libinput_event_type type;
	struct axis_replacement axes[] = {
		{ ABS_DISTANCE, 10 },
		{ -1, -1 }
	};

	litest_drain_events(dev->libinput);

	litest_tablet_proximity_in(dev, 10, 10, axes);
	libinput_dispatch(li);

	while ((event = libinput_get_event(li))) {
		tablet_event = libinput_event_get_tablet_event(event);
		type = libinput_event_get_type(event);
		axis_has_changed = libinput_event_tablet_axis_has_changed(
		    tablet_event, LIBINPUT_TABLET_AXIS_X);

		if (type == LIBINPUT_EVENT_TABLET_AXIS && axis_has_changed)
			have_motion++;

		libinput_event_destroy(event);
	}
	ck_assert_int_eq(have_motion, 1);

	litest_tablet_motion(dev, 20, 10, axes);
	libinput_dispatch(li);

	while ((event = libinput_get_event(li))) {
		tablet_event = libinput_event_get_tablet_event(event);
		type = libinput_event_get_type(event);
		axis_has_changed = libinput_event_tablet_axis_has_changed(
		    tablet_event, LIBINPUT_TABLET_AXIS_X);

		if (type == LIBINPUT_EVENT_TABLET_AXIS && axis_has_changed)
			have_motion++;

		libinput_event_destroy(event);
	}
	ck_assert_int_eq(have_motion, 2);
}
END_TEST

START_TEST(bad_distance_events)
{
	struct litest_device *dev = litest_current_device();
	struct libinput *li = dev->libinput;
	struct libinput_event_tablet *tablet_event;
	struct libinput_event *event;
	bool bad_distance_event_received = false,
	     axis_has_changed;
	enum libinput_event_type type;

	litest_drain_events(dev->libinput);

	litest_tablet_proximity_out(dev);
	litest_tablet_bad_distance_events(dev);

	/* We shouldn't be able to see any of the bad distance events that got
	 * sent
	 */
	while ((event = libinput_get_event(li))) {
		tablet_event = libinput_event_get_tablet_event(event);
		type = libinput_event_get_type(event);
		axis_has_changed = libinput_event_tablet_axis_has_changed(
		    tablet_event, LIBINPUT_TABLET_AXIS_DISTANCE);

		if (type == LIBINPUT_EVENT_TABLET_AXIS && axis_has_changed)
			bad_distance_event_received = true;

		libinput_event_destroy(event);
	}
	ck_assert(!bad_distance_event_received);
}
END_TEST

START_TEST(normalization)
{
	struct litest_device *dev = litest_current_device();
	struct libinput *li = dev->libinput;
	struct libinput_event_tablet *tablet_event;
	struct libinput_event *event;
	double pressure,
	       tilt_vertical,
	       tilt_horizontal;
	struct axis_replacement axes[] = {
		{ -1, -1 }
	};

	litest_drain_events(dev->libinput);

	litest_tablet_normalization_min(dev, 10, 10, axes);
	libinput_dispatch(li);

	while ((event = libinput_get_event(li))) {
		if (libinput_event_get_type(event) == LIBINPUT_EVENT_TABLET_AXIS) {
			tablet_event = libinput_event_get_tablet_event(event);
			pressure = libinput_event_tablet_get_axis_value(
			    tablet_event, LIBINPUT_TABLET_AXIS_PRESSURE);

			litest_assert_double_eq(pressure, 0);

			if (libinput_event_tablet_axis_has_changed(
				tablet_event,
				LIBINPUT_TABLET_AXIS_TILT_VERTICAL)) {
				tilt_vertical =
					libinput_event_tablet_get_axis_value(
					    tablet_event,
					    LIBINPUT_TABLET_AXIS_TILT_VERTICAL);

				litest_assert_double_eq(tilt_vertical, -1);
			}
			if (libinput_event_tablet_axis_has_changed(
				tablet_event,
				LIBINPUT_TABLET_AXIS_TILT_HORIZONTAL)) {
				tilt_horizontal =
					libinput_event_tablet_get_axis_value(
					    tablet_event,
					    LIBINPUT_TABLET_AXIS_TILT_HORIZONTAL);

				litest_assert_double_eq(tilt_horizontal, -1);
			}
		}
	}

	litest_tablet_normalization_max(dev, 10, 10, axes);
	libinput_dispatch(li);

	while ((event = libinput_get_event(li))) {
		if (libinput_event_get_type(event) == LIBINPUT_EVENT_TABLET_AXIS) {
			tablet_event = libinput_event_get_tablet_event(event);
			pressure = libinput_event_tablet_get_axis_value(
			    tablet_event, LIBINPUT_TABLET_AXIS_PRESSURE);

			litest_assert_double_eq(pressure, 1);

			if (libinput_event_tablet_axis_has_changed(
				tablet_event,
				LIBINPUT_TABLET_AXIS_TILT_VERTICAL)) {
				tilt_vertical =
					libinput_event_tablet_get_axis_value(
					    tablet_event,
					    LIBINPUT_TABLET_AXIS_TILT_VERTICAL);

				litest_assert_double_eq(tilt_vertical, 1);
			}
			if (libinput_event_tablet_axis_has_changed(
				tablet_event,
				LIBINPUT_TABLET_AXIS_TILT_HORIZONTAL)) {
				tilt_horizontal =
					libinput_event_tablet_get_axis_value(
					    tablet_event,
					    LIBINPUT_TABLET_AXIS_TILT_HORIZONTAL);

				litest_assert_double_eq(tilt_horizontal, 1);
			}
		}
	}
}
END_TEST

int
main(int argc, char **argv)
{
	litest_add("tablet:proximity-in-out", proximity_in_out, LITEST_TABLET, LITEST_ANY);
	litest_add("tablet:motion", motion, LITEST_TABLET, LITEST_ANY);
	litest_add("tablet:bad-distance-events", bad_distance_events, LITEST_TABLET | LITEST_DISTANCE, LITEST_ANY);
	litest_add("tablet:normalization", normalization, LITEST_TABLET, LITEST_ANY);

	return litest_run(argc, argv);
}
