
CFLAGS = -DCONF_UDEV -g -Wall -Wextra # -Wpedantic
LDFLAGS = -g -ludev -Wall -Wextra -Wpedantic

main: main.o
	$(CC) -o $@ $^ $(LDFLAGS)
	
all: main
