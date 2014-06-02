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

#ifndef FIELD_LOCATIONS_H
#define FIELD_LOCATIONS_H

/* The position of each field */
#define TABLET_SYSTEM_NAME_ROW     0
#define TABLET_STYLUS_TOUCHING_ROW 1

#define TABLET_TOOL_NAME_ROW       3
#define TABLET_TOOL_SERIAL_ROW     4

#define TABLET_X_AND_Y_ROW         6

#define TABLET_TILT_VERTICAL_ROW   8
#define TABLET_TILT_HORIZONTAL_ROW 9

#define TABLET_DISTANCE_ROW        11
#define TABLET_PRESSURE_ROW        12

#define TABLET_STYLUS_BUTTONS_ROW  14

/* The text in each field */
#define TABLET_SYSTEM_NAME_FIELD     "System name: %s"
#define TABLET_STYLUS_TOUCHING_FIELD "Stylus is touching tablet? %s"

#define TABLET_TOOL_NAME_FIELD       "Current tool: %s"
#define TABLET_TOOL_SERIAL_FIELD     "Tool serial number: %d"

#define TABLET_X_AND_Y_FIELD         "X: %7.3f Y: %7.3f"

#define TABLET_TILT_VERTICAL_FIELD   "Vertical tilt: %.3f"
#define TABLET_TILT_HORIZONTAL_FIELD "Horizontal tilt: %.3f"

#define TABLET_DISTANCE_FIELD        "Distance: %.3f"
#define TABLET_PRESSURE_FIELD        "Pressure: %.3f"

#define TABLET_STYLUS_BUTTONS_FIELD  "Stylus Buttons: #1: %-5s  #2: %s"

#endif /* FIELD_LOCATIONS_H */
