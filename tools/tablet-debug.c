/*
 * Copyright © 2014 Lyude
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

#include <stdbool.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <poll.h>
#include <signal.h>
#include <sys/signalfd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <termios.h>

#include <linux/input.h>

#include <ncurses.h>
#include <panel.h>

#include <libinput.h>
#include <libudev.h>

#include "tablet-debug-fields.h"

#define bool_to_string(bool_) (bool_ ? "True" : "False")

static struct udev *udev;
static struct libinput *li;
static const char *seat = "seat0";

static int tablet_count = 0;

struct tablet_panel {
	WINDOW * window;
	PANEL * panel;
	struct libinput_device * dev;

	bool stylus_touching;

	double x;
	double y;

	char * tool_str;
	uint32_t tool_serial;

	double tilt_vertical;
	double tilt_horizontal;

	double pressure;
	double distance;

	enum libinput_button_state stylus_button_1;
	enum libinput_button_state stylus_button_2;
};

WINDOW * placeholder_window;
PANEL * placeholder_panel;

static int
open_restricted(const char *path, int flags, void *user_data)
{
	int fd=open(path, flags);
	return fd < 0 ? -errno : fd;
}

static void
close_restricted(int fd, void *user_data)
{
	close(fd);
}

const static struct libinput_interface interface = {
	.open_restricted = open_restricted,
	.close_restricted = close_restricted
};

static int
open_udev()
{
	printf("Setting up udev...\n");

	udev = udev_new();
	if (!udev) {
		fprintf(stderr, "Failed to initialize udev\n");
		return 1;
	}

	li = libinput_udev_create_for_seat(&interface, NULL, udev, seat);
	if (!li) {
		fprintf(stderr, "Failed to initialize context from udev\n");
		return 1;
	}

	return 0;
}

static inline void
update_display()
{
	update_panels();
	doupdate();
}

static void
update_line(struct tablet_panel * panel,
	    int line,
	    const char * format,
	    ...)
{
	va_list args;

	wmove(panel->window, line, 0);

	va_start(args, format);
	vwprintw(panel->window, format, args); 
	va_end(args);

	wclrtoeol(panel->window);
}

static void
paint_placeholder_panel()
{
	wclear(placeholder_window);
	wprintw(placeholder_window,
		"There are no currently detected tablets.\n"
		"Make sure your tablet is plugged in, and that you have the "
		"right permissions.");
}

static void
paint_panel(struct tablet_panel * panel)
{
	wclear(panel->window);

	mvwprintw(panel->window, TABLET_SYSTEM_NAME_ROW, 0,
		  TABLET_SYSTEM_NAME_FIELD, libinput_device_get_sysname(panel->dev));
	mvwprintw(panel->window, TABLET_STYLUS_TOUCHING_ROW, 0,
		  TABLET_STYLUS_TOUCHING_FIELD,
		  bool_to_string(panel->stylus_touching));

	mvwprintw(panel->window, TABLET_TOOL_NAME_ROW, 0,
		  TABLET_TOOL_NAME_FIELD, panel->tool_str);
	mvwprintw(panel->window, TABLET_TOOL_SERIAL_ROW, 0,
		  TABLET_TOOL_SERIAL_FIELD, panel->tool_serial);

	mvwprintw(panel->window, TABLET_X_AND_Y_ROW, 0,
		  TABLET_X_AND_Y_FIELD, panel->x, panel->y);

	mvwprintw(panel->window, TABLET_TILT_VERTICAL_ROW, 0,
		  TABLET_TILT_VERTICAL_FIELD, panel->tilt_vertical);
	mvwprintw(panel->window, TABLET_TILT_HORIZONTAL_ROW, 0,
		  TABLET_TILT_HORIZONTAL_FIELD, panel->tilt_horizontal);

	mvwprintw(panel->window, TABLET_DISTANCE_ROW, 0,
		  TABLET_DISTANCE_FIELD, panel->distance);
	mvwprintw(panel->window, TABLET_PRESSURE_ROW, 0,
		  TABLET_PRESSURE_FIELD, panel->pressure);

	mvwprintw(panel->window, TABLET_STYLUS_BUTTONS_ROW, 0,
		  TABLET_STYLUS_BUTTONS_FIELD,
		  bool_to_string(panel->stylus_button_1),
		  bool_to_string(panel->stylus_button_2));
}

static struct tablet_panel *
new_tablet_panel(struct libinput_device * dev)
{
	struct tablet_panel * panel;

	panel = malloc(sizeof(struct tablet_panel));
	memset(panel, 0, sizeof(struct tablet_panel));
	panel->window = newwin(0, 0, 0, 0);
	panel->panel = new_panel(panel->window);
	panel->dev = dev;
	panel->tool_str = "None";

	set_panel_userptr(panel->panel, panel);

	return panel;
}

static void
handle_new_device(struct libinput_event *ev,
		  struct libinput_device *dev)
{
	struct tablet_panel * panel;

	libinput_device_ref(dev);

	panel = new_tablet_panel(dev);
	bottom_panel(panel->panel);
	paint_panel(panel);

	/* If this is our only tablet, get rid of the placeholder panel */
	if (++tablet_count == 1) {
		hide_panel(placeholder_panel);
		update_display();
	}

	libinput_device_set_user_data(dev, panel);
}

static void
handle_removed_device(struct libinput_event *ev,
		      struct libinput_device *dev)
{
	struct tablet_panel * panel;
	
	panel = libinput_device_get_user_data(dev);
	del_panel(panel->panel);
	delwin(panel->window);
	free(panel);

	if (--tablet_count == 0) {
		show_panel(placeholder_panel);
		update_display();
	}

	libinput_device_unref(dev);
}

static void
handle_tablet_motion(struct libinput_event_tablet *ev,
		      struct libinput_device *dev)
{
	struct tablet_panel * panel;

	panel = libinput_device_get_user_data(dev);
	panel->x = li_fixed_to_double(libinput_event_tablet_get_x(ev));
	panel->y = li_fixed_to_double(libinput_event_tablet_get_y(ev));

	update_line(panel, TABLET_X_AND_Y_ROW, TABLET_X_AND_Y_FIELD, panel->x,
		    panel->y);

	update_display();
}

static void
handle_tool_update(struct libinput_event_tablet *ev,
		   struct libinput_device *dev)
{
	struct tablet_panel * panel;
	struct libinput_tool * tool;
	char * tool_str;
	uint32_t serial;

	panel = libinput_device_get_user_data(dev);
	tool = libinput_event_tablet_get_tool(ev);

	switch (libinput_tool_get_type(tool)) {
	case LIBINPUT_TOOL_NONE:
		tool_str = "None";
		break;
	case LIBINPUT_TOOL_PEN:
		tool_str = "Pen";
		break;
	case LIBINPUT_TOOL_ERASER:
		tool_str = "Eraser";
		break;
	case LIBINPUT_TOOL_BRUSH:
		tool_str = "Brush";
		break;
	case LIBINPUT_TOOL_PENCIL:
		tool_str = "Pencil";
		break;
	case LIBINPUT_TOOL_AIRBRUSH:
		tool_str = "Airbrush";
		break;
	case LIBINPUT_TOOL_FINGER:
		tool_str = "Finger";
		break;
	case LIBINPUT_TOOL_MOUSE:
		tool_str = "Mouse";
		break;
	case LIBINPUT_TOOL_LENS:
		tool_str = "Lens";
		break;
	default:
		tool_str = "???";
		break;
	}
	serial = libinput_tool_get_serial(tool);

	update_line(panel, TABLET_TOOL_NAME_ROW, TABLET_TOOL_NAME_FIELD,
		    tool_str);
	update_line(panel, TABLET_TOOL_SERIAL_ROW, TABLET_TOOL_SERIAL_FIELD,
		    serial);

	panel->tool_str = tool_str;
	panel->tool_serial = serial;
}

static void
handle_axis_update(struct libinput_event_tablet *ev,
		   struct libinput_device *dev)
{
	struct tablet_panel * panel;

	panel = libinput_device_get_user_data(dev);

	int a;
	for (a = 0; a <= LIBINPUT_TABLET_AXIS_MAX; a++) {
		int row;
		const char * field;
		double value;

		if (!libinput_event_tablet_axis_updated(ev, a))
			continue;

		value = li_fixed_to_double(
		    libinput_event_tablet_get_axis_value(ev, a));

		switch (a) {
		case LIBINPUT_TABLET_AXIS_TILT_VERTICAL:
			row = TABLET_TILT_VERTICAL_ROW;
			field = TABLET_TILT_VERTICAL_FIELD;
			panel->tilt_vertical = value;
			break;
		case LIBINPUT_TABLET_AXIS_TILT_HORIZONTAL:
			row = TABLET_TILT_HORIZONTAL_ROW;
			field = TABLET_TILT_HORIZONTAL_FIELD;
			panel->tilt_horizontal = value;
			break;
		case LIBINPUT_TABLET_AXIS_DISTANCE:
			row = TABLET_DISTANCE_ROW;
			field = TABLET_DISTANCE_FIELD;
			panel->distance = value;
			break;
		case LIBINPUT_TABLET_AXIS_PRESSURE:
			row = TABLET_PRESSURE_ROW;
			field = TABLET_PRESSURE_FIELD;
			panel->pressure = value;
			break;
		}
		update_line(panel, row, field, value);
	}
}

static void
handle_button_update(struct libinput_event_tablet* ev,
		     struct libinput_device *dev)
{
	struct tablet_panel * panel;
	uint32_t button;
	enum libinput_button_state state;

	panel = libinput_device_get_user_data(dev);
	button = libinput_event_tablet_get_button(ev);
	state = libinput_event_tablet_get_button_state(ev);

	switch(button) {
	case BTN_TOUCH:
		update_line(panel, TABLET_STYLUS_TOUCHING_ROW,
			    TABLET_STYLUS_TOUCHING_FIELD,
			    bool_to_string(state));
		panel->stylus_touching = state;
		break;
	case BTN_STYLUS:
		update_line(panel, TABLET_STYLUS_BUTTONS_ROW,
			    TABLET_STYLUS_BUTTONS_FIELD,
			    bool_to_string(state),
			    bool_to_string(panel->stylus_button_2));
		panel->stylus_button_1 = state;
		break;
	case BTN_STYLUS2:
		update_line(panel, TABLET_STYLUS_BUTTONS_ROW,
			    TABLET_STYLUS_BUTTONS_FIELD,
			    bool_to_string(panel->stylus_button_1),
			    bool_to_string(state));
		panel->stylus_button_2 = state;
		break;
	default:
		return;
	}
}

static int
handle_tablet_events()
{
	struct libinput_event *ev;

	libinput_dispatch(li);
	while ((ev = libinput_get_event(li))) {
		struct libinput_device *dev = libinput_event_get_device(ev);

		if (!libinput_device_has_capability(dev, LIBINPUT_DEVICE_CAP_STYLUS))
			continue;

		switch (libinput_event_get_type(ev)) {
		case LIBINPUT_EVENT_NONE:
			abort();
		case LIBINPUT_EVENT_DEVICE_ADDED:
			handle_new_device(ev, dev);
			break;
		case LIBINPUT_EVENT_DEVICE_REMOVED:
			handle_removed_device(ev, dev);
			break;
		case LIBINPUT_EVENT_TABLET_MOTION_ABSOLUTE:
			handle_tablet_motion(libinput_event_get_tablet_event(ev),
					     dev);
			break;
		case LIBINPUT_EVENT_TABLET_TOOL_UPDATE:
			handle_tool_update(libinput_event_get_tablet_event(ev),
					   dev);
			break;
		case LIBINPUT_EVENT_TABLET_AXIS_UPDATE:
			handle_axis_update(libinput_event_get_tablet_event(ev),
					   dev);
			break;
		case LIBINPUT_EVENT_TABLET_BUTTON:
			handle_button_update(libinput_event_get_tablet_event(ev),
					     dev);
			break;
		default:
			break;
		}
	}
	
	return 0;
}

static void
mainloop()
{
	struct pollfd fds[3];
	sigset_t mask;

	fds[0].fd = libinput_get_fd(li);
	fds[0].events = POLLIN;
	fds[0].revents = 0;

	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);
	sigaddset(&mask, SIGWINCH);

	fds[1].fd = signalfd(-1, &mask, SFD_NONBLOCK);
	fds[1].events = POLLIN;
	fds[1].revents = 0;

	fds[2].fd = 0;
	fds[2].events = POLLIN;
	fds[2].revents = 0;

	if (fds[1].fd == -1 ||
	    sigprocmask(SIG_BLOCK, &mask, NULL) == -1) {
		fprintf(stderr, "Failed to set up signal handling (%s)\n",
				strerror(errno));
	}

	/* Create the placeholder panel */
	placeholder_window = newwin(0, 0, 0, 0);
	placeholder_panel = new_panel(placeholder_window);
	paint_placeholder_panel();
	update_display();

	/* Handle already-pending device added events */
	if (handle_tablet_events())
		fprintf(stderr, "Expected device added events on startup but got none. "
				"Maybe you don't have the right permissions?\n");

	while (poll(fds, 3, -1) > -1) {
		if (fds[1].revents) {
			int sig;
			
			/* Check what type of signal is waiting */
			sigwait(&mask, &sig);

			if (sig == SIGINT)
				break;

			endwin();
			refresh();

			/* Resize all of the panels and their windows (if we
			 * have any) */
			if (panel_above(NULL) != placeholder_panel) {
				for (PANEL * p = panel_above(NULL);
				     p != NULL;
				     p = panel_below(p))
				{
					struct tablet_panel * panel =
						(void*)panel_userptr(p);

					paint_panel(panel);
				}
			}
			paint_placeholder_panel();

			update_display();
		} else if (fds[2].revents) {
			switch(getch()) {
			case 'q':
			case 27: /* Escape */
				goto exit_mainloop;
			case KEY_LEFT:
				top_panel(panel_below(NULL));
				show_panel(panel_above(NULL));

				update_display();
				break;
			case KEY_RIGHT:
				bottom_panel(panel_above(NULL));
				show_panel(panel_above(NULL));

				update_display();
				break;
			}
		} else
			handle_tablet_events();
	}
exit_mainloop:

	close(fds[1].fd);
}

int
main()
{
	printf("libinput tablet debugger\n"
	       "©2014 Lyude <thatslyude@gmail.com>\n"
	       "This is free software; see the source for copying conditions. "
	       "There is NO warranty; not even for MERCHANTABILITY or FITNESS "
	       "FOR A PARTICULAR PURPOSE.\n");

	if (open_udev() == 1) {
		fprintf(stderr, "Cannot continue.\n");
		return 1;
	}

	/* setup ncurses */
	setenv("ESCDELAY", "25", 0);
	initscr();
	keypad(stdscr, true);
	noecho();
	clear();

	mainloop();

	endwin();
	udev_unref(udev);

	return 0;	
}
