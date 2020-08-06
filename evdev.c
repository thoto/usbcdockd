#include<linux/uinput.h> // evdev event generation
#include<fcntl.h> // open()
#include<unistd.h> // write() close() pause()
#include<string.h> // strlen() strncpy()
#include<err.h>
#include<assert.h>

#include"evdev.h"

#define EVDEV_DEV_NAME "USB type C dock virtual event device"

// configure evdev and return uinput file descriptor
int evdev_init(void){
	struct uinput_setup usetup;
	int fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK | O_CLOEXEC);
	int ret;
	if(fd < 0) err(-1, "uinput device open");

	ioctl(fd, UI_SET_EVBIT, EV_SW);
	ioctl(fd, UI_SET_SWBIT, SW_DOCK);

	memset(&usetup, 0, sizeof(usetup));
	usetup.id.bustype = BUS_USB;
	usetup.id.vendor = 0x1234;
	usetup.id.product = 0x5678;
	assert(strlen(EVDEV_DEV_NAME) < UINPUT_MAX_NAME_SIZE);
	strncpy(usetup.name, EVDEV_DEV_NAME, strlen(EVDEV_DEV_NAME)+1);

	ret = ioctl(fd, UI_DEV_SETUP, &usetup);
	if(ret < 0) errx(-1, "device setup: 0x%x", ret);
	ret = ioctl(fd, UI_DEV_CREATE);
	if(ret < 0) errx(-1, "device creation: 0x%x", ret);

	return fd;
}

void evdev_deinit(int fd){
	ioctl(fd, UI_DEV_DESTROY);
	close(fd);
}

void evdev_emit(int fd, int type, int code, int val){
	struct input_event ie;
	ie.type = type;
	ie.code = code;
	ie.value = val;
	ie.time.tv_sec = 0;
	ie.time.tv_usec = 0;

	if(write(fd, &ie, sizeof(ie)) < 0)
		err(-1, "emit event");
}

void evdev_dock_event(int evdev_fd, int docked){
	evdev_emit(evdev_fd, EV_SW, SW_DOCK, docked);
	evdev_emit(evdev_fd, EV_SYN, SYN_REPORT, 0);
}

