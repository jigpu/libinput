/*
 * Copyright © 2014 Lyude
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
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE
 */

#ifndef MISC_H
#define MISC_H

/* Same bitfield helpers as those in ../src/libinput-util.h, originally taken
 * from libevdev.
 */
static inline int
bit_is_set(const unsigned char *array, int bit)
{
    return !!(array[bit / 8] & (1 << (bit % 8)));
}

static inline void
set_bit(unsigned char *array, int bit)
{
    array[bit / 8] |= (1 << (bit % 8));
}

static inline void
clear_bit(unsigned char *array, int bit)
{
    array[bit / 8] &= ~(1 << (bit % 8));
}

/* Just here for convienence purposes. Copied from src/libinput-util.h */
#define ARRAY_LENGTH(a) (sizeof (a) / sizeof (a)[0])

#endif /* MISC_H */
