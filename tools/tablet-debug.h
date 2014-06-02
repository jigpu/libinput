/*
 * Copyright ©2014 Lyude
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
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE*
 */

#ifndef TABLET_DEBUG_H
#define TABLET_DEBUG_H

#include "libinput-util.h"

/* window widths */
#define WIN_TABLET_PANEL_WIDTH 43
#define WIN_SEAT_COUNT_WIDTH    0 /* auto width */
#define WIN_HELP_WIDTH          0
#define WIN_PLACEHOLDER_WIDTH   0
#define WIN_LOG_WIDTH           0
#define WIN_STATUS_WIDTH        0

/* window heights */
#define WIN_TABLET_PANEL_HEIGHT (LINES - 1)
#define WIN_SEAT_COUNT_HEIGHT   (LINES - 1)
#define WIN_HELP_HEIGHT         (LINES - 1)
#define WIN_PLACEHOLDER_HEIGHT  (LINES - 1)
#define WIN_LOG_HEIGHT          (LINES - 1)
#define WIN_STATUS_HEIGHT       1

/* window begin_y values */
#define WIN_TABLET_PANEL_BEGIN_Y 0
#define WIN_SEAT_COUNT_BEGIN_Y   0
#define WIN_HELP_BEGIN_Y         0
#define WIN_PLACEHOLDER_BEGIN_Y  0
#define WIN_LOG_BEGIN_Y          0
#define WIN_STATUS_BEGIN_Y       (LINES - 1)

/* window begin_x values */
#define WIN_TABLET_PANEL_BEGIN_X 0
#define WIN_SEAT_COUNT_BEGIN_X   (WIN_TABLET_PANEL_WIDTH + 1)
#define WIN_HELP_BEGIN_X         0
#define WIN_PLACEHOLDER_BEGIN_X  0
#define WIN_LOG_BEGIN_X          0
#define WIN_STATUS_BEGIN_X       0

/* the row of each field */
#define TABLET_SYSTEM_NAME_ROW         0
#define TABLET_STYLUS_TOUCHING_ROW     1

#define TABLET_TOOL_NAME_ROW           3
#define TABLET_TOOL_SERIAL_ROW         4

#define TABLET_X_AND_Y_ROW             6

#define TABLET_TILT_VERTICAL_ROW       8
#define TABLET_TILT_HORIZONTAL_ROW     9

#define TABLET_DISTANCE_ROW           11
#define TABLET_PRESSURE_ROW           12

#define TABLET_STYLUS_BUTTONS_ROW     14
#define TABLET_PAD_BUTTONS_1_TO_3_ROW 15
#define TABLET_PAD_BUTTONS_4_TO_6_ROW 16
#define TABLET_PAD_BUTTONS_7_TO_9_ROW 17

#define SEAT_COUNT_BTN_TOUCH_ROW          1
#define SEAT_COUNT_STYLUS_BUTTONS_ROW     2
#define SEAT_COUNT_PAD_BUTTONS_1_TO_3_ROW 3
#define SEAT_COUNT_PAD_BUTTONS_4_TO_6_ROW 4
#define SEAT_COUNT_PAD_BUTTONS_7_TO_9_ROW 5

/* The text in each field */
#define TABLET_SYSTEM_NAME_FIELD        "System name: %s"
#define TABLET_STYLUS_TOUCHING_FIELD    "Stylus is touching tablet? %s"

#define TABLET_TOOL_NAME_FIELD          "Current tool: %s"
#define TABLET_TOOL_SERIAL_FIELD        "Tool serial number: %d"

#define TABLET_X_AND_Y_FIELD            "X: %7.3f Y: %7.3f"

#define TABLET_TILT_VERTICAL_FIELD      "Vertical tilt: %.7f"
#define TABLET_TILT_HORIZONTAL_FIELD    "Horizontal tilt: %.7f"

#define TABLET_DISTANCE_FIELD           "Distance: %-2.0f mm"
#define TABLET_PRESSURE_FIELD           "Pressure: %.7f"

#define TABLET_STYLUS_BUTTONS_FIELD     "Stylus buttons: #1: %-5s  #2: %s"
#define TABLET_PAD_BUTTONS_1_TO_3_FIELD "Pad buttons: #1: %-5s #2: %-5s #3: %s"
#define TABLET_PAD_BUTTONS_4_TO_6_FIELD "             #4: %-5s #5: %-5s #6: %s"
#define TABLET_PAD_BUTTONS_7_TO_9_FIELD "             #7: %-5s #8: %-5s #9: %s"

#define SEAT_COUNT_BTN_TOUCH_FIELD          "BTN_TOUCH: %d"
#define SEAT_COUNT_STYLUS_BUTTONS_FIELD     "Stylus buttons: #1: %-3d  #2: %d"
#define SEAT_COUNT_PAD_BUTTONS_1_TO_3_FIELD "Pad buttons: #1: %-3d #2: %-3d #3: %d"
#define SEAT_COUNT_PAD_BUTTONS_4_TO_6_FIELD "             #4: %-3d #5: %-3d #6: %d"
#define SEAT_COUNT_PAD_BUTTONS_7_TO_9_FIELD "             #7: %-3d #8: %-3d #9: %d"

/* various status messages */
#define STATUS_MSG_DEFAULT   "Press h for help"
#define STATUS_MSG_ERROR     "libinput error occured, press l to view the log"
#define STATUS_MSG_IN_WINDOW "Press q to go back to the debugger"

enum status_color {
	STATUS_COLOR_NONE = 0,
	STATUS_COLOR_ERROR = 1
};

#endif /* TABLET_DEBUG_H */
