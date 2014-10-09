/*
 * Copyright © 2014 Red Hat, Inc.
 * Copyright © 2014 Stephen Chandler "Lyude" Paul
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

	struct axis_replacement axes[] = {
		{ ABS_DISTANCE, 10 },
		{ -1, -1 }
	};

	litest_drain_events(dev->libinput);

	litest_tablet_proximity_in(dev, 10, 10, axes);
	libinput_dispatch(li);

	while ((event = libinput_get_event(li))) {
		if (libinput_event_get_type(event) ==
		    LIBINPUT_EVENT_TABLET_PROXIMITY_IN) {
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

	/* Proximity out must not emit axis events */
	litest_tablet_proximity_out(dev);
	libinput_dispatch(li);

	while ((event = libinput_get_event(li))) {
		enum libinput_event_type type = libinput_event_get_type(event);

		ck_assert(type != LIBINPUT_EVENT_TABLET_AXIS);

		libinput_event_destroy(event);
	}
}
END_TEST

START_TEST(proximity_out_clear_buttons)
{
	struct litest_device *dev = litest_current_device();
	struct libinput *li = dev->libinput;
	struct libinput_event_tablet *tablet_event;
	struct libinput_event *event;
	uint32_t button;

	struct axis_replacement axes[] = {
		{ ABS_DISTANCE, 10 },
		{ -1, -1 }
	};

	litest_drain_events(dev->libinput);

	/* Test that proximity out events send button releases for any currently
	 * pressed stylus buttons
	 */
	for (button = BTN_TOUCH; button <= BTN_STYLUS2; button++) {
		bool button_released = false;
		uint32_t event_button;
		enum libinput_button_state state;

		litest_tablet_proximity_in(dev, 10, 10, axes);
		litest_event(dev, EV_KEY, button, 1);
		litest_event(dev, EV_SYN, SYN_REPORT, 0);
		litest_tablet_proximity_out(dev);

		libinput_dispatch(li);

		while ((event = libinput_get_event(li))) {
			tablet_event = libinput_event_get_tablet_event(event);

			if (libinput_event_get_type(event) ==
			    LIBINPUT_EVENT_TABLET_BUTTON) {

				event_button = libinput_event_tablet_get_button(tablet_event);
				state = libinput_event_tablet_get_button_state(tablet_event);

				if (event_button == button &&
				    state == LIBINPUT_BUTTON_STATE_RELEASED)
					button_released = true;
			}

			libinput_event_destroy(event);
		}

		ck_assert_msg(button_released,
			      "Button %d was not released.",
			      event_button);
	}
}
END_TEST

START_TEST(motion)
{
	struct litest_device *dev = litest_current_device();
	struct libinput *li = dev->libinput;
	struct libinput_event_tablet *tablet_event;
	struct libinput_event *event;
	int test_x, test_y;
	double last_reported_x, last_reported_y;
	enum libinput_event_type type;
	struct axis_replacement axes[] = {
		{ ABS_DISTANCE, 10 },
		{ -1, -1 }
	};

	litest_drain_events(li);

	litest_tablet_proximity_in(dev, 5, 100, axes);
	libinput_dispatch(li);

	while ((event = libinput_get_event(li))) {
		tablet_event = libinput_event_get_tablet_event(event);
		type = libinput_event_get_type(event);

		if (type == LIBINPUT_EVENT_TABLET_AXIS) {
			bool x_changed, y_changed;
			double reported_x, reported_y;

			x_changed = libinput_event_tablet_axis_has_changed(
			    tablet_event, LIBINPUT_TABLET_AXIS_X);
			y_changed = libinput_event_tablet_axis_has_changed(
			    tablet_event, LIBINPUT_TABLET_AXIS_Y);

			ck_assert(x_changed);
			ck_assert(y_changed);

			reported_x = libinput_event_tablet_get_axis_value(
			    tablet_event, LIBINPUT_TABLET_AXIS_X);
			reported_y = libinput_event_tablet_get_axis_value(
			    tablet_event, LIBINPUT_TABLET_AXIS_Y);

			litest_assert_double_lt(reported_x, reported_y);

			last_reported_x = reported_x;
			last_reported_y = reported_y;
		}

		libinput_event_destroy(event);
	}

	for (test_x = 10, test_y = 90;
	     test_x <= 100;
	     test_x += 10, test_y -= 10) {
		bool x_changed, y_changed;
		double reported_x, reported_y;

		litest_tablet_proximity_in(dev, test_x, test_y, axes);
		libinput_dispatch(li);

		while ((event = libinput_get_event(li))) {
			tablet_event = libinput_event_get_tablet_event(event);
			type = libinput_event_get_type(event);

			if (type == LIBINPUT_EVENT_TABLET_AXIS) {
				x_changed = libinput_event_tablet_axis_has_changed(
				    tablet_event, LIBINPUT_TABLET_AXIS_X);
				y_changed = libinput_event_tablet_axis_has_changed(
				    tablet_event, LIBINPUT_TABLET_AXIS_Y);

				ck_assert(x_changed);
				ck_assert(y_changed);

				reported_x = libinput_event_tablet_get_axis_value(
				    tablet_event, LIBINPUT_TABLET_AXIS_X);
				reported_y = libinput_event_tablet_get_axis_value(
				    tablet_event, LIBINPUT_TABLET_AXIS_Y);

				litest_assert_double_gt(reported_x,
							last_reported_x);
				litest_assert_double_lt(reported_y,
							last_reported_y);

				last_reported_x = reported_x;
				last_reported_y = reported_y;
			}

			libinput_event_destroy(event);
		}
	}
}
END_TEST

START_TEST(motion_event_state)
{
	struct litest_device *dev = litest_current_device();
	struct libinput *li = dev->libinput;
	struct libinput_event *event;
	struct libinput_event_tablet *tablet_event;
	int test_x, test_y;
	double last_x, last_y;

	struct axis_replacement axes[] = {
		{ ABS_DISTANCE, 10 },
		{ -1, -1 }
	};

	litest_drain_events(li);
	litest_tablet_proximity_in(dev, 5, 100, axes);
	litest_drain_events(li);

	/* couple of events that go left/bottom to right/top */
	for (test_x = 0, test_y = 100; test_x < 100; test_x += 10, test_y -= 10)
		litest_tablet_proximity_in(dev, test_x, test_y, axes);

	libinput_dispatch(li);

	while ((event = libinput_get_event(li))) {
		if (libinput_event_get_type(event) == LIBINPUT_EVENT_TABLET_AXIS)
			break;
		libinput_event_destroy(event);
	}

	/* pop the first event off */
	ck_assert_notnull(event);
	tablet_event = libinput_event_get_tablet_event(event);
	ck_assert_notnull(tablet_event);

	last_x = libinput_event_tablet_get_axis_value(tablet_event,
						      LIBINPUT_TABLET_AXIS_X);
	last_y = libinput_event_tablet_get_axis_value(tablet_event,
						      LIBINPUT_TABLET_AXIS_Y);

	/* mark with a button event, then go back to bottom/left */
	litest_event(dev, EV_KEY, BTN_STYLUS, 1);
	litest_event(dev, EV_SYN, SYN_REPORT, 0);

	for (test_x = 100, test_y = 0; test_x > 0; test_x -= 10, test_y += 10)
		litest_tablet_proximity_in(dev, test_x, test_y, axes);

	libinput_event_destroy(event);
	libinput_dispatch(li);
	ck_assert_int_eq(libinput_next_event_type(li),
			 LIBINPUT_EVENT_TABLET_AXIS);

	/* we expect all events up to the button event to go from
	   bottom/left to top/right */
	while ((event = libinput_get_event(li))) {
		double x, y;

		if (libinput_event_get_type(event) != LIBINPUT_EVENT_TABLET_AXIS)
			break;

		tablet_event = libinput_event_get_tablet_event(event);
		ck_assert_notnull(tablet_event);

		x = libinput_event_tablet_get_axis_value(tablet_event,
							 LIBINPUT_TABLET_AXIS_X);
		y = libinput_event_tablet_get_axis_value(tablet_event,
							 LIBINPUT_TABLET_AXIS_Y);

		ck_assert(x > last_x);
		ck_assert(y < last_y);

		last_x = x;
		last_y = y;
		libinput_event_destroy(event);
	}

	ck_assert_int_eq(libinput_event_get_type(event),
			 LIBINPUT_EVENT_TABLET_BUTTON);
	libinput_event_destroy(event);
}
END_TEST

START_TEST(bad_distance_events)
{
	struct litest_device *dev = litest_current_device();
	struct libinput *li = dev->libinput;
	const struct input_absinfo *absinfo;
	struct axis_replacement axes[] = {
		{ -1, -1 },
	};

	litest_tablet_proximity_in(dev, 10, 10, axes);
	litest_tablet_proximity_out(dev);
	litest_drain_events(dev->libinput);

	absinfo = libevdev_get_abs_info(dev->evdev, ABS_DISTANCE);
	ck_assert(absinfo != NULL);

	litest_event(dev, EV_ABS, ABS_DISTANCE, absinfo->maximum);
	litest_event(dev, EV_SYN, SYN_REPORT, 0);
	litest_event(dev, EV_ABS, ABS_DISTANCE, absinfo->minimum);
	litest_event(dev, EV_SYN, SYN_REPORT, 0);

	litest_assert_empty_queue(li);
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
	       tilt_horizontal,
	       twist;
	const struct input_absinfo *pressure_absinfo,
                                   *tilt_vertical_absinfo,
                                   *tilt_horizontal_absinfo,
                                   *twist_absinfo;

	litest_drain_events(dev->libinput);

	pressure_absinfo = libevdev_get_abs_info(dev->evdev, ABS_PRESSURE);
	tilt_vertical_absinfo = libevdev_get_abs_info(dev->evdev, ABS_TILT_X);
	tilt_horizontal_absinfo = libevdev_get_abs_info(dev->evdev, ABS_TILT_Y);
	twist_absinfo = libevdev_get_abs_info(dev->evdev, ABS_Z);

	/* Test minimum */
	if (pressure_absinfo != NULL)
		litest_event(dev,
			     EV_ABS,
			     ABS_PRESSURE,
			     pressure_absinfo->minimum);

	if (tilt_vertical_absinfo != NULL)
		litest_event(dev,
			     EV_ABS,
			     ABS_TILT_X,
			     tilt_vertical_absinfo->minimum);

	if (tilt_horizontal_absinfo != NULL)
		litest_event(dev,
			     EV_ABS,
			     ABS_TILT_Y,
			     tilt_horizontal_absinfo->minimum);

	if (twist_absinfo != NULL)
		litest_event(dev,
			     EV_ABS,
			     ABS_Z,
			     twist_absinfo->minimum);

	litest_event(dev, EV_SYN, SYN_REPORT, 0);

	libinput_dispatch(li);

	while ((event = libinput_get_event(li))) {
		if (libinput_event_get_type(event) == LIBINPUT_EVENT_TABLET_AXIS) {
			tablet_event = libinput_event_get_tablet_event(event);

			if (libinput_event_tablet_axis_has_changed(
				tablet_event,
				LIBINPUT_TABLET_AXIS_PRESSURE)) {
				pressure = libinput_event_tablet_get_axis_value(
				    tablet_event, LIBINPUT_TABLET_AXIS_PRESSURE);

				litest_assert_double_eq(pressure, 0);
			}

			if (libinput_event_tablet_axis_has_changed(
				tablet_event,
				LIBINPUT_TABLET_AXIS_TILT_X)) {
				tilt_vertical =
					libinput_event_tablet_get_axis_value(
					    tablet_event,
					    LIBINPUT_TABLET_AXIS_TILT_X);

				litest_assert_double_eq(tilt_vertical, -1);
			}

			if (libinput_event_tablet_axis_has_changed(
				tablet_event,
				LIBINPUT_TABLET_AXIS_TILT_Y)) {
				tilt_horizontal =
					libinput_event_tablet_get_axis_value(
					    tablet_event,
					    LIBINPUT_TABLET_AXIS_TILT_Y);

				litest_assert_double_eq(tilt_horizontal, -1);
			}

			if (libinput_event_tablet_axis_has_changed(
				tablet_event,
				LIBINPUT_TABLET_AXIS_TWIST)) {
				twist =
					libinput_event_tablet_get_axis_value(
					    tablet_event,
					    LIBINPUT_TABLET_AXIS_TWIST);

				litest_assert_double_eq(twist, -1);
			}
		}

		libinput_event_destroy(event);
	}

	/* Test maximum */
	if (pressure_absinfo != NULL)
		litest_event(dev,
			     EV_ABS,
			     ABS_PRESSURE,
			     pressure_absinfo->maximum);

	if (tilt_vertical_absinfo != NULL)
		litest_event(dev,
			     EV_ABS,
			     ABS_TILT_X,
			     tilt_vertical_absinfo->maximum + 1);

	if (tilt_horizontal_absinfo != NULL)
		litest_event(dev,
			     EV_ABS,
			     ABS_TILT_Y,
			     tilt_horizontal_absinfo->maximum + 1);

	if (twist_absinfo != NULL)
		litest_event(dev,
			     EV_ABS,
			     ABS_Z,
			     twist_absinfo->maximum + 1);

	litest_event(dev, EV_SYN, SYN_REPORT, 0);

	libinput_dispatch(li);

	while ((event = libinput_get_event(li))) {
		if (libinput_event_get_type(event) == LIBINPUT_EVENT_TABLET_AXIS) {
			tablet_event = libinput_event_get_tablet_event(event);

			if (libinput_event_tablet_axis_has_changed(
				tablet_event,
				LIBINPUT_TABLET_AXIS_PRESSURE)) {
				pressure = libinput_event_tablet_get_axis_value(
				    tablet_event, LIBINPUT_TABLET_AXIS_PRESSURE);

				litest_assert_double_eq(pressure, 1);
			}

			if (libinput_event_tablet_axis_has_changed(
				tablet_event,
				LIBINPUT_TABLET_AXIS_TILT_X)) {
				tilt_vertical =
					libinput_event_tablet_get_axis_value(
					    tablet_event,
					    LIBINPUT_TABLET_AXIS_TILT_X);

				litest_assert_double_eq(tilt_vertical, 1);
			}

			if (libinput_event_tablet_axis_has_changed(
				tablet_event,
				LIBINPUT_TABLET_AXIS_TILT_Y)) {
				tilt_horizontal =
					libinput_event_tablet_get_axis_value(
					    tablet_event,
					    LIBINPUT_TABLET_AXIS_TILT_Y);

				litest_assert_double_eq(tilt_horizontal, 1);
			}

			if (libinput_event_tablet_axis_has_changed(
				tablet_event,
				LIBINPUT_TABLET_AXIS_TWIST)) {
				twist =
					libinput_event_tablet_get_axis_value(
					    tablet_event,
					    LIBINPUT_TABLET_AXIS_TWIST);

				litest_assert_double_eq(twist, 1);
			}
		}

		libinput_event_destroy(event);
	}

}
END_TEST

START_TEST(tool_serial)
{
	struct litest_device *dev = litest_current_device();
	struct libinput *li = dev->libinput;
	struct libinput_event_tablet *tablet_event;
	struct libinput_event *event;
	struct libinput_tool *tool;

	litest_drain_events(li);

	litest_event(dev, EV_KEY, BTN_TOOL_PEN, 1);
	litest_event(dev, EV_MSC, MSC_SERIAL, 1000);
	litest_event(dev, EV_SYN, SYN_REPORT, 0);

	libinput_dispatch(li);
	while ((event = libinput_get_event(li))) {
		if (libinput_event_get_type(event) ==
		    LIBINPUT_EVENT_TABLET_PROXIMITY_IN) {
			tablet_event = libinput_event_get_tablet_event(event);
			tool = libinput_event_tablet_get_tool(tablet_event);

			ck_assert_uint_eq(libinput_tool_get_serial(tool), 1000);
		}

		libinput_event_destroy(event);
	}
}
END_TEST

START_TEST(serial_changes_tool)
{
	struct litest_device *dev = litest_current_device();
	struct libinput *li = dev->libinput;
	struct libinput_event_tablet *tablet_event;
	struct libinput_event *event;
	struct libinput_tool *tool;
	bool tool_updated = false;

	litest_drain_events(li);

	litest_event(dev, EV_KEY, BTN_TOOL_PEN, 1);
	litest_event(dev, EV_MSC, MSC_SERIAL, 1000);
	litest_event(dev, EV_SYN, SYN_REPORT, 0);
	litest_event(dev, EV_KEY, BTN_TOOL_PEN, 0);
	litest_event(dev, EV_SYN, SYN_REPORT, 0);
	litest_drain_events(li);

	litest_event(dev, EV_KEY, BTN_TOOL_PEN, 1);
	litest_event(dev, EV_MSC, MSC_SERIAL, 2000);
	litest_event(dev, EV_SYN, SYN_REPORT, 0);

	libinput_dispatch(li);
	while ((event = libinput_get_event(li))) {
		if (libinput_event_get_type(event) ==
		    LIBINPUT_EVENT_TABLET_PROXIMITY_IN) {
			tablet_event = libinput_event_get_tablet_event(event);
			tool = libinput_event_tablet_get_tool(tablet_event);

			ck_assert_uint_eq(libinput_tool_get_serial(tool), 2000);
			tool_updated = true;
		}

		libinput_event_destroy(event);
	}
	ck_assert(tool_updated);
}
END_TEST

START_TEST(invalid_serials)
{
	struct litest_device *dev = litest_current_device();
	struct libinput *li = dev->libinput;
	struct libinput_event *event;
	struct libinput_event_tablet *tablet_event;
	struct libinput_tool *tool;

	litest_drain_events(li);

	litest_event(dev, EV_KEY, BTN_TOOL_PEN, 1);
	litest_event(dev, EV_MSC, MSC_SERIAL, 1000);
	litest_event(dev, EV_SYN, SYN_REPORT, 0);
	litest_event(dev, EV_KEY, BTN_TOOL_PEN, 0);
	litest_event(dev, EV_SYN, SYN_REPORT, 0);
	litest_drain_events(li);

	litest_event(dev, EV_KEY, BTN_TOOL_PEN, 1);
	litest_event(dev, EV_MSC, MSC_SERIAL, -1);
	litest_event(dev, EV_SYN, SYN_REPORT, 0);

	libinput_dispatch(li);
	while ((event = libinput_get_event(li))) {
		if (libinput_event_get_type(event) ==
		    LIBINPUT_EVENT_TABLET_PROXIMITY_IN) {
			tablet_event = libinput_event_get_tablet_event(event);
			tool = libinput_event_tablet_get_tool(tablet_event);

			ck_assert_uint_eq(libinput_tool_get_serial(tool), 1000);
		}

		libinput_event_destroy(event);
	}
}
END_TEST

START_TEST(tool_ref)
{
	struct litest_device *dev = litest_current_device();
	struct libinput *li = dev->libinput;
	struct libinput_event_tablet *tablet_event;
	struct libinput_event *event;
	struct libinput_tool *tool;

	litest_drain_events(li);

	litest_event(dev, EV_KEY, BTN_TOOL_PEN, 1);
	litest_event(dev, EV_MSC, MSC_SERIAL, 1000);
	litest_event(dev, EV_SYN, SYN_REPORT, 0);

	libinput_dispatch(li);
	while ((event = libinput_get_event(li))) {
		if (libinput_event_get_type(event) ==
		    LIBINPUT_EVENT_TABLET_PROXIMITY_IN) {
			break;
		}
		libinput_event_destroy(event);
	}

	tablet_event = libinput_event_get_tablet_event(event);
	tool = libinput_event_tablet_get_tool(tablet_event);

	ck_assert_notnull(tool);
	ck_assert(tool == libinput_tool_ref(tool));
	ck_assert(tool == libinput_tool_unref(tool));
	ck_assert(libinput_tool_unref(tool) == NULL);

	libinput_event_destroy(event);
}
END_TEST


START_TEST(pad_buttons_ignored)
{
	struct litest_device *dev = litest_current_device();
	struct libinput *li = dev->libinput;
	struct libinput_event *event;
	struct axis_replacement axes[] = {
		{ ABS_DISTANCE, 10 },
		{ -1, -1 }
	};
	int button;

	litest_drain_events(li);

	for (button = BTN_0; button < BTN_MOUSE; button++) {
		litest_event(dev, EV_KEY, button, 1);
		litest_event(dev, EV_SYN, SYN_REPORT, 0);
		litest_event(dev, EV_KEY, button, 0);
		litest_event(dev, EV_SYN, SYN_REPORT, 0);
		libinput_dispatch(li);
	}

	while ((event = libinput_get_event(li))) {
		ck_assert_int_ne(libinput_event_get_type(event),
				 LIBINPUT_EVENT_TABLET_BUTTON);
		libinput_event_destroy(event);
		libinput_dispatch(li);
	}

	/* same thing while in prox */
	litest_tablet_proximity_in(dev, 10, 10, axes);
	for (button = BTN_0; button < BTN_MOUSE; button++) {
		litest_event(dev, EV_KEY, button, 1);
		litest_event(dev, EV_SYN, SYN_REPORT, 0);
		litest_event(dev, EV_KEY, button, 0);
		litest_event(dev, EV_SYN, SYN_REPORT, 0);
		libinput_dispatch(li);
	}
	litest_tablet_proximity_out(dev);

	libinput_dispatch(li);
	while ((event = libinput_get_event(li))) {
		ck_assert_int_ne(libinput_event_get_type(event),
				 LIBINPUT_EVENT_TABLET_BUTTON);
		libinput_event_destroy(event);
		libinput_dispatch(li);
	}
}
END_TEST

START_TEST(tools_with_serials)
{
	struct libinput *li = litest_create_context();
	struct litest_device *dev[2];
	struct libinput_tool *tool[2] = {0};
	struct libinput_event *event;
	int i;

	for (i = 0; i < 2; i++) {
		dev[i] = litest_add_device_with_overrides(li,
							  LITEST_WACOM_INTUOS,
							  NULL,
							  NULL,
							  NULL,
							  NULL);
		/* WARNING: this test fails if UI_GET_SYSNAME isn't
		 * available or isn't used by libevdev (1.3, commit 2ff45c73).
		 * Put a sleep(1) here and that usually fixes it.
		 */

		litest_event(dev[i], EV_KEY, BTN_TOOL_PEN, 1);
		litest_event(dev[i], EV_MSC, MSC_SERIAL, 100);
		litest_event(dev[i], EV_SYN, SYN_REPORT, 0);

		libinput_dispatch(li);
		while ((event = libinput_get_event(li))) {
			if (libinput_event_get_type(event) ==
			    LIBINPUT_EVENT_TABLET_PROXIMITY_IN) {
				struct libinput_event_tablet *t =
					libinput_event_get_tablet_event(event);

				tool[i] = libinput_event_tablet_get_tool(t);
			}

			libinput_event_destroy(event);
		}
	}

	/* We should get the same object for both devices */
	ck_assert_notnull(tool[0]);
	ck_assert_notnull(tool[1]);
	ck_assert_ptr_eq(tool[0], tool[1]);

	litest_delete_device(dev[0]);
	litest_delete_device(dev[1]);
	libinput_unref(li);
}
END_TEST

START_TEST(tools_without_serials)
{
	struct libinput *li = litest_create_context();
	struct litest_device *dev[2];
	struct libinput_tool *tool[2] = {0};
	struct libinput_event *event;
	int i;

	for (i = 0; i < 2; i++) {
		dev[i] = litest_add_device_with_overrides(li,
							  LITEST_WACOM_ISDV4,
							  NULL,
							  NULL,
							  NULL,
							  NULL);

		/* WARNING: this test fails if UI_GET_SYSNAME isn't
		 * available or isn't used by libevdev (1.3, commit 2ff45c73).
		 * Put a sleep(1) here and that usually fixes it.
		 */

		litest_event(dev[i], EV_KEY, BTN_TOOL_PEN, 1);
		litest_event(dev[i], EV_SYN, SYN_REPORT, 0);

		libinput_dispatch(li);
		while ((event = libinput_get_event(li))) {
			if (libinput_event_get_type(event) ==
			    LIBINPUT_EVENT_TABLET_PROXIMITY_IN) {
				struct libinput_event_tablet *t =
					libinput_event_get_tablet_event(event);

				tool[i] = libinput_event_tablet_get_tool(t);
			}

			libinput_event_destroy(event);
		}
	}

	/* We should get different tool objects for each device */
	ck_assert_notnull(tool[0]);
	ck_assert_notnull(tool[1]);
	ck_assert_ptr_ne(tool[0], tool[1]);

	litest_delete_device(dev[0]);
	litest_delete_device(dev[1]);
	libinput_unref(li);
}
END_TEST

START_TEST(tool_capabilities)
{
	struct libinput *li = litest_create_context();
	struct litest_device *intuos;
	struct litest_device *bamboo;
	struct libinput_event *event;

	/* The axis capabilities of a tool can differ depending on the type of
	 * tablet the tool is being used with */
	bamboo = litest_create_device_with_overrides(LITEST_WACOM_BAMBOO,
						     NULL,
						     NULL,
						     NULL,
						     NULL);
	intuos = litest_create_device_with_overrides(LITEST_WACOM_INTUOS,
						     NULL,
						     NULL,
						     NULL,
						     NULL);

	litest_event(bamboo, EV_KEY, BTN_TOOL_PEN, 1);
	litest_event(bamboo, EV_SYN, SYN_REPORT, 0);

	libinput_dispatch(li);
	while ((event = libinput_get_event(li))) {
		if (libinput_event_get_type(event) ==
		    LIBINPUT_EVENT_TABLET_PROXIMITY_IN) {
			struct libinput_event_tablet *t =
				libinput_event_get_tablet_event(event);
			struct libinput_tool *tool =
				libinput_event_tablet_get_tool(t);

			ck_assert(libinput_tool_has_axis(tool,
							 LIBINPUT_TABLET_AXIS_PRESSURE));
			ck_assert(libinput_tool_has_axis(tool,
							 LIBINPUT_TABLET_AXIS_DISTANCE));
			ck_assert(!libinput_tool_has_axis(tool,
							  LIBINPUT_TABLET_AXIS_TILT_X));
			ck_assert(!libinput_tool_has_axis(tool,
							  LIBINPUT_TABLET_AXIS_TILT_Y));
		}

		libinput_event_destroy(event);
	}

	litest_event(intuos, EV_KEY, BTN_TOOL_PEN, 1);
	litest_event(intuos, EV_SYN, SYN_REPORT, 0);

	while ((event = libinput_get_event(li))) {
		if (libinput_event_get_type(event) ==
		    LIBINPUT_EVENT_TABLET_PROXIMITY_IN) {
			struct libinput_event_tablet *t =
				libinput_event_get_tablet_event(event);
			struct libinput_tool *tool =
				libinput_event_tablet_get_tool(t);

			ck_assert(libinput_tool_has_axis(tool,
							 LIBINPUT_TABLET_AXIS_PRESSURE));
			ck_assert(libinput_tool_has_axis(tool,
							 LIBINPUT_TABLET_AXIS_DISTANCE));
			ck_assert(libinput_tool_has_axis(tool,
							 LIBINPUT_TABLET_AXIS_TILT_X));
			ck_assert(libinput_tool_has_axis(tool,
							 LIBINPUT_TABLET_AXIS_TILT_Y));
		}

		libinput_event_destroy(event);
	}

	litest_delete_device(bamboo);
	litest_delete_device(intuos);
	libinput_unref(li);
}
END_TEST

int
main(int argc, char **argv)
{
	litest_add("tablet:tool", tool_ref, LITEST_TABLET | LITEST_TOOL_SERIAL, LITEST_ANY);
	litest_add_no_device("tablet:tool", tool_capabilities);
	litest_add("tablet:tool_serial", tool_serial, LITEST_TABLET | LITEST_TOOL_SERIAL, LITEST_ANY);
	litest_add("tablet:tool_serial", serial_changes_tool, LITEST_TABLET | LITEST_TOOL_SERIAL, LITEST_ANY);
	litest_add("tablet:tool_serial", invalid_serials, LITEST_TABLET | LITEST_TOOL_SERIAL, LITEST_ANY);
	litest_add_no_device("tablet:tool_serial", tools_with_serials);
	litest_add_no_device("tablet:tool_serial", tools_without_serials);
	litest_add("tablet:proximity", proximity_out_clear_buttons, LITEST_TABLET, LITEST_ANY);
	litest_add("tablet:proximity", proximity_in_out, LITEST_TABLET, LITEST_ANY);
	litest_add("tablet:proximity", bad_distance_events, LITEST_TABLET | LITEST_DISTANCE, LITEST_ANY);
	litest_add("tablet:motion", motion, LITEST_TABLET, LITEST_ANY);
	litest_add("tablet:motion", motion_event_state, LITEST_TABLET, LITEST_ANY);
	litest_add("tablet:normalization", normalization, LITEST_TABLET, LITEST_ANY);
	litest_add("tablet:pad", pad_buttons_ignored, LITEST_TABLET, LITEST_ANY);

	return litest_run(argc, argv);
}
