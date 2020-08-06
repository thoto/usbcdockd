CFLAGS = -g -Wall -Wextra # -Wpedantic
LDFLAGS = -g -Wall -Wextra -Wpedantic
LDFLAGS_UDEV = -ludev

.PHONY: all
all: main evdev-faker

evdev-faker: evdev-faker.o evdev.o
	$(CC) -o $@ $^ $(LDFLAGS)

main: main.o evdev.o
	$(CC) -o $@ $^ $(LDFLAGS) $(LDFLAGS_UDEV)
