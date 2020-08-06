#include<unistd.h> // sleep()
#include<string.h> // strlen() strncpy() strdup()
#include<stdlib.h> // exit()
#include<sys/select.h> // select()
#include<signal.h>

#include<argp.h>
#include<err.h>
#include<assert.h>

#include<libudev.h>

#include"common.h"
#include"evdev.h"

#define DESCR "virtual USB type C dock event"

// *** printing stuff ***
int verb=0;
int termcolor_on=1;
const char* termcolor_strings[] = { "\033[0m", "\033[0;31m", "\033[0;34m" };
#define TERMCOLOR_OFF 0
#define TERMCOLOR_RED 1
#define TERMCOLOR_BLUE 2
#define TERMCOLOR_NONE -1
void termcolor(int color){
	if(termcolor_on && color != TERMCOLOR_NONE)
		printf("%s", termcolor_strings[color]);
}
#define VERB(color, ...) \
	{ if(verb) { termcolor(color); printf(__VA_ARGS__); \
		termcolor(TERMCOLOR_OFF); } }

void dock_event(int val, int evdev_fd, char* cmd){
	pid_t p;
	evdev_dock_event(evdev_fd, val);

	VERB(TERMCOLOR_RED, "docked = %d%c", val, !cmd?'\n':' ');
	if(!cmd)
		return;
	VERB(TERMCOLOR_RED, "exec cmd %s\n", cmd);
	p=fork();
	assert(p >= 0);
	if(p==0){
		signal(SIGINT, SIG_DFL);
		// TODO: close stuff here
		execl("/bin/sh", "sh", "-c", cmd, (char*) NULL);
	}
}

struct udev_monitor* udev_init(){
	struct udev* udev;
	struct udev_monitor* mon;
	int ret;

	udev = udev_new(); // never fails if malloc does not
	assert(udev);
	mon = udev_monitor_new_from_netlink(udev, "udev");
	if(!mon){
		fprintf(stderr, "Cannot open udev monitor. Is udev running?");
		return NULL;
	}
	ret = udev_monitor_filter_add_match_subsystem_devtype(mon, "usb",
			"usb_device");
	assert(ret >= 0);
	ret = udev_monitor_enable_receiving(mon);
	assert(ret >= 0);
	return mon;
}

void udev_deinit(struct udev_monitor* mon){
	struct udev* udev;
	if(mon){
		udev = udev_monitor_get_udev(mon);
		udev_monitor_unref(mon);
		udev_unref(udev);
	}else warnx("udev monitor not initialised");
}

// vendor and product string must be preallocated
int udev_parse_usbid(char* usbid, char* vendor, char* product){
	size_t i;
	size_t n = 0; // counter for number of digits: must not be > 4
	size_t m = 0; // : character found

	for(i=0; i<strlen(usbid); i++){
		if((usbid[i]>='0' && usbid[i]<='9') ||
			(usbid[i]>='A' && usbid[i]<='F') ||
			(usbid[i]>='a' && usbid[i]<='f')){ // hexadecimal digit
			n++;
			if(n > 4) // more than 4 digits
				return 1;
		} else if(usbid[i]==':' && m == 0){ // separator
			if(n==0) // there must be a vendor id
				return 2;
			strncpy(vendor, usbid, n);
			vendor[n]='\0';
			n = 0; // reset digit counter
			m = i+1; // indicate found :
		} else return 3;
	}
	if(m >= i) // there must be a product id
		return 4;
	strncpy(product, usbid+m, n);
	product[n]='\0';
	return 0;
}
const char* udev_parse_usbid_errstrs[] = {"success",
	"invalid vendor/product id",
	"there must be a vendor id",
	"invalid character in vendor/product id",
	"there must be a product id"};

int udev_match_add(struct udev_device* udev_dev, char* vendor_id,
		char* product_id){
	const char* s;
	 // either no action at all or add action for enumeration and monitor match
	s=udev_device_get_action(udev_dev);
	if(s != NULL && strcmp(s, "add") != 0) return 0;
	s = udev_device_get_sysattr_value(udev_dev, "idVendor");
	if(!s || strcmp(s, vendor_id) != 0) return 0;
	s = udev_device_get_sysattr_value(udev_dev, "idProduct");
	if(!s || strcmp(s, product_id) != 0) return 0;
	return 1;
}
int udev_match_remove(struct udev_device* udev_dev, char* syspath){
	const char* s;
	if(!syspath) return 0;
	if(strcmp(udev_device_get_action(udev_dev), "remove") == 0){
		s = udev_device_get_syspath(udev_dev);
		if(!s || strcmp(s, syspath) != 0) return 0;
		return 1;
	}
	return 0;
}
void udev_dbg(struct udev_device* udev_dev){
	const char* s;
	if(verb){
		s=udev_device_get_action(udev_dev);
		if(s){
			VERB(TERMCOLOR_BLUE, "action %s: ", s);
		}else
			VERB(TERMCOLOR_BLUE, "scanned: ");
		s=udev_device_get_sysattr_value(udev_dev,"idVendor");
		VERB(TERMCOLOR_BLUE, "vendor %s, ", s?s:"");
		s=udev_device_get_sysattr_value(udev_dev,"idProduct");
		VERB(TERMCOLOR_BLUE, "product %s, ", s?s:"");
		s=udev_device_get_syspath(udev_dev);
		VERB(TERMCOLOR_BLUE, "syspath %s\n", s?s:"");
	}
/*					if(verb){
		s=udev_device_get_sysattr_value(udev_dev,"idVendor");
		VERB(TERMCOLOR_BLUE, "vendor: %s\n", s?s:"");
		s=udev_device_get_sysattr_value(udev_dev,"idProduct");
		VERB(TERMCOLOR_BLUE, "product: %s\n", s?s:"");
		s=udev_device_get_syspath(udev_dev);
		VERB(TERMCOLOR_BLUE, "syspath: %s\n", s?s:"");
		s=udev_device_get_sysname(udev_dev);
		VERB(TERMCOLOR_BLUE, "sysname: %s\n", s?s:"");
		s=udev_device_get_sysnum(udev_dev);
		VERB(TERMCOLOR_BLUE, "sysnum: %s\n", s?s:"");
		s=udev_device_get_devpath(udev_dev);
		VERB(TERMCOLOR_BLUE, "devpath: %s\n", s?s:"");
		s=udev_device_get_devnode(udev_dev);
		VERB(TERMCOLOR_BLUE, "devnode: %s\n", s?s:"");
		s=udev_device_get_devtype(udev_dev);
		VERB(TERMCOLOR_BLUE, "devtype: %s\n", s?s:"");
		s=udev_device_get_subsystem(udev_dev);
		VERB(TERMCOLOR_BLUE, "subsystem: %s\n", s?s:"");
		s=udev_device_get_driver(udev_dev);
		VERB(TERMCOLOR_BLUE, "driver: %s\n", s?s:"");
	}*/
}

struct udev_device* udev_scan_device(struct udev* udev, char* vendor_id,
		char* product_id){
	struct udev_enumerate* e;
	struct udev_list_entry* l;
	struct udev_list_entry* le;
	struct udev_device* d;
	int ret;

	e = udev_enumerate_new(udev);
	ret = udev_enumerate_add_match_subsystem(e, "usb");
	assert(ret >= 0);
	// since matching multiple properties does not work, match just devtype
	// better filtering could be achieved by matching model_id
	ret = udev_enumerate_add_match_property(e, "DEVTYPE", "usb_device");
	assert(ret >= 0);
	ret = udev_enumerate_scan_devices(e);
	assert(ret >= 0);
	l = udev_enumerate_get_list_entry(e);
	assert(l != NULL);

	udev_list_entry_foreach(le, l){
		d = udev_device_new_from_syspath(udev, udev_list_entry_get_name(le));
		udev_dbg(d);
		if(udev_match_add(d, vendor_id, product_id))
			return d;
		udev_device_unref(d);
	};
	return NULL;
}

// TODO: on suspend/resume: does device still exist?
// via udev_device_new_from_syspath()


int main(int argc, char** argv){
	int fd;
	int x=0;
	int o;
	char* cmd=NULL;

	struct udev_monitor* udev_mon = NULL;
	struct udev_device* udev_dev = NULL;
	int udev_fd;
	fd_set udev_fds;
	char udev_vendor_id[5], udev_product_id[5];
	char* udev_syspath = NULL;
	const char* s;
	const char* opts="h?vVr:";

	void quit(int code){
		dock_event(0, fd, cmd);
		evdev_deinit(fd);
		udev_deinit(udev_mon);
		exit(code);
	}
	void quit_sig(__attribute__((unused)) int sig){
		quit(0);
	}
	signal(SIGINT, quit_sig);

	#define USAGE fprintf(stderr, "Usage: %s [-h?vV] [-r command] "\
		"vendor-id:product-id\n", argv[0]);

	// option parsing
	while((o = getopt(argc, argv, opts)) != -1){
		switch(o){
			// functional arguments
			case 'v': verb=1; break;
			case 'r': cmd=optarg; break;
			// other arguments
			case 'V':
				fprintf(stderr, "%s: %s version %s\n",argv[0], DESCR, VERSION);
				exit(0);
				break;
			case '?':
			case 'h':
				USAGE;
				fprintf(stderr, "\t-h, -?\tprint this help and exit\n");
				fprintf(stderr, "\t-V\tprint version and exit\n");
				fprintf(stderr, "\t-v\tincrease output verbosity\n");
				fprintf(stderr, "\t-r\trun command on docking action\n");
				exit(0);
				break;
			default:
				USAGE;
				exit(1);
		}
	}

	if((optind+1) == argc){
		x = udev_parse_usbid(argv[optind], udev_vendor_id, udev_product_id);
		if(x)
			errx(-1, "invalid usb vendor/product id string: %s",
				udev_parse_usbid_errstrs[x]);
		VERB(TERMCOLOR_NONE,
			"parsing for vendor id %s, product id %s\n",
			udev_vendor_id, udev_product_id);
	}else{
		USAGE;
		exit(1);
	}

	// initializing and running
	fd=evdev_init();
	sleep(1);
	udev_mon = udev_init();
	if(!udev_mon) quit(-1);
	assert(udev_mon);
	udev_fd = udev_monitor_get_fd(udev_mon);
	assert(udev_fd);

	udev_dev = udev_scan_device(udev_monitor_get_udev(udev_mon),
		udev_vendor_id, udev_product_id);
	if(udev_dev){
		s = udev_device_get_syspath(udev_dev);
		udev_syspath = strdup(s);
		dock_event(1, fd, cmd);
		udev_device_unref(udev_dev);
	}

	while(1){
		FD_ZERO(&udev_fds);
		FD_SET(udev_fd, &udev_fds);
		x = select(udev_fd+1, &udev_fds, NULL, NULL, NULL);
		if( x > 0 && FD_ISSET(udev_fd, &udev_fds)){
			udev_dev = udev_monitor_receive_device(udev_mon);
			udev_dbg(udev_dev);
			if(udev_match_add(udev_dev, udev_vendor_id,
					udev_product_id)){
				free(udev_syspath);
				s = udev_device_get_syspath(udev_dev);
				udev_syspath = strdup(s);
				dock_event(1, fd, cmd);
			}
			if(udev_match_remove(udev_dev, udev_syspath)){
				free(udev_syspath);
				udev_syspath=NULL;
				dock_event(0, fd, cmd);
			}
			udev_device_unref(udev_dev);
		} else assert(NULL);
	}
	quit(0);

	return 0;
}
