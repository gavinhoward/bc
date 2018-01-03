#include <readline/readline.h>

int readchar(void)
{
	static struct termios term, oterm;
	char str[1];
	str[0] = 0;
	if ((tcgetattr(0, &oterm)) != 0)
		return -1;
	memcpy(&term, &oterm, sizeof(term));
	term.c_lflag &= ~(ICANON | ECHO);
	term.c_cc[VMIN] = 1;
	term.c_cc[VTIME] = 0;
	if ((tcsetattr(0, TCSANOW, &term)) != 0 )
		return -1;
	read(0, str, 1);
	if ((tcsetattr(0, TCSANOW, &oterm)) != 0 )
		return -1;
	return str[0];
}

