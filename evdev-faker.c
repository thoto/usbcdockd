#include<unistd.h> // pause()
#include<stdlib.h> // exit()
#include<argp.h>
#include<signal.h>
#include<err.h>
#include<assert.h>

#include"common.h"
#include"evdev.h"

#define DESCR "virtual USB type C dock event"

#define M_KEY 0
#define M_DAEMON 1

int main(int argc, char** argv) {
	int fd;
	char c='\0';
	int mode=M_KEY;
	int o;

	const char* opts="h?Vd";

	void quit(int code){
		evdev_dock_event(fd, 0);
		evdev_deinit(fd);

		exit(code);
	}
	void quit_sig(__attribute__((unused)) int sig){
		quit(0);
	}
	signal(SIGINT, quit_sig);

	// option parsing
	while((o = getopt(argc, argv, opts)) != -1){
		switch(o){
			case 'd':
				mode=M_DAEMON;
				break;
			case 'V':
				fprintf(stderr, "%s: %s version %s\n",argv[0], DESCR, VERSION);
				exit(0);
				break;
			case '?':
			case 'h':
				fprintf(stderr, "Usage: %s [-%s]\n", argv[0], opts);
				fprintf(stderr, "\t-h, -?\tprint this help and exit\n");
				fprintf(stderr, "\t-V\tprint version and exit\n");
				fprintf(stderr, "\t-d\tdaemon mode: run until termination\n");
				fprintf(stderr, "\n\nunless run in daemon mode it toggles the"
					"emulated event device state by input of y or n and "
					"quits on q (followed by newline)\n");
				exit(0);
				break;
			default:
				fprintf(stderr, "Usage: %s [-%s]\n", argv[0], opts);
				exit(1);
		}
	}

	// initializing and running
	fd=evdev_init();
	sleep(1);
	if(mode==M_DAEMON){
		evdev_dock_event(fd, 1);
		pause();
	}else if(mode==M_KEY){
		evdev_dock_event(fd, 0);
		while( (c=getchar()) ){
			if(c=='y')
				evdev_dock_event(fd, 1);
			else if(c=='n')
				evdev_dock_event(fd, 0);
			else if(c=='q')
				break;
		}
	}
	quit(0);
	return 0;
}
