#include <stdio.h>
#include <libinput.h>

#include "libinput-util.h"
#include "litest.h"

struct libevdev_uinput* create_uinput_device_from_file(FILE *f)
{
	struct input_absinfo *absinfo;
	int *events;
	unsigned char nabsinfo;
	unsigned char nevents;
	int i;
	struct input_id input_id = {
		.bustype = 0x11,
		.vendor = 0x2,
		.product = 0x8,
	};

	if (fread(&nabsinfo, 1, 1, f) != 1)
		return NULL;

	if (fread(&nevents, 1, 1, f) != 1)
		return NULL;

	if (nabsinfo) {
		absinfo = calloc(nabsinfo, sizeof(*absinfo));
		if (absinfo == NULL)
			return NULL;
	}
	else {
		absinfo = NULL;
	}

	if (nevents) {
		events = calloc(nevents, sizeof(*events));
		if (events == NULL)
			return NULL;
	}
	else {
		events = NULL;
	}

	for (i = 0; i < nabsinfo; i++) {
		if (fread(&absinfo[i], sizeof(*absinfo), 1, f) != sizeof(*absinfo))
			return NULL;
	}
	for (i = 0; i < nevents; i++) {
		if (fread(&events[i], sizeof(*events), 1, f) != sizeof(*events))
			return NULL;
	}

	return litest_create_uinput_device_from_description("test device",
							    &input_id,
							    absinfo,
							    events);
}

int write_packet_from_file(FILE *f, struct libevdev_uinput *uinput)
{
	int type;
	int code;
	int value;

	do {
		if (fread(&type, sizeof(int), 1, f) != sizeof(int))
			return 0;
		if (fread(&code, sizeof(int), 1, f) != sizeof(int))
			return 0;
		if (fread(&value, sizeof(int), 1, f) != sizeof(int))
			return 0;

		libevdev_uinput_write_event(uinput, type, code, value);
	} while (type != 0 && code != 0 && value != 0);

	return 1;
}

START_TEST(device_events_from_file)
{
	struct libevdev_uinput *uinput;
	struct libinput *li;
	struct libinput_device *device;
	FILE *f;

	f = fopen("/tmp/foo.bin", "rb");
	if (f == NULL)
		return;

	uinput = create_uinput_device_from_file(f);
	ck_assert(uinput != NULL);

	li = litest_create_context();
	device = libinput_path_add_device(li,
					  libevdev_uinput_get_devnode(uinput));

	ck_assert(device != NULL);

	litest_disable_log_handler(li);
	while (write_packet_from_file(f, uinput)) {
		libinput_dispatch(li);
	}
	litest_restore_log_handler(li);

	libinput_unref(li);
	libevdev_uinput_destroy(uinput);
}
END_TEST

void
litest_setup_tests(void)
{
	litest_add_no_device("file:stream", device_events_from_file);
}
