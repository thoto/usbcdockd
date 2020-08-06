CFLAGS = -DCONF_UDEV -g -Wall -Wextra # -Wpedantic
LDFLAGS = -g -Wall -Wextra -Wpedantic
LDFLAGS_UDEV = -ludev

.PHONY: all

evdev-faker: evdev-faker.o evdev.o
	$(CC) -o $@ $^ $(LDFLAGS)

main: main.o evdev.o
	$(CC) -o $@ $^ $(LDFLAGS) $(LDFLAGS_UDEV)
	
all: main evdev-faker
