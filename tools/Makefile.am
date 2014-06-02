noinst_PROGRAMS = event-debug

AM_CPPFLAGS = -I$(top_srcdir)/include \
              -I$(top_srcdir)/src

event_debug_SOURCES = event-debug.c
event_debug_LDADD = ../src/libinput.la $(LIBUDEV_LIBS)
event_debug_LDFLAGS = -static
event_debug_CFLAGS = $(LIBUDEV_CFLAGS)

if HAVE_NCURSES
if HAVE_PANEL
noinst_PROGRAMS += tablet-debug

tablet_debug_SOURCES = tablet-debug.c
tablet_debug_LDADD = ../src/libinput.la $(LIBUDEV_LIBS) $(PANEL_LIBS) $(CURSES_LIBS)
tablet_debug_LDFLAGS = -static
tablet_debug_CFLAGS = $(LIBUDEV_CFLAGS)
endif
endif
