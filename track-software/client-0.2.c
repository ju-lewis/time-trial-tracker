#include <stdlib.h>
#include <ncurses.h>
#include <string.h>
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

int main(int argc, char *argv[]) {

	// Init ncurses screen
	initscr();
	noecho();
	keypad(stdscr, TRUE);
	int row, col;
	getmaxyx(stdscr, row, col);

	int serial_port = open("/dev/ttyUSB0", O_RDONLY);
	if(serial_port < 0) {
		// Failed to open
		printw("Couldn't access GPS! (Error: %i)\nPress 'q' to exit.\n", errno);
		while(1) {
			// Loop until q is pressed, then exit
			if(getch() == 'q') {
				break;
			}
		}
		goto end;
	}
	
	// Now configure serial port for reading
	struct termios tty;
	if(tcgetattr(serial_port, &tty) != 0) {
		// Handle error
		goto end;
	}

	tty.c_cflag &= ~PARENB; // Clear parity bit
	tty.c_cflag &= ~CSTOPB; // 1 Stop bit

	tty.c_cflag &= ~CSIZE;
	tty.c_cflag |= CS8;     // 8 Bits per byte

	tty.c_cflag &= ~CRTSCTS; // Disable RTS/CTS hardware flow control
	tty.c_cflag |= CREAD | CLOCAL; // Turn on READ & ignore ctrl lines

	tty.c_lflag &= ~ICANON;	// Disable canonical mode
	tty.c_lflag &= ~ECHO; // Disable echo
	tty.c_lflag &= ~ECHOE; // Disable erasure
	tty.c_lflag &= ~ECHONL; // Disable new-line echo

	tty.c_lflag &= ~ISIG; // Disable interpretation of INTR, QUIT and SUSP

	tty.c_lflag &= ~ISIG; // Disable interpretation of INTR, QUIT and SUSP
	tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL); // Disable any special handling of received bytes

	tty.c_cc[VTIME] = 100; // Wait for 100ms when read is called
	tty.c_cc[VMIN] = 0;    // Don't expect any bytes on read

	cfsetispeed(&tty, 57600);
	cfsetospeed(&tty, 57600);

	if(tcsetattr(serial_port, TCSANOW, &tty) != 0) {
		fprintf(stderr, "Couldn't set port attributes! (Error: %i)\n", errno);
		goto end;
	}

	// Now we can read data!
	int ch;
	mvprintw(5, (col-21)/2, "GPS Logging Software.");
	while(1) {
		ch = getch(); // Get user input on each frame
		
		if(ch == 'q') {
			printw("Would you like to quit? (Press 'q' again to quit)");
			refresh();
			while(1) {
				ch = getch();
				if(ch == 'q'){
					goto end;
				} else {
					// Clear prompt
					move(1,1);
					clrtoeol;
					refresh();
					break;
				}
			}
		}
	}


end:
	endwin();
	return 0;
}
