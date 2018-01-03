#include <sys/ioctl.h>
#include <termios.h>

/* Cursor movement */
#define T_CURSUP1ROW	"\033[A"	/* [A	Cursor up one row -- stop at top of screen 	*/
#define T_CURSUP1ROW_SZ	3
#define T_CURSDN1ROW    "\033[B"	/* [B	Cursor down one row -- stop at bottom of screen	*/
#define T_CURSDN1ROW_SZ 3
#define T_CURS4D1COL    "\033[C"	/* [C	Cursor forward one col -- stop at right of screen */
#define T_CURS4D1COL_SZ	3
#define T_CURSBK1COL    "\033[D"	/* [D	Cursor back one col -- stop at left of screen 	*/
#define T_CURSBK1COL_SZ	3
#define T_GOHOME	"\033[H"	/* [H	Go home to row 1 x col 1 aka: [1;1H 		*/
#define T_GOHOME_SZ 	3
#define T_CLRCUR2BOT	"\033[J"	/* [J	Clear from cur pos to bottom of screen 		*/
#define T_CLRCUR2BOT_SZ 3
#define T_CLRCUR2END    "\033[K"	/* [K	Clear from cur pos to end of line 		*/ 
#define T_CLRCUR2END_SZ	3
#define T_CLRCHARS	"\033[0m"	/* [0m	Clear attributes to normal chars 		*/ 
#define T_CLRCHARS_SZ	4

/* Colors */
#define T_BOLD		"\033[1m"	/* 1	Bold 		*/
#define T_BOLD_SZ 	4
#define T_DIM		"\033[2m"	/* 2	Dim 		*/
#define T_DIM_SZ 	4
#define T_UNDER      	"\033[4m"	/* 4 	Underscore 	*/	
#define T_UNDER_SZ	4
#define T_BLINK      	"\033[5m"	/* 5 	Blink 		*/
#define T_BLINK_SZ	4
#define T_REVERSE      	"\033[7m"	/* 7 	Reverse 	*/
#define T_REVERSE_SZ	4
#define T_INVERSECHARS	"\033[;7m"	/* [0;7m	Set char attributes to inverse video only */ 
#define T_HIDDEN      	"\033[8m"	/* 8 	Hidden 		*/
#define T_HIDDEN_SZ	4
#define T_BLACK_FG      "\033[30m"	/* 30 	Black fg 	*/
#define T_BLACK_FG_SZ	5
#define T_RED_FG      	"\033[31m"	/* 31 	Red fg 		*/
#define T_RED_FG_SZ	5
#define T_GREEN_FG      "\033[32m"	/* 32 	Green fg 	*/
#define T_GREEN_FG_SZ	5
#define T_YELLOW_FG     "\033[33m"	/* 33 	Yellow fg 	*/ 
#define T_YELLOW_FG_SZ	5
#define T_BLUE_FG       "\033[34m"	/* 34 	Blue fg 	*/ 
#define T_BLUE_FG_SZ	5
#define T_MAGENTA_FG	"\033[35m"	/* 35 	Magenta fg 	*/ 
#define T_MAGENTA_FG_SZ	5
#define T_CYAN_FG	"\033[36m"	/* 36 	Cyan fg 	*/ 
#define T_CYAN_FG_SZ	5
#define T_WHITE_FG      "\033[37m"	/* 37 	White fg 	*/
#define T_WHITE_FG_SZ	5
#define T_BLACK_BG      "\033[40m"	/* 40	Black bg 	*/
#define T_BLACK_BG_SZ	5
#define T_RED_BG      	"\033[41m"	/* 41	Red bg 		*/
#define T_RED_BG_SZ	5
#define T_GREEN_BG     	"\033[42m"	/* 42	Green bg 	*/
#define T_GREEN_BG_SZ	5
#define T_YELLOW_BG    	"\033[43m"	/* 43	Yellow bg 	*/
#define T_YELLOW_BG_SZ	5
#define T_BLUE_BG      	"\033[44m"	/* 44	Blue bg 	*/
#define T_BLUE_BG_SZ	5
#define T_MAGENTA_BG    "\033[45m"	/* 45	Magenta bg 	*/
#define T_MAGENTA_BG_SZ	5
#define T_CYAN_BG      	"\033[46m"	/* 46	Cyan bg 	*/
#define T_CYAN_BG_SZ	5
#define T_WHITE_BG      "\033[47m"	/* 47	White bg 	*/ 
#define T_WHITE_BG_SZ	5

/* Editing */
#define T_ERASECUR2BOT 	"\033[0J"	/* [0J	Erase from cur pos to bottom of screen inclusive 	*/
#define T_ERASECUR2BO_SZ 4
#define T_ERASETOP2CUR 	"\033[1J"	/* [1J	Erase from top of screen to cur pos inclusive 		*/
#define T_ERASETOP2CUR_SZ 4
#define T_ERASEALL 	"\033[2J"	/* [2J	Erase entire screen (without moving the cursor) 	*/
#define T_ERASEALL_SZ	4
#define T_ERASECUR2ENDL "\033[0K"	/* [0K	Erase from cur pos to end of line inclusive 		*/
#define T_ERASECUR2ENDL_SZ 4
#define T_ERASEBEGL2CUR "\033[1K"	/* [1K	Erase from beginning of line to cur pos inclusive 	*/
#define T_ERASEBEGL2CUR_SZ 4
#define T_ERASELINE	"\033[2K"	/* [2K 	Erase entire line without moving cursor 		*/ 
#define T_ERASELINE_SZ	4

/* Identification */
#define T_IDENTIFY	"\033[c"	/* [c	Identify terminal 					*/ 
#define T_IDENTIFY_SZ	3

/* Editing */
#define T_INSBLANK	"\033[1@"	/* [1@	Insert a blank char pos which shifts line to the right 	*/
#define T_INSBLANK_SZ	4
#define T_DELCHAR	"\033[1P"	/* [1P	Delete a char pos which shifts line to the left 	*/
#define T_DELCHAR_SZ	4
#define T_BLANKLINE	"\033[1L"	/* [1L	Insert a blank line at cur row which shifts screen down */
#define T_BLANKLINE_SZ	4
#define T_DELLINESCRUP	"\033[1M"	/* [1M	Delete cur line and shift the screen up. 		*/
#define T_DELLINESCRUP_SZ 4

/* Insertion mode */
#define T_INSERTSET	"\033[4h"	/* [4h	Insert mode set and new chars push the old right 	*/
#define T_INSERTSET_SZ	4
#define T_INSERTRESET	"\033[4l"	/* [4l	Insert mode reset and all chars are replaced with new 	*/
#define T_INSERTRESET_SZ 4

/* Printer toggling */
#define T_PRNTSCR	"\033[0i"	/* [0i	Print screen 			*/
#define T_PRNTSCR_SZ	4
#define T_DATA2PRNT	"\033[4i"	/* [4i	Data to printer not screen 	*/
#define T_DATA2PRNT_SZ	4
#define T_DATA2SCR	"\033[5i"	/* [5i	Data to screen not printer 	*/
#define T_DATA2SCR_SZ	4 

/* unused sequences */
/* [?1;0c	VT100 with memory for 24 by 80, inverse video char attribute 	*/
/* [?1;2c	VT100 capable of 132 col mode, with bold+blink+underline+inverse */ 
/* [?6c	 	VT102 (printer port, 132 col mode, and ins/del) 	*/ 


/* Mouse hits (must have a mouse enabled user daemon like gpm) */
#define M_MOUSECATCH	"\033[?9h"	/* [?9h 	start catching mouse 		*/
#define M_MOUSECATCH_SZ	5
#define M_MOUSERELEA	"\033[?9l"	/* [?9l 	disable mouse catching 		*/
#define M_MOUSERELEA_SZ 5
#define M_CATCH		"\033[?1000h"	/* [?1000h 	true mouse catching mode 	*/
#define M_CATCH_SZ	8
#define M_RELEASE	"\033[?1000l"	/* \033[?1000l 	release true mouse catching mode*/
#define M_RELEASE_SZ	8
#define M_CAPABLE	"\033[?1;2c"	/* [ ? 1 ; 2 c 	Report terminal capability */

/* Keystrokes */
#define K_CTRLENN 	016
#define K_CTRLEXX	030
#define K_BACKSPACE 	0177
#define K_ESCAPE 	033 

/* Misc */
#define	K_PF1P		"\033OP"	/* OP		PF1 key sends ESC O P 	*/
#define	K_PF2Q		"\033OQ"	/* OQ		PF2 key sends ESC O Q 	*/
#define K_PF3R		"\033OR"	/* OR		PF3 key sends ESC O R 	*/
#define	k_PF3S		"\033OS"	/* OS		PF3 key sends ESC O S 	*/ 
#define K_NUMPADAPPMODE	"\033 ="	/* ESC =	Set numpad applications mode 	*/
#define K_NUMPADNUMMODE	"\033 >"	/* ESC >	Set numpad numbers mode 	*/

/* Dumb terminal */
#define	K_DUMBUP	"\033OA"	/* OA		Up cursor 	*/
#define	K_DUMBDOWN	"\033OB"	/* OB		Down cursor 	*/
#define K_DUMBRIGHT	"\033OC"	/* OC		Right cursor 	*/
#define K_DUMBLEFT	"\033OD"	/* OD		Left cursor 	*/
#define K_DUMBENTER	"\033OM"	/* OM		enter  		*/

/* Keypad */
#define K_KEYPAD_COMMA	"\033O1"	/* Ol		',' keypad 	*/
#define K_KEYPAD_SLASH	"\033Om"	/* Om		'-' keypad 	*/
#define K_KEYPAD_0	"\033Op"	/* Op		'0' keypad  	*/
#define K_KEYPAD_1	"\033Oq"	/* Oq		'1' keypad  	*/
#define K_KEYPAD_2	"\033Or"	/* Or		'2' keypad 	*/
#define K_KEYPAD_3	"\033Os"	/* Os		'3' keypad 	*/
#define K_KEYPAD_4	"\033Ot"	/* Ot		'4' keypad 	*/
#define K_KEYPAD_5	"\033Ou"	/* Ou		'5' keypad 	*/
#define K_KEYPAD_6	"\033Ov"	/* Ov		'6' keypad 	*/
#define K_KEYPAD_7	"\033Ow"	/* Ow		'7' keypad 	*/
#define K_KEYPAD_8	"\033Ox"	/* Ox		'8' keypad 	*/
#define K_KEYPAD_9	"\033Oy"	/* Oy		'9' keypad 	*/

/* -------------------------- */

/*----------------------------------------------------------------------------*/
#define WHITESPACE "                                                           \
                                                                               \
                                                                               \
                                                                               \
                                                                               \
                                                                               \
                                                                               \
                                                                               \
                                                                               \
                                                                               \
                                                                               \
                                                                               \
                                                                               \
                                                                               \
                                                                               \
                                                                               \
                                                                               \
                                                                               \
                                                                               \
                                                                               \
                                                                               \
                                                                               \
                                                                               "
/*----------------------------------------------------------------------------*/
