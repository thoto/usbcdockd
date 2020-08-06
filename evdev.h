int evdev_init(void);
void evdev_deinit(int fd);
void evdev_dock_event(int evdev_fd, int docked);
