#include<linux/uinput.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<string.h>
#include<stdio.h>
#include<err.h> // err()
#include<stdlib.h> // exit()
#include<argp.h> // exit()

#define EVDEV_DEV_NAME "USB type C dock virtual event device"
#define VERSION "0.0.1"
#define DESCR "virtual USB type C dock event"

int verb=0;

int evdev_init(void){
	struct uinput_setup usetup;
	int fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
	int ret;
	if(fd < 0) err(-1, "uinput device open");
	
	ioctl(fd, UI_SET_EVBIT, EV_SW);
	ioctl(fd, UI_SET_SWBIT, SW_DOCK);

	memset(&usetup, 0, sizeof(usetup));
	usetup.id.bustype = BUS_USB;
	usetup.id.vendor = 0x1234;
	usetup.id.product = 0x5678;
//	assert(strlen(EVDEV_DEV_NAME) < UINPUT_MAX_NAME_SIZE);
	strncpy(usetup.name, EVDEV_DEV_NAME, strlen(EVDEV_DEV_NAME)+1);

	ret = ioctl(fd, UI_DEV_SETUP, &usetup);
	if(ret < 0) errx(-1, "device setup: 0x%x", ret);
	ret = ioctl(fd, UI_DEV_CREATE);
	if(ret < 0) errx(-1, "device creation: 0x%x", ret);

	return fd;
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

void evdev_dock(int fd, int val){
	evdev_emit(fd, EV_SW, SW_DOCK, val);
	evdev_emit(fd, EV_SYN, SYN_REPORT, 0);

	if(verb)
		printf("docked = %d\n", val);
}

int evdev_deinit(int fd){
	ioctl(fd, UI_DEV_DESTROY);
	close(fd);
	if(verb)
		printf("quit");
	exit(0);
}
	

int main(int argc, char** argv){
	int fd;
	int x=0;
	int o;

	while((o = getopt(argc, argv, "vV")) != -1){
		switch(o){
			case 'v': verb=1; break;
			case 'V':
				printf("%s: %s version %s\n",argv[0], DESCR, VERSION);
				exit(0);
			break;
			default:
				err(1, "Usage: %s [-vV]\n", argv[0]);
		}
	}

	fd=evdev_init();
	sleep(1);
	evdev_dock(fd, 0);
	while(1){
		getchar();
		x^=1;
		evdev_dock(fd, x);
	}
	evdev_dock(fd, 0);
	evdev_deinit(fd);

	return 0;
}
