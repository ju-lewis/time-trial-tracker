#include <ncurses.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <libusb-1.0/libusb.h>

#define LIST_START_ROW 5
#define POINT_NAME_LEN 20
#define TIMEOUT 3000

typedef struct {
	char name[POINT_NAME_LEN];
	double lat;
	double lon;
} point_t;

int create_point(point_t *point_list, int num_points, libusb_device_handle *handle);

int main(int argc, char *argv[]) {

	// Configure screen
	initscr();
	noecho();
	keypad(stdscr, TRUE);
	int row, col;
	getmaxyx(stdscr, row, col);

	int input, status;
	char *heading = "GPS Point Logging Software.";
	libusb_device_handle *dev = NULL;
	libusb_device **list;


	// Connect to USB Device
	printw("Connecting to GPS...\n");
	status = libusb_init(NULL);
	printw("Initialised with code: %s\n",  libusb_error_name(status));
	refresh();

	// Get device based on product and vendor ID
	dev = libusb_open_device_with_vid_pid(NULL, 0x10c4, 0xea70);

	if(dev) {
		printw("Connected! Device Handle = %p\n", dev);
		refresh();

		// Detach kernel processes and claim interface
		libusb_detach_kernel_driver(dev, 0);
		status = libusb_claim_interface(dev, 0);
		printw("Claimed interface: %s\n", libusb_error_name(status));
		refresh();

		char data[256];
		memset(data, ' ', 256);
		int len;

		// Read data from GPS
		printw("Transferring data\n");
		refresh();
		status = libusb_bulk_transfer(dev, 0x81, data, 256, &len, TIMEOUT);
		printw("Status: %s  Length read: %d  Read from USB: %s\n", libusb_error_name(status), len, data);
		refresh();
		libusb_release_interface(dev, 0);

		// Reattach kernel driver
		libusb_attach_kernel_driver(dev, 0);
	} else {
		
		libusb_exit(NULL);
		endwin();
		printf("Couldn't connect to GPS device!\n");
		return 0;
	}
	
	// Print heading to screen
	mvprintw(2,(col-strlen(heading))/2, "%s\n", heading);
	refresh();

	while(1) {
		input = getch();

		switch (input) {
			case KEY_LEFT:
				break;
			case KEY_UP:
				break;
			case KEY_RIGHT:
				break;
			case KEY_DOWN:
				break;
			default:
				break;
		}
		refresh();
	}

	// Clean up and exit
	if(dev) {libusb_close(dev);}

	libusb_exit(NULL);
	endwin();
	return 0;
}

/* Appends a race point to point_list */
int create_point(point_t *point_list, int num_points, libusb_device_handle *handle) {
	// Realloc point_list to hold 1 more point
	point_list = realloc(point_list, sizeof(point_t) * (++num_points));

	point_t new_point;
	char data[33];
	int len;
	// Read data from GPS
	int status = libusb_bulk_transfer(handle, 0x81, data, 32, &len, 0);
	printw("Read from USB: %s\n", data);
}


