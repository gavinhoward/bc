#include <readline/readline.h>

void determinewin(void)
{ 
	ioctl(0, TIOCGWINSZ, &win);
	hglb.w = win.ws_col;
	hglb.h = win.ws_row;
}
