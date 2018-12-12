/*
 * *****************************************************************************
 *
 * Copyright 2018 Gavin D. Howard
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * *****************************************************************************
 *
 * Adapted from the following:
 *
 * linenoise.c -- guerrilla line editing library against the idea that a
 * line editing lib needs to be 20,000 lines of C code.
 *
 * You can find the original source code at:
 *   http://github.com/antirez/linenoise
 *
 * You can find the fork that this code is based on at:
 *   https://github.com/rain-1/linenoise-mob
 *
 * ------------------------------------------------------------------------
 *
 * This code is also under the following license:
 *
 * Copyright (c) 2010-2016, Salvatore Sanfilippo <antirez at gmail dot com>
 * Copyright (c) 2010-2013, Pieter Noordhuis <pcnoordhuis at gmail dot com>
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *  *  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 *  *  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * ------------------------------------------------------------------------
 *
 * Does a number of crazy assumptions that happen to be true in 99.9999% of
 * the 2010 UNIX computers around.
 *
 * References:
 * - http://invisible-island.net/xterm/ctlseqs/ctlseqs.html
 * - http://www.3waylabs.com/nw/WWW/products/wizcon/vt220.html
 *
 * Todo list:
 * - Filter bogus Ctrl+<char> combinations.
 * - Win32 support
 *
 * Bloat:
 * - History search like Ctrl+r in readline?
 *
 * List of escape sequences used by this program, we do everything just
 * with three sequences. In order to be so cheap we may have some
 * flickering effect with some slow terminal, but the lesser sequences
 * the more compatible.
 *
 * EL (Erase Line)
 *    Sequence: ESC [ n K
 *    Effect: if n is 0 or missing, clear from cursor to end of line
 *    Effect: if n is 1, clear from beginning of line to cursor
 *    Effect: if n is 2, clear entire line
 *
 * CUF (CUrsor Forward)
 *    Sequence: ESC [ n C
 *    Effect: moves cursor forward n chars
 *
 * CUB (CUrsor Backward)
 *    Sequence: ESC [ n D
 *    Effect: moves cursor backward n chars
 *
 * The following is used to get the terminal width if getting
 * the width with the TIOCGWINSZ ioctl fails
 *
 * DSR (Device Status Report)
 *    Sequence: ESC [ 6 n
 *    Effect: reports the current cusor position as ESC [ n ; m R
 *            where n is the row and m is the column
 *
 * When multi line mode is enabled, we also use two additional escape
 * sequences. However multi line editing is disabled by default.
 *
 * CUU (CUrsor Up)
 *    Sequence: ESC [ n A
 *    Effect: moves cursor up of n chars.
 *
 * CUD (CUrsor Down)
 *    Sequence: ESC [ n B
 *    Effect: moves cursor down of n chars.
 *
 * When linenoiseClearScreen() is called, two additional escape sequences
 * are used in order to clear the screen and position the cursor at home
 * position.
 *
 * CUP (CUrsor Position)
 *    Sequence: ESC [ H
 *    Effect: moves the cursor to upper left corner
 *
 * ED (Erase Display)
 *    Sequence: ESC [ 2 J
 *    Effect: clear the whole screen
 *
 * *****************************************************************************
 *
 * Code for line history.
 *
 */

#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#include <vector.h>
#include <history.h>

#if BC_ENABLE_HISTORY

static struct termios orig_termios; /* In order to restore at exit.*/
static int rawmode = 0; /* For atexit() function to check if restore is needed*/
static int atexit_registered = 0; /* Register atexit just 1 time. */
static int history_max_len = BC_HISTORY_DEF_MAX_LEN;
static int history_len = 0;
static char **history = NULL;

static void linenoiseAtExit(void);
int linenoiseHistoryAdd(const char *line);
static void refreshLine(struct linenoiseState *l);

/* Check if the code is a wide character
 */
static int isWideChar(unsigned long cp) {
    size_t i;
    for (i = 0; i < bc_history_wchars_len; i++) {
        /* ranges are listed in ascending order.  Therefore, once the
         * whole range is higher than the codepoint we're testing, the
         * codepoint won't be found in any remaining range => bail early. */
        if(bc_history_wchars[i][0] > cp) return 0;

        /* test this range */
        if (bc_history_wchars[i][0] <= cp && cp <= bc_history_wchars[i][1]) return 1;
    }
    return 0;
}

/* Check if the code is a combining character
 */
static int isCombiningChar(unsigned long cp) {
    size_t i;
    for (i = 0; i < bc_history_combo_chars_len; i++) {
        /* combining chars are listed in ascending order, so once we pass
         * the codepoint of interest, we know it's not a combining char. */
        if(bc_history_combo_chars[i] > cp) return 0;
        if (bc_history_combo_chars[i] == cp) return 1;
    }
    return 0;
}

/* Get length of previous UTF8 character
 */
static size_t prevUtf8CharLen(const char* buf, int pos) {
    int end = pos--;
    while (pos >= 0 && ((unsigned char)buf[pos] & 0xC0) == 0x80)
        pos--;
    return end - pos;
}

/* Convert UTF8 to Unicode code point
 */
static size_t utf8BytesToCodePoint(const char* buf, size_t len, int* cp) {
    if (len) {
        unsigned char byte = buf[0];
        if ((byte & 0x80) == 0) {
            *cp = byte;
            return 1;
        } else if ((byte & 0xE0) == 0xC0) {
            if (len >= 2) {
                *cp = (((unsigned long)(buf[0] & 0x1F)) << 6) |
                       ((unsigned long)(buf[1] & 0x3F));
                return 2;
            }
        } else if ((byte & 0xF0) == 0xE0) {
            if (len >= 3) {
                *cp = (((unsigned long)(buf[0] & 0x0F)) << 12) |
                      (((unsigned long)(buf[1] & 0x3F)) << 6) |
                       ((unsigned long)(buf[2] & 0x3F));
                return 3;
            }
        } else if ((byte & 0xF8) == 0xF0) {
            if (len >= 4) {
                *cp = (((unsigned long)(buf[0] & 0x07)) << 18) |
                      (((unsigned long)(buf[1] & 0x3F)) << 12) |
                      (((unsigned long)(buf[2] & 0x3F)) << 6) |
                       ((unsigned long)(buf[3] & 0x3F));
                return 4;
            }
        }
        else {
            *cp = 0xFFFD;
            return 1;
        }
    }
    *cp = 0;
    return 1;
}

/* Get length of next grapheme
 */
size_t linenoiseUtf8NextCharLen(const char* buf, size_t buf_len, size_t pos, size_t *col_len) {
    size_t beg = pos;
    int cp;
    size_t len = utf8BytesToCodePoint(buf + pos, buf_len - pos, &cp);
    if (isCombiningChar(cp)) {
        /* NOTREACHED */
        return 0;
    }
    if (col_len != NULL) *col_len = isWideChar(cp) ? 2 : 1;
    pos += len;
    while (pos < buf_len) {
        int cp;
        len = utf8BytesToCodePoint(buf + pos, buf_len - pos, &cp);
        if (!isCombiningChar(cp)) return pos - beg;
        pos += len;
    }
    return pos - beg;
}

/* Get length of previous grapheme
 */
size_t linenoiseUtf8PrevCharLen(const char* buf, size_t buf_len, size_t pos, size_t *col_len) {
    (void) (buf_len);
    size_t end = pos;
    while (pos > 0) {
        size_t len = prevUtf8CharLen(buf, pos);
        pos -= len;
        int cp;
        utf8BytesToCodePoint(buf + pos, len, &cp);
        if (!isCombiningChar(cp)) {
            if (col_len != NULL) *col_len = isWideChar(cp) ? 2 : 1;
            return end - pos;
        }
    }
    /* NOTREACHED */
    return 0;
}

/* Read a Unicode from file.
 */
size_t linenoiseUtf8ReadCode(int fd, char* buf, size_t buf_len, int* cp) {
    if (buf_len < 1) return -1;
    size_t nread = read(fd,&buf[0],1);
    if (nread <= 0) return nread;

    unsigned char byte = buf[0];
    if ((byte & 0x80) == 0) {
        ;
    } else if ((byte & 0xE0) == 0xC0) {
        if (buf_len < 2) return -1;
        nread = read(fd,&buf[1],1);
        if (nread <= 0) return nread;
    } else if ((byte & 0xF0) == 0xE0) {
        if (buf_len < 3) return -1;
        nread = read(fd,&buf[1],2);
        if (nread <= 0) return nread;
    } else if ((byte & 0xF8) == 0xF0) {
        if (buf_len < 3) return -1;
        nread = read(fd,&buf[1],3);
        if (nread <= 0) return nread;
    } else {
        return -1;
    }

    return utf8BytesToCodePoint(buf, buf_len, cp);
}

/* ========================== Encoding functions ============================= */

/* Get column length from begining of buffer to current byte position */
static size_t columnPos(const char *buf, size_t buf_len, size_t pos) {
    size_t ret = 0;
    size_t off = 0;
    while (off < pos) {
        size_t col_len;
        size_t len = linenoiseUtf8NextCharLen(buf,buf_len,off,&col_len);
        off += len;
        ret += col_len;
    }
    return ret;
}

/* ======================= Low level terminal handling ====================== */

/* Return true if the terminal name is in the list of terminals we know are
 * not able to understand basic escape sequences. */
static int isUnsupportedTerm(void) {
    char *term = getenv("TERM");
    int j;

    if (term == NULL) return 0;
    for (j = 0; bc_history_bad_terms[j]; j++)
        if (!strcasecmp(term, bc_history_bad_terms[j])) return 1;
    return 0;
}

/* Raw mode: 1960 magic ***. */
static int enableRawMode(int fd) {
    struct termios raw;

    if (!isatty(STDIN_FILENO)) goto fatal;
    if (!atexit_registered) {
        atexit(linenoiseAtExit);
        atexit_registered = 1;
    }
    if (tcgetattr(fd,&orig_termios) == -1) goto fatal;

    raw = orig_termios;  /* modify the original mode */
    /* input modes: no break, no CR to NL, no parity check, no strip char,
     * no start/stop output control. */
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    /* control modes - set 8 bit chars */
    raw.c_cflag |= (CS8);
    /* local modes - choing off, canonical off, no extended functions,
     * no signal chars (^Z,^C) */
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    /* control chars - set return condition: min number of bytes and timer.
     * We want read to return every single byte, without timeout. */
    raw.c_cc[VMIN] = 1; raw.c_cc[VTIME] = 0; /* 1 byte, no timer */

    /* put terminal in raw mode after flushing */
    if (tcsetattr(fd,TCSAFLUSH,&raw) < 0) goto fatal;
    rawmode = 1;
    return 0;

fatal:
    errno = ENOTTY;
    return -1;
}

static void disableRawMode(int fd) {
    /* Don't even check the return value as it's too late. */
    if (rawmode && tcsetattr(fd,TCSAFLUSH,&orig_termios) != -1)
        rawmode = 0;
}

/* Use the ESC [6n escape sequence to query the horizontal cursor position
 * and return it. On error -1 is returned, on success the position of the
 * cursor. */
static int getCursorPosition(int ifd, int ofd) {
    char buf[32];
    int cols, rows;
    unsigned int i = 0;

    /* Report cursor location */
    if (write(ofd, "\x1b[6n", 4) != 4) return -1;

    /* Read the response: ESC [ rows ; cols R */
    while (i < sizeof(buf)-1) {
        if (read(ifd,buf+i,1) != 1) break;
        if (buf[i] == 'R') break;
        i++;
    }
    buf[i] = '\0';

    /* Parse it. */
    if (buf[0] != BC_ACTION_ESC || buf[1] != '[') return -1;
    if (sscanf(buf+2,"%d;%d",&rows,&cols) != 2) return -1;
    return cols;
}

/* Try to get the number of columns in the current terminal, or assume 80
 * if it fails. */
static int getColumns(int ifd, int ofd) {
    struct winsize ws;

    if (ioctl(1, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        /* ioctl() failed. Try to query the terminal itself. */
        int start, cols;

        /* Get the initial position so we can restore it later. */
        start = getCursorPosition(ifd,ofd);
        if (start == -1) goto failed;

        /* Go to right margin and get position. */
        if (write(ofd,"\x1b[999C",6) != 6) goto failed;
        cols = getCursorPosition(ifd,ofd);
        if (cols == -1) goto failed;

        /* Restore position. */
        if (cols > start) {
            char seq[32];
            snprintf(seq,32,"\x1b[%dD",cols-start);
            if (write(ofd,seq,strlen(seq)) == -1) {
                /* Can't recover... */
            }
        }
        return cols;
    } else {
        return ws.ws_col;
    }

failed:
    return 80;
}

/* Get the length of the string ignoring escape-sequences */
// TODO merge with columnPos
static int strlenPerceived(const char* str) {
	int len = 0;
	if (str) {
		int escaping = 0;
		while(*str) {
			if (escaping) { /* was terminating char reached? */
				if(*str >= 0x40 && *str <= 0x7E)
					escaping = 0;
			}
			else if(*str == '\x1b') {
				escaping = 1;
				if (str[1] == '[') str++;
			}
			else {
				len++;
			}
			str++;
		}
	}
	return len;
}

/* Clear the screen. Used to handle ctrl+l */
void linenoiseClearScreen(void) {
    int fd;

    fd = isatty(STDOUT_FILENO) ? STDOUT_FILENO : STDERR_FILENO;
    if (write(fd,"\x1b[H\x1b[2J",7) <= 0) {
        /* nothing to do, just to avoid warning. */
    }
}


/* =========================== Line editing ================================= */


/* Check if text is an ANSI escape sequence
 */
static int isAnsiEscape(const char *buf, size_t buf_len, size_t* len) {
    if (buf_len > 2 && !memcmp("\033[", buf, 2)) {
        size_t off = 2;
        while (off < buf_len) {
            switch (buf[off++]) {
            case 'A': case 'B': case 'C': case 'D': case 'E':
            case 'F': case 'G': case 'H': case 'J': case 'K':
            case 'S': case 'T': case 'f': case 'm':
                *len = off;
                return 1;
            }
        }
    }
    return 0;
}

/* Get column length of prompt text
 */
static size_t promptTextColumnLen(const char *prompt, size_t plen) {
    char buf[BC_HISTORY_MAX_LINE];
    size_t buf_len = 0;
    size_t off = 0;
    while (off < plen) {
        size_t len;
        if (isAnsiEscape(prompt + off, plen - off, &len)) {
            off += len;
            continue;
        }
        buf[buf_len++] = prompt[off++];
    }
    return columnPos(buf,buf_len,buf_len);
}

/* Single line low level line refresh.
 *
 * Rewrite the currently edited line accordingly to the buffer content,
 * cursor position, and number of columns of the terminal. */
static void refreshSingleLine(struct linenoiseState *l) {
    char seq[64];
    size_t pcollen = promptTextColumnLen(l->prompt,strlen(l->prompt));
    int fd = l->ofd;
    char *buf = l->buf;
    size_t len = l->len;
    size_t pos = l->pos;
    BcVec vec;

    while((pcollen+columnPos(buf,len,pos)) >= l->cols) {
        int chlen = linenoiseUtf8NextCharLen(buf,len,0,NULL);
        buf += chlen;
        len -= chlen;
        pos -= chlen;
    }
    while (pcollen+columnPos(buf,len,len) > l->cols) {
        len -= linenoiseUtf8PrevCharLen(buf,len,len,NULL);
    }

    bc_vec_init(&vec, sizeof(char), NULL);
    /* Cursor to left edge */
    snprintf(seq,64,"\r");
	bc_vec_string(&vec, strlen(seq), seq);
    /* Write the prompt and the current buffer content */
	bc_vec_concat(&vec, l->prompt);
	bc_vec_concat(&vec, buf);
    /* Erase to right */
    snprintf(seq,64,"\x1b[0K");
	bc_vec_concat(&vec, seq);
    /* Move cursor to original position. */
//    snprintf(seq,64,"\r\x1b[%dC", (int)(pos+strlenPerceived(l->prompt)));
    snprintf(seq,64,"\r\x1b[%dC", (int)(columnPos(buf,len,pos)+pcollen));
	bc_vec_concat(&vec, seq);
    if (write(fd, vec.v, vec.len - 1) == -1) {} /* Can't recover from write error. */
    bc_vec_free(&vec);
}

/* Calls the two low level functions refreshSingleLine() or
 * refreshMultiLine() according to the selected mode. */
static void refreshLine(struct linenoiseState *l) {
    refreshSingleLine(l);
}

/* Insert the character 'c' at cursor current position.
 *
 * On error writing to the terminal -1 is returned, otherwise 0. */
int linenoiseEditInsert(struct linenoiseState *l, const char *cbuf, int clen) {
    if (l->len+clen <= l->buflen) {
        if (l->len == l->pos) {
            memcpy(&l->buf[l->pos],cbuf,clen);
            l->pos+=clen;
            l->len+=clen;;
            l->buf[l->len] = '\0';
            if ((!promptTextColumnLen(l->prompt,l->plen)+columnPos(l->buf,l->len,l->len) < l->cols)) {
                /* Avoid a full update of the line in the
                 * trivial case. */
                if (write(l->ofd,cbuf,clen) == -1) return -1;
            } else {
                refreshLine(l);
            }
        } else {
            memmove(l->buf+l->pos+clen,l->buf+l->pos,l->len-l->pos);
            memcpy(&l->buf[l->pos],cbuf,clen);
            l->pos+=clen;
            l->len+=clen;
            l->buf[l->len] = '\0';
            refreshLine(l);
        }
    }
    return 0;
}

/* Move cursor on the left. */
void linenoiseEditMoveLeft(struct linenoiseState *l) {
    if (l->pos > 0) {
        l->pos -= linenoiseUtf8PrevCharLen(l->buf,l->len,l->pos,NULL);
        refreshLine(l);
    }
}

/* Move cursor on the right. */
void linenoiseEditMoveRight(struct linenoiseState *l) {
    if (l->pos != l->len) {
        l->pos += linenoiseUtf8NextCharLen(l->buf,l->len,l->pos,NULL);
        refreshLine(l);
    }
}

/* Move cursor to the end of the current word. */
void linenoiseEditMoveWordEnd(struct linenoiseState *l) {
    if (l->len == 0 || l->pos >= l->len) return;
    if (l->buf[l->pos] == ' ')
        while (l->pos < l->len && l->buf[l->pos] == ' ') ++l->pos;
    while (l->pos < l->len && l->buf[l->pos] != ' ') ++l->pos;
    refreshLine(l);
}

/* Move cursor to the start of the current word. */
void linenoiseEditMoveWordStart(struct linenoiseState *l) {
    if (l->len == 0) return;
    if (l->buf[l->pos-1] == ' ') --l->pos;
    if (l->buf[l->pos] == ' ')
        while (l->pos > 0 && l->buf[l->pos] == ' ') --l->pos;
    while (l->pos > 0 && l->buf[l->pos-1] != ' ') --l->pos;
    refreshLine(l);
}

/* Move cursor to the start of the line. */
void linenoiseEditMoveHome(struct linenoiseState *l) {
    if (l->pos != 0) {
        l->pos = 0;
        refreshLine(l);
    }
}

/* Move cursor to the end of the line. */
void linenoiseEditMoveEnd(struct linenoiseState *l) {
    if (l->pos != l->len) {
        l->pos = l->len;
        refreshLine(l);
    }
}

/* Substitute the currently edited line with the next or previous history
 * entry as specified by 'dir'. */
#define LINENOISE_HISTORY_NEXT 0
#define LINENOISE_HISTORY_PREV 1
void linenoiseEditHistoryNext(struct linenoiseState *l, int dir) {
    if (history_len > 1) {
        /* Update the current history entry before to
         * overwrite it with the next one. */
        free(history[history_len - 1 - l->history_index]);
        history[history_len - 1 - l->history_index] = strdup(l->buf);
        /* Show the new entry */
        l->history_index += (dir == LINENOISE_HISTORY_PREV) ? 1 : -1;
        if (l->history_index < 0) {
            l->history_index = 0;
            return;
        } else if (l->history_index >= history_len) {
            l->history_index = history_len-1;
            return;
        }
        strncpy(l->buf,history[history_len - 1 - l->history_index],l->buflen);
        l->buf[l->buflen-1] = '\0';
        l->len = l->pos = strlen(l->buf);
        refreshLine(l);
    }
}

/* Delete the character at the right of the cursor without altering the cursor
 * position. Basically this is what happens with the "Delete" keyboard key. */
void linenoiseEditDelete(struct linenoiseState *l) {
    if (l->len > 0 && l->pos < l->len) {
        int chlen = linenoiseUtf8NextCharLen(l->buf,l->len,l->pos,NULL);
        memmove(l->buf+l->pos,l->buf+l->pos+chlen,l->len-l->pos-chlen);
        l->len-=chlen;
        l->buf[l->len] = '\0';
        refreshLine(l);
    }
}

/* Backspace implementation. */
void linenoiseEditBackspace(struct linenoiseState *l) {
    if (l->pos > 0 && l->len > 0) {
        int chlen = linenoiseUtf8PrevCharLen(l->buf,l->len,l->pos,NULL);
        memmove(l->buf+l->pos-chlen,l->buf+l->pos,l->len-l->pos);
        l->pos-=chlen;
        l->len-=chlen;
        l->buf[l->len] = '\0';
        refreshLine(l);
    }
}

/* Delete the previous word, maintaining the cursor at the start of the
 * current word. */
void linenoiseEditDeletePrevWord(struct linenoiseState *l) {
    size_t old_pos = l->pos;
    size_t diff;

    while (l->pos > 0 && l->buf[l->pos-1] == ' ')
        l->pos--;
    while (l->pos > 0 && l->buf[l->pos-1] != ' ')
        l->pos--;
    diff = old_pos - l->pos;
    memmove(l->buf+l->pos,l->buf+old_pos,l->len-old_pos+1);
    l->len -= diff;
    refreshLine(l);
}

/* Delete the next word, maintaining the cursor at the same position */
void linenoiseEditDeleteNextWord(struct linenoiseState *l) {
    size_t next_word_end = l->pos;
    while (next_word_end < l->len && l->buf[next_word_end] == ' ') ++next_word_end;
    while (next_word_end < l->len && l->buf[next_word_end] != ' ') ++next_word_end;
    memmove(l->buf+l->pos, l->buf+next_word_end, l->len-next_word_end);
    l->len -= next_word_end - l->pos;
    refreshLine(l);
}

/* This function is the core of the line editing capability of linenoise.
 * It expects 'fd' to be already in "raw mode" so that every key pressed
 * will be returned ASAP to read().
 *
 * The resulting string is put into 'buf' when the user type enter, or
 * when ctrl+d is typed.
 *
 * The function returns the length of the current buffer. */
static int linenoiseEdit(int stdin_fd, int stdout_fd, char *buf, size_t buflen, const char *prompt)
{
    struct linenoiseState l;

    /* Populate the linenoise state that we pass to functions implementing
     * specific editing functionalities. */
    l.ifd = stdin_fd;
    l.ofd = stdout_fd;
    l.buf = buf;
    l.buflen = buflen;
    l.prompt = prompt;
    l.plen = strlen(prompt);
    l.oldcolpos = l.pos = 0;
    l.len = 0;
    l.cols = getColumns(stdin_fd, stdout_fd);
    l.history_index = 0;

    /* Buffer starts empty. */
    l.buf[0] = '\0';
    l.buflen--; /* Make sure there is always space for the nulterm */

    /* The latest history entry is always our current buffer, that
     * initially is just an empty string. */
    linenoiseHistoryAdd("");

    if (write(l.ofd,prompt,l.plen) == -1) return -1;
    while(1) {
        int c;
        char cbuf[32]; // large enough for any encoding?
        int nread;
        char seq[3];

	/* Continue reading if interrupted by a signal */
// TODO
//	do {
//          nread = read(l.ifd,&c,1);
//        } while((nread == -1) && (errno == EINTR));
        nread = linenoiseUtf8ReadCode(l.ifd,cbuf,sizeof(cbuf), &c);
        if (nread <= 0) return l.len;

        switch(c) {
        case BC_ACTION_LINE_FEED:/* line feed */
        case BC_ACTION_ENTER:    /* enter */
            history_len--;
            free(history[history_len]);
            return (int)l.len;
        case BC_ACTION_CTRL_C:     /* ctrl-c */
            errno = EAGAIN;
            return -1;
        case BC_ACTION_BACKSPACE:   /* backspace */
        case 8:     /* ctrl-h */
            linenoiseEditBackspace(&l);
            break;
        case BC_ACTION_CTRL_D:     /* ctrl-d, remove char at right of cursor, or if the
                            line is empty, act as end-of-file. */
            if (l.len > 0) {
                linenoiseEditDelete(&l);
            } else {
                history_len--;
                free(history[history_len]);
                return -1;
            }
            break;
        case BC_ACTION_CTRL_T:    /* ctrl-t, swaps current character with previous. */
            {
              int pcl, ncl;
              char auxb[5];

 	      pcl = linenoiseUtf8PrevCharLen(l.buf,l.len,l.pos,NULL);
	      ncl = linenoiseUtf8NextCharLen(l.buf,l.len,l.pos,NULL);
//            printf("[%d %d %d]\n", pcl, l.pos, ncl);
	      // to perform a swap we need
              // * nonzero char length to the left
              // * not at the end of the line
              if(pcl != 0 && l.pos != l.len && pcl < 5 && ncl < 5) {
		// the actual transpose works like this
		//
		//           ,--- l.pos
		//          v
		// xxx [AAA] [BB] xxx
		// xxx [BB] [AAA] xxx
		memcpy(auxb, l.buf+l.pos-pcl, pcl);
		memcpy(l.buf+l.pos-pcl, l.buf+l.pos, ncl);
		memcpy(l.buf+l.pos-pcl+ncl, auxb, pcl);
		l.pos += -pcl+ncl;
		refreshLine(&l);
              }
            }
            break;
        case BC_ACTION_CTRL_B:     /* ctrl-b */
            linenoiseEditMoveLeft(&l);
            break;
        case BC_ACTION_CTRL_F:     /* ctrl-f */
            linenoiseEditMoveRight(&l);
            break;
        case BC_ACTION_CTRL_P:    /* ctrl-p */
            linenoiseEditHistoryNext(&l, LINENOISE_HISTORY_PREV);
            break;
        case BC_ACTION_CTRL_N:    /* ctrl-n */
            linenoiseEditHistoryNext(&l, LINENOISE_HISTORY_NEXT);
            break;
        case BC_ACTION_ESC:    /* escape sequence */
            if (read(l.ifd,seq,1) == -1) break;
            /* ESC ? sequences */
            if (seq[0] != '[' && seq[0] != '0') {
                switch (seq[0]) {
                case 'f':
                    linenoiseEditMoveWordEnd(&l);
                    break;
                case 'b':
                    linenoiseEditMoveWordStart(&l);
                    break;
                case 'd':
                    linenoiseEditDeleteNextWord(&l);
                    break;
                }
            } else {
                if (read(l.ifd,seq+1,1) == -1) break;
                /* ESC [ sequences. */
                if (seq[0] == '[') {
                    if (seq[1] >= '0' && seq[1] <= '9') {
                        /* Extended escape, read additional byte. */
                        if (read(l.ifd,seq+2,1) == -1) break;
                        if (seq[2] == '~') {
                            switch(seq[1]) {
                            case '3': /* Delete key. */
                                linenoiseEditDelete(&l);
                                break;
                            }
                        }
                    } else {
                        switch(seq[1]) {
                        case 'A': /* Up */
                            linenoiseEditHistoryNext(&l, LINENOISE_HISTORY_PREV);
                            break;
                        case 'B': /* Down */
                            linenoiseEditHistoryNext(&l, LINENOISE_HISTORY_NEXT);
                            break;
                        case 'C': /* Right */
                            linenoiseEditMoveRight(&l);
                            break;
                        case 'D': /* Left */
                            linenoiseEditMoveLeft(&l);
                            break;
                        case 'H': /* Home */
                            linenoiseEditMoveHome(&l);
                            break;
                        case 'F': /* End*/
                            linenoiseEditMoveEnd(&l);
                            break;
                        case 'd': /* End*/
                            linenoiseEditDeleteNextWord(&l);
                            break;
                        case '1': /* Home */
                            linenoiseEditMoveHome(&l);
                            break;
                        case '4': /* End */
                            linenoiseEditMoveEnd(&l);
                            break;
                        }
                    }
                }
                /* ESC O sequences. */
                else if (seq[0] == 'O') {
                    switch(seq[1]) {
                    case 'H': /* Home */
                        linenoiseEditMoveHome(&l);
                        break;
                    case 'F': /* End*/
                        linenoiseEditMoveEnd(&l);
                        break;
                    }
                }
            }
            break;
        default:
            if (linenoiseEditInsert(&l,cbuf,nread)) return -1;
            break;
        case BC_ACTION_CTRL_U: /* Ctrl+u, delete the whole line. */
            buf[0] = '\0';
            l.pos = l.len = 0;
            refreshLine(&l);
            break;
        case BC_ACTION_CTRL_K: /* Ctrl+k, delete from current to end of line. */
            buf[l.pos] = '\0';
            l.len = l.pos;
            refreshLine(&l);
            break;
        case BC_ACTION_CTRL_A: /* Ctrl+a, go to the start of the line */
            linenoiseEditMoveHome(&l);
            break;
        case BC_ACTION_CTRL_E: /* ctrl+e, go to the end of the line */
            linenoiseEditMoveEnd(&l);
            break;
        case BC_ACTION_CTRL_L: /* ctrl+l, clear screen */
            linenoiseClearScreen();
            refreshLine(&l);
            break;
        case BC_ACTION_CTRL_W: /* ctrl+w, delete previous word */
            linenoiseEditDeletePrevWord(&l);
            break;
        }
    }
    return l.len;
}

/* This special mode is used by linenoise in order to print scan codes
 * on screen for debugging / development purposes. It is implemented
 * by the linenoise_example program using the --keycodes option. */
#ifndef NDEBUG
void linenoisePrintKeyCodes() {
    char quit[4];

    printf("Linenoise key codes debugging mode.\n"
            "Press keys to see scan codes. Type 'quit' at any time to exit.\n");
    if (enableRawMode(STDIN_FILENO) == -1) return;
    memset(quit,' ',4);
    while(1) {
        char c;
        int nread;

        nread = read(STDIN_FILENO,&c,1);
        if (nread <= 0) continue;
        memmove(quit,quit+1,sizeof(quit)-1); /* shift string to left. */
        quit[sizeof(quit)-1] = c; /* Insert current char on the right. */
        if (memcmp(quit,"quit",sizeof(quit)) == 0) break;

        printf("'%c' %02x (%d) (type quit to exit)\n",
            isprint((int)c) ? c : '?', (int)c, (int)c);
        printf("\r"); /* Go left edge manually, we are in raw mode. */
        fflush(stdout);
    }
    disableRawMode(STDIN_FILENO);
}
#endif // NDEBUG

/* This function calls the line editing function linenoiseEdit() using
 * the STDIN file descriptor set in raw mode. */
static int linenoiseRaw(char *buf, FILE *out, size_t buflen, const char *prompt) {
    int outfd, count;

    if (buflen == 0) {
        errno = EINVAL;
        return -1;
    }

    if ((outfd = fileno(out)) == -1)
        return -1;

    if (enableRawMode(STDIN_FILENO) == -1) return -1;
    count = linenoiseEdit(STDIN_FILENO, outfd, buf, buflen, prompt);
    disableRawMode(STDIN_FILENO);
    fprintf(out, "\n");
    return count;
}

/* This function is called when linenoise() is called with the standard
 * input file descriptor not attached to a TTY. So for example when the
 * program using linenoise is called in pipe or with a file redirected
 * to its standard input. In this case, we want to be able to return the
 * line regardless of its length (by default we are limited to 4k). */
static char *linenoiseNoTTY(void) {
    char *line = NULL;
    size_t len = 0, maxlen = 0;

    while(1) {
    	int c;
        if (len == maxlen) {
            char *oldval = line;
            if (maxlen == 0) maxlen = 16;
            maxlen *= 2;
            line = realloc(line,maxlen);
            if (line == NULL) {
                if (oldval) free(oldval);
                return NULL;
            }
        }
        c = fgetc(stdin);
        if (c == EOF || c == '\n') {
            if (c == EOF && len == 0) {
                free(line);
                return NULL;
            } else {
                line[len] = '\0';
                return line;
            }
        } else {
            line[len] = c;
            len++;
        }
    }
}

/* The high level function that is the main API of the linenoise library.
 * This function checks if the terminal has basic capabilities, just checking
 * for a blacklist of stupid terminals, and later either calls the line
 * editing function or uses dummy fgets() so that you will be able to type
 * something even in the most desperate of the conditions. */
char *linenoise(const char *prompt) {
    char buf[BC_HISTORY_MAX_LINE];
    FILE *stream;
    int count;

    stream = isatty(STDOUT_FILENO) ? stdout : stderr;
    if (!isatty(STDIN_FILENO)) {
        /* Not a tty: read from file / pipe. In this mode we don't want any
         * limit to the line size, so we call a function to handle that. */
        return linenoiseNoTTY();
    } else if (isUnsupportedTerm()) {
        size_t len;

        fprintf(stream, "%s",prompt);
        fflush(stream);
        if (fgets(buf,BC_HISTORY_MAX_LINE,stdin) == NULL) return NULL;
        len = strlen(buf);
        while(len && (buf[len-1] == '\n' || buf[len-1] == '\r')) {
            len--;
            buf[len] = '\0';
        }
        return strdup(buf);
    } else {
        count = linenoiseRaw(buf,stream,BC_HISTORY_MAX_LINE,prompt);
        if (count == -1) return NULL;
        return strdup(buf);
    }
}

/* ================================ History ================================= */

#ifdef VALGRIND
/* Free the history, but does not reset it. Only used when we have to
 * exit() to avoid memory leaks are reported by valgrind & co. */
static void freeHistory(void) {
    if (history) {
        int j;

        for (j = 0; j < history_len; j++)
            free(history[j]);
        free(history);
    }
}
#endif

/* At exit we'll try to fix the terminal to the initial conditions. */
static void linenoiseAtExit(void) {
    disableRawMode(STDIN_FILENO);
#ifdef VALGRIND
    freeHistory();
#endif
}

/* This is the API call to add a new entry in the linenoise history.
 * It uses a fixed array of char pointers that are shifted (memmoved)
 * when the history max length is reached in order to remove the older
 * entry and make room for the new one, so it is not exactly suitable for huge
 * histories, but will work well for a few hundred of entries.
 *
 * Using a circular buffer is smarter, but a bit more complex to handle. */
int linenoiseHistoryAdd(const char *line) {
    char *linecopy;

    if (history_max_len == 0) return 0;

    /* Initialization on first call. */
    if (history == NULL) {
        history = malloc(sizeof(char*)*history_max_len);
        if (history == NULL) return 0;
        memset(history,0,(sizeof(char*)*history_max_len));
    }

    /* Don't add duplicated lines. */
    if (history_len && !strcmp(history[history_len-1], line)) return 0;

    /* Add an heap allocated copy of the line in the history.
     * If we reached the max length, remove the older line. */
    linecopy = strdup(line);
    if (!linecopy) return 0;
    if (history_len == history_max_len) {
        free(history[0]);
        memmove(history,history+1,sizeof(char*)*(history_max_len-1));
        history_len--;
    }
    history[history_len] = linecopy;
    history_len++;
    return 1;
}

/* Set the maximum length for the history. This function can be called even
 * if there is already some history, the function will make sure to retain
 * just the latest 'len' elements if the new history length value is smaller
 * than the amount of items already inside the history. */
int linenoiseHistorySetMaxLen(int len) {
    char **new;

    if (len < 1) return 0;
    if (history) {
        int tocopy = history_len;

        new = malloc(sizeof(char*)*len);
        if (new == NULL) return 0;

        /* If we can't copy everything, free the elements we'll not use. */
        if (len < tocopy) {
            int j;

            for (j = 0; j < tocopy-len; j++) free(history[j]);
            tocopy = len;
        }
        memset(new,0,sizeof(char*)*len);
        memcpy(new,history+(history_len-tocopy), sizeof(char*)*tocopy);
        free(history);
        history = new;
    }
    history_max_len = len;
    if (history_len > history_max_len)
        history_len = history_max_len;
    return 1;
}

int linenoiseHistoryGetMaxLen(void) {
    return history_max_len;
}

/* Copy the history into the specified array.
 * it must already be allocated to have at least destlen spaces.
 * The size of the history is returned. */
int linenoiseHistoryCopy(char** dest, int destlen) {
    for(int i = 0; i < destlen; ++i) {
        if (i >= history_len) break;
        dest[i] = strdup(history[i]);
    }

    return history_len;
}
#endif // BC_ENABLE_HISTORY
