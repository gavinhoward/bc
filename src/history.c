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

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>

#include <termios.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#include <vector.h>
#include <history.h>
#include <vm.h>

#if BC_ENABLE_HISTORY

/**
 * Check if the code is a wide character.
 */
static bool bc_history_wchar(unsigned long cp) {

	size_t i;

	for (i = 0; i < bc_history_wchars_len; i++) {

		// Ranges are listed in ascending order.  Therefore, once the
		// whole range is higher than the codepoint we're testing, the
		// codepoint won't be found in any remaining range => bail early.
		if (bc_history_wchars[i][0] > cp) return false;

		// Test this range.
		if (bc_history_wchars[i][0] <= cp && cp <= bc_history_wchars[i][1])
			return true;
	}

	return false;
}

/**
 * Check if the code is a combining character.
 */
static bool bc_history_comboChar(unsigned long cp) {

	size_t i;

	for (i = 0; i < bc_history_combo_chars_len; i++) {

		// Combining chars are listed in ascending order, so once we pass
		// the codepoint of interest, we know it's not a combining char.
		if(bc_history_combo_chars[i] > cp) return false;
		if (bc_history_combo_chars[i] == cp) return true;
	}

	return false;
}

/**
 * Get length of previous UTF8 character.
 */
static size_t bc_history_prevCharLen(const char *buf, int pos) {

	int end = pos;

	for (pos -= 1; pos >= 0 && ((unsigned char)buf[pos] & 0xC0) == 0x80; --pos);

	return end - pos;
}

/**
 * Convert UTF-8 to Unicode code point.
 */
static size_t bc_history_bytesToCodePoint(const char *buf, size_t len, int *cp)
{
	if (len) {

		unsigned char byte = buf[0];

		if ((byte & 0x80) == 0) {
			*cp = byte;
			return 1;
		}
		else if ((byte & 0xE0) == 0xC0) {

			if (len >= 2) {
				*cp = (((unsigned long)(buf[0] & 0x1F)) << 6) |
					   ((unsigned long)(buf[1] & 0x3F));
				return 2;
			}
		}
		else if ((byte & 0xF0) == 0xE0) {

			if (len >= 3) {
				*cp = (((unsigned long)(buf[0] & 0x0F)) << 12) |
					  (((unsigned long)(buf[1] & 0x3F)) << 6) |
					   ((unsigned long)(buf[2] & 0x3F));
				return 3;
			}
		}
		else if ((byte & 0xF8) == 0xF0) {

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

/**
 * Get length of next grapheme.
 */
size_t bc_history_nextLen(const char *buf, size_t buf_len,
                          size_t pos, size_t *col_len)
{
	int cp;
	size_t beg = pos;
	size_t len = bc_history_bytesToCodePoint(buf + pos, buf_len - pos, &cp);

	if (bc_history_comboChar(cp)) {
		// Currently unreachable?
		return 0;
	}

	if (col_len != NULL) *col_len = bc_history_wchar(cp) ? 2 : 1;

	pos += len;

	while (pos < buf_len) {

		int cp;

		len = bc_history_bytesToCodePoint(buf + pos, buf_len - pos, &cp);

		if (!bc_history_comboChar(cp)) return pos - beg;

		pos += len;
	}

	return pos - beg;
}

/**
 * Get length of previous grapheme.
 */
size_t bc_history_prevLen(const char* buf, size_t buf_len,
                          size_t pos, size_t *col_len)
{
	size_t end = pos;

	(void) (buf_len);

	while (pos > 0) {

		int cp;
		size_t len = bc_history_prevCharLen(buf, pos);

		pos -= len;
		bc_history_bytesToCodePoint(buf + pos, len, &cp);

		if (!bc_history_comboChar(cp)) {
			if (col_len != NULL) *col_len = bc_history_wchar(cp) ? 2 : 1;
			return end - pos;
		}
	}

	// Currently unreachable?
	return 0;
}

/**
 * Read a Unicode code point from a file.
 */
size_t bc_history_readCode(int fd, char *buf, size_t buf_len, int *cp) {

	size_t nread;

	if (buf_len < 1) return -1;
	nread = read(fd, buf, 1);
	if (nread <= 0) return nread;

	unsigned char byte = buf[0];

	if ((byte & 0x80) != 0) {

		if ((byte & 0xE0) == 0xC0) {

			if (buf_len < 2) return -1;
			nread = read(fd, buf + 1, 1);
			if (nread <= 0) return nread;
		}
		else if ((byte & 0xF0) == 0xE0) {

			if (buf_len < 3) return -1;
			nread = read(fd, buf + 1, 2);
			if (nread <= 0) return nread;
		}
		else if ((byte & 0xF8) == 0xF0) {

			if (buf_len < 3) return -1;
			nread = read(fd, buf + 1, 3);
			if (nread <= 0) return nread;
		}
		else {
			return -1;
		}
	}

	return bc_history_bytesToCodePoint(buf, buf_len, cp);
}

/* ========================== Encoding functions ============================= */

/**
 * Get column length from begining of buffer to current byte position.
 */
static size_t bc_history_colPos(const char *buf, size_t buf_len, size_t pos) {

	size_t ret = 0, off = 0;

	while (off < pos) {

		size_t col_len, len;

		len = bc_history_nextLen(buf, buf_len, off, &col_len);

		off += len;
		ret += col_len;
	}

	return ret;
}

/* ======================= Low level terminal handling ====================== */

/**
 * Return true if the terminal name is in the list of terminals we know are
 * not able to understand basic escape sequences.
 */
static bool bc_history_isBadTerm() {

	int i;
	char *term = getenv("TERM");

	if (term == NULL) return false;

	for (i = 0; bc_history_bad_terms[i]; i++) {
		if (!strcasecmp(term, bc_history_bad_terms[i])) return true;
	}

	return false;
}

/**
 * Raw mode: 1960's black magic.
 */
static int bc_history_enableRaw(BcHistory *l, int fd) {

	struct termios raw;

	if (!isatty(STDIN_FILENO)) goto fatal;
	if (tcgetattr(fd, &l->orig_termios) == -1) goto fatal;

	// Modify the original mode.
	raw = l->orig_termios;

	// Input modes: no break, no CR to NL, no parity check, no strip char,
	// no start/stop output control.
	raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);

	// Control modes - set 8 bit chars.
	raw.c_cflag |= (CS8);

	// Local modes - choing off, canonical off, no extended functions,
	// no signal chars (^Z,^C).
	raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);

	// Control chars - set return condition: min number of bytes and timer.
	// We want read to give every single byte, w/o timeout (1 byte, no timer).
	raw.c_cc[VMIN] = 1;
	raw.c_cc[VTIME] = 0;

	// Put terminal in raw mode after flushing.
	if (tcsetattr(fd,TCSAFLUSH,&raw) < 0) goto fatal;
	l->rawmode = true;

	return 0;

fatal:
	errno = ENOTTY;
	return -1;
}

static void bc_history_disableRaw(BcHistory *l, int fd) {

	// Don't even check the return value as it's too late.
	if (l->rawmode && tcsetattr(fd, TCSAFLUSH, &l->orig_termios) != -1)
		l->rawmode = false;
}

/**
 * Use the ESC [6n escape sequence to query the horizontal cursor position
 * and return it. On error -1 is returned, on success the position of the
 * cursor.
 */
static int bc_history_cursorPos(int ifd, int ofd) {

	char buf[32];
	int cols, rows;
	unsigned int i;

	// Report cursor location.
	if (write(ofd, "\x1b[6n", 4) != 4) return -1;

	// Read the response: ESC [ rows ; cols R.
	for (i = 0; i < sizeof(buf) - 1; ++i) {
		if (read(ifd, buf + i, 1) != 1 || buf[i] == 'R') break;
	}

	buf[i] = '\0';

	// Parse it.
	if (buf[0] != BC_ACTION_ESC || buf[1] != '[') return -1;
	if (sscanf(buf + 2, "%d;%d", &rows, &cols) != 2) return -1;

	return cols;
}

/**
 * Try to get the number of columns in the current terminal, or assume 80
 * if it fails.
 */
static int getColumns(int ifd, int ofd) {

	struct winsize ws;

	if (ioctl(1, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {

		// Calling ioctl() failed. Try to query the terminal itself.
		int start, cols;

		// Get the initial position so we can restore it later.
		start = bc_history_cursorPos(ifd, ofd);
		if (start == -1) goto failed;

		// Go to right margin and get position.
		if (write(ofd,"\x1b[999C",6) != 6) goto failed;
		cols = bc_history_cursorPos(ifd, ofd);
		if (cols == -1) goto failed;

		// Restore position.
		if (cols > start) {

			char seq[32];

			snprintf(seq, 32, "\x1b[%dD", cols - start);

			if (write(ofd, seq, strlen(seq)) == -1)
				bc_vm_exit(BC_STATUS_IO_ERR);
		}

		return cols;
	}

	return ws.ws_col;

failed:
	return BC_HISTORY_DEF_COLS;
}

/**
 * Clear the screen. Used to handle ctrl+l.
 */
void linenoiseClearScreen(void) {

	int fd;

	fd = isatty(STDOUT_FILENO) ? STDOUT_FILENO : STDERR_FILENO;
	if (write(fd, "\x1b[H\x1b[2J", 7) <= 0) bc_vm_exit(BC_STATUS_IO_ERR);
}


/* =========================== Line editing ================================= */


/**
 * Check if text is an ANSI escape sequence.
 */
static int bc_history_ansiEscape(const char *buf, size_t buf_len, size_t *len) {

	if (buf_len > 2 && !memcmp("\033[", buf, 2)) {

		size_t off = 2;

		while (off < buf_len) {

			char c = buf[off++];

			if ((c >= 'A' && c <= 'K' && c != 'I') ||
			    c == 'S' || c == 'T' || c == 'f' || c == 'm')
			{
				*len = off;
				return 1;
			}
		}
	}
	return 0;
}

/**
 * Get column length of prompt text.
 */
static size_t bc_history_promptColLen(const char *prompt, size_t plen) {

	char buf[BC_HISTORY_MAX_LINE];
	size_t buf_len = 0, off = 0;

	while (off < plen) {

		size_t len;

		if (bc_history_ansiEscape(prompt + off, plen - off, &len)) {
			off += len;
			continue;
		}

		buf[buf_len++] = prompt[off++];
	}

	return bc_history_colPos(buf,buf_len,buf_len);
}

/**
 * Single line low level line refresh.
 *
 * Rewrites the currently edited line accordingly to the buffer content,
 * cursor position, and number of columns of the terminal.
 */
static void bc_history_refresh(BcHistory *l) {

	char seq[64];
	int colpos, fd = l->ofd;
	char *buf = l->buf;
	size_t len = l->len, pos = l->pos, pcollen;
	BcVec vec;

	pcollen = bc_history_promptColLen(l->prompt, strlen(l->prompt));

	while(pcollen + bc_history_colPos(buf, len, pos) >= l->cols) {

		int chlen = bc_history_nextLen(buf, len, 0, NULL);

		buf += chlen;
		len -= chlen;
		pos -= chlen;
	}

	while (pcollen + bc_history_colPos(buf, len, len) > l->cols)
		len -= bc_history_prevLen(buf, len, len, NULL);

	bc_vec_init(&vec, sizeof(char), NULL);

	// Cursor to left edge.
	snprintf(seq,64,"\r");
	bc_vec_string(&vec, strlen(seq), seq);

	// Write the prompt and the current buffer content.
	bc_vec_concat(&vec, l->prompt);
	bc_vec_concat(&vec, buf);

	// Erase to right.
	snprintf(seq,64,"\x1b[0K");
	bc_vec_concat(&vec, seq);

	// Move cursor to original position.
	colpos = (int) (bc_history_colPos(buf, len, pos) + pcollen);
	snprintf(seq, 64, "\r\x1b[%dC", colpos);
	bc_vec_concat(&vec, seq);

	if (write(fd, vec.v, vec.len - 1) == -1) bc_vm_exit(BC_STATUS_IO_ERR);

	bc_vec_free(&vec);
}

/**
 * Insert the character 'c' at cursor current position.
 *
 * On error writing to the terminal -1 is returned, otherwise 0.
 */
int bc_history_edit_insert(BcHistory *l, const char *cbuf, int clen) {

	if (l->len+clen <= l->buflen) {

		if (l->len == l->pos) {

			size_t colpos;

			memcpy(&l->buf[l->pos], cbuf, clen);

			l->pos += clen;
			l->len += clen;
			l->buf[l->len] = '\0';

			colpos = !bc_history_promptColLen(l->prompt, l->plen);
			colpos += bc_history_colPos(l->buf, l->len, l->len);

			if (colpos < l->cols) {
				// Avoid a full update of the line in the trivial case.
				if (write(l->ofd, cbuf, clen) == -1) return -1;
			}
			else bc_history_refresh(l);
		}
		else {

			memmove(l->buf + l->pos + clen, l->buf + l->pos, l->len - l->pos);
			memcpy(l->buf + l->pos, cbuf, clen);

			l->pos += clen;
			l->len += clen;
			l->buf[l->len] = '\0';

			bc_history_refresh(l);
		}
	}

	return 0;
}

/**
 * Move cursor to the left.
 */
void bc_history_edit_left(BcHistory *l) {
	if (l->pos > 0) {
		l->pos -= bc_history_prevLen(l->buf, l->len, l->pos, NULL);
		bc_history_refresh(l);
	}
}

/**
 * Move cursor on the right.
*/
void bc_history_edit_right(BcHistory *l) {
	if (l->pos != l->len) {
		l->pos += bc_history_nextLen(l->buf, l->len, l->pos, NULL);
		bc_history_refresh(l);
	}
}

/**
 * Move cursor to the end of the current word.
 */
void bc_history_edit_wordEnd(BcHistory *l) {

	if (l->len == 0 || l->pos >= l->len) return;

	if (l->buf[l->pos] == ' ') {
		while (l->pos < l->len && l->buf[l->pos] == ' ') ++l->pos;
	}

	while (l->pos < l->len && l->buf[l->pos] != ' ') ++l->pos;

	bc_history_refresh(l);
}

/**
 * Move cursor to the start of the current word.
 */
void bc_history_edit_wordStart(BcHistory *l) {

	if (l->len == 0) return;
	if (l->buf[l->pos-1] == ' ') --l->pos;

	if (l->buf[l->pos] == ' ') {
		while (l->pos > 0 && l->buf[l->pos] == ' ') --l->pos;
	}

	while (l->pos > 0 && l->buf[l->pos-1] != ' ') --l->pos;

	bc_history_refresh(l);
}

/**
 * Move cursor to the start of the line.
 */
void bc_history_edit_home(BcHistory *l) {
	if (l->pos != 0) {
		l->pos = 0;
		bc_history_refresh(l);
	}
}

/**
 * Move cursor to the end of the line.
 */
void bc_history_edit_end(BcHistory *l) {
	if (l->pos != l->len) {
		l->pos = l->len;
		bc_history_refresh(l);
	}
}

/**
 * Substitute the currently edited line with the next or previous history
 * entry as specified by 'dir' (direction).
 */
void linenoiseEditHistoryNext(BcHistory *l, int dir) {

	if (l->history_len > 1) {

		size_t idx = l->history_len - 1 - l->history_index;

		// Update the current history entry before
		// overwriting it with the next one.
		free(l->history[idx]);
		l->history[idx] = strdup(l->buf);

		// Show the new entry.
		l->history_index += (dir == BC_HISTORY_PREV) ? 1 : -1;

		if (l->history_index < 0) {
			l->history_index = 0;
			return;
		}
		else if (l->history_index >= l->history_len) {
			l->history_index = l->history_len - 1;
			return;
		}

		idx = l->history_len - 1 - l->history_index;

		strncpy(l->buf, l->history[idx], l->buflen);

		l->buf[l->buflen - 1] = '\0';
		l->len = l->pos = strlen(l->buf);

		bc_history_refresh(l);
	}
}

/**
 * Delete the character at the right of the cursor without altering the cursor
 * position. Basically this is what happens with the "Delete" keyboard key.
 */
void bc_history_edit_delete(BcHistory *l) {

	if (l->len > 0 && l->pos < l->len) {

		int chlen = bc_history_nextLen(l->buf, l->len, l->pos, NULL);

		memmove(l->buf + l->pos, l->buf + l->pos + chlen, l->len - l->pos - chlen);

		l->len -= chlen;
		l->buf[l->len] = '\0';

		bc_history_refresh(l);
	}
}

void bc_history_edit_backspace(BcHistory *l) {

	if (l->pos > 0 && l->len > 0) {

		int chlen = bc_history_prevLen(l->buf, l->len, l->pos, NULL);

		memmove(l->buf + l->pos - chlen, l->buf + l->pos, l->len - l->pos);

		l->pos -= chlen;
		l->len -= chlen;
		l->buf[l->len] = '\0';

		bc_history_refresh(l);
	}
}

/**
 * Delete the previous word, maintaining the cursor at the start of the
 * current word.
 */
void bc_history_edit_deletePrevWord(BcHistory *l) {

	size_t diff, old_pos = l->pos;

	while (l->pos > 0 && l->buf[l->pos - 1] == ' ') --l->pos;
	while (l->pos > 0 && l->buf[l->pos - 1] != ' ') --l->pos;

	diff = old_pos - l->pos;
	memmove(l->buf + l->pos, l->buf + old_pos, l->len - old_pos + 1);
	l->len -= diff;

	bc_history_refresh(l);
}

/**
 * Delete the next word, maintaining the cursor at the same position.
 */
void bc_history_deleteNextWord(BcHistory *l) {

	size_t next_word_end = l->pos;

	while (next_word_end < l->len && l->buf[next_word_end] == ' ') ++next_word_end;
	while (next_word_end < l->len && l->buf[next_word_end] != ' ') ++next_word_end;

	memmove(l->buf+l->pos, l->buf+next_word_end, l->len-next_word_end);

	l->len -= next_word_end - l->pos;

	bc_history_refresh(l);
}

static void bc_history_escape(BcHistory *l) {

	char seq[3];

	if (read(l->ifd, seq, 1) == -1) return;

	// ESC ? sequences.
	if (seq[0] != '[' && seq[0] != '0') {

		switch (seq[0]) {

			case 'f':
			{
				bc_history_edit_wordEnd(l);
				break;
			}

			case 'b':
			{
				bc_history_edit_wordStart(l);
				break;
			}

			case 'd':
			{
				bc_history_deleteNextWord(l);
				break;
			}
		}
	}
	else {

		if (read(l->ifd, seq + 1, 1) == -1) return;

		// ESC [ sequences.
		if (seq[0] == '[') {

			if (seq[1] >= '0' && seq[1] <= '9') {

				// Extended escape, read additional byte.
				if (read(l->ifd, seq + 2, 1) == -1) return;
				if (seq[2] == '~') {

					switch(seq[1]) {

						// Delete key.
						case '3':
						{
							bc_history_edit_delete(l);
							break;
						}
					}
				}
			}
			else {

				switch(seq[1]) {

					// Up.
					case 'A':
					{
						linenoiseEditHistoryNext(l, BC_HISTORY_PREV);
						break;
					}

					// Down.
					case 'B':
					{
						linenoiseEditHistoryNext(l, BC_HISTORY_NEXT);
						break;
					}

					// Right.
					case 'C':
					{
						bc_history_edit_right(l);
						break;
					}

					// Left.
					case 'D':
					{
						bc_history_edit_left(l);
						break;
					}

					// Home.
					case 'H':
					case '1':
					{
						bc_history_edit_home(l);
						break;
					}

					// End.
					case 'F':
					case '4':
					{
						bc_history_edit_end(l);
						break;
					}

					case 'd':
					{
						bc_history_deleteNextWord(l);
						break;
					}
				}
			}
		}
		// ESC O sequences.
		else if (seq[0] == 'O') {

			switch(seq[1]) {

				case 'H':
				{
					bc_history_edit_home(l);
					break;
				}

				case 'F':
				{
					bc_history_edit_end(l);
					break;
				}
			}
		}
	}
}

/**
 * This function is the core of the line editing capability of linenoise.
 * It expects 'fd' to be already in "raw mode" so that every key pressed
 * will be returned ASAP to read().
 *
 * The resulting string is put into 'buf' when the user type enter, or
 * when ctrl+d is typed.
 *
 * The function returns the length of the current buffer.
 */
static int bc_history_edit(BcHistory *l, int ifd, int ofd, char *buf,
                           size_t buflen, const char *prompt)
{
	// Populate the linenoise state that we pass to functions
	// implementing specific editing functionalities.
	l->ifd = ifd;
	l->ofd = ofd;
	l->buf = buf;
	l->buflen = buflen;
	l->prompt = prompt;
	l->plen = strlen(prompt);
	l->oldcolpos = l->pos = 0;
	l->len = 0;
	l->cols = getColumns(ifd, ofd);
	l->history_index = 0;

	// Buffer starts empty.
	l->buf[0] = '\0';

	// Make sure there is always space for the nulterm.
	l->buflen--;

	// The latest history entry is always our current buffer, that
	// initially is just an empty string.
	bc_history_add(l, "");

	if (write(l->ofd, prompt, l->plen) == -1) return -1;

	while(1) {

		int c;
		char cbuf[32]; // large enough for any encoding?
		int nread;

	/* Continue reading if interrupted by a signal */
// TODO
//	do {
//		  nread = read(l.ifd,&c,1);
//		} while((nread == -1) && (errno == EINTR));
		nread = bc_history_readCode(l->ifd, cbuf, sizeof(cbuf), &c);
		if (nread <= 0) return l->len;

		switch(c) {

			case BC_ACTION_LINE_FEED:
			case BC_ACTION_ENTER:
			{
				l->history_len--;
				free(l->history[l->history_len]);
				return (int)l->len;
			}

			case BC_ACTION_CTRL_C:
			{
				errno = EAGAIN;
				return -1;
			}

			case BC_ACTION_BACKSPACE:
			case BC_ACTION_CTRL_H:
			{
				bc_history_edit_backspace(l);
				break;
			}

			// Remove char at right of cursor, or if the
			// line is empty, act as end-of-file.
			case BC_ACTION_CTRL_D:
			{
				if (l->len > 0) {
					bc_history_edit_delete(l);
				}
				else {
					l->history_len--;
					free(l->history[l->history_len]);
					return -1;
				}

				break;
			}

			// Swaps current character with previous.
			case BC_ACTION_CTRL_T:
			{
				int pcl, ncl;
				char auxb[5];

				pcl = bc_history_prevLen(l->buf, l->len, l->pos, NULL);
				ncl = bc_history_nextLen(l->buf, l->len, l->pos, NULL);

				// To perform a swap we need:
				// * nonzero char length to the left
				// * not at the end of the line
				if(pcl != 0 && l->pos != l->len && pcl < 5 && ncl < 5) {

					// The actual transpose works like this
					//		   ,--- l.pos
					//		  v
					// xxx [AAA] [BB] xxx
					// xxx [BB] [AAA] xxx
					memcpy(auxb, l->buf + l->pos - pcl, pcl);
					memcpy(l->buf + l->pos - pcl, l->buf + l->pos, ncl);
					memcpy(l->buf + l->pos - pcl + ncl, auxb, pcl);

					l->pos += -pcl + ncl;

					bc_history_refresh(l);
				}

				break;
			}

			case BC_ACTION_CTRL_B:
			{
				bc_history_edit_left(l);
				break;
			}

			case BC_ACTION_CTRL_F:
			{
				bc_history_edit_right(l);
				break;
			}

			case BC_ACTION_CTRL_P:
			{
				linenoiseEditHistoryNext(l, BC_HISTORY_PREV);
				break;
			}

			case BC_ACTION_CTRL_N:
			{
				linenoiseEditHistoryNext(l, BC_HISTORY_NEXT);
				break;
			}

			case BC_ACTION_ESC:
			{
				bc_history_escape(l);
				break;
			}

			// Delete the whole line.
			case BC_ACTION_CTRL_U:
			{
				buf[0] = '\0';
				l->pos = l->len = 0;

				bc_history_refresh(l);

				break;
			}

			// Delete from current to end of line.
			case BC_ACTION_CTRL_K:
			{
				buf[l->pos] = '\0';
				l->len = l->pos;

				bc_history_refresh(l);

				break;
			}

			// Go to the start of the line.
			case BC_ACTION_CTRL_A:
			{
				bc_history_edit_home(l);
				break;
			}

			// Go to the end of the line.
			case BC_ACTION_CTRL_E:
			{
				bc_history_edit_end(l);
				break;
			}

			// Clear screen.
			case BC_ACTION_CTRL_L:
			{
				linenoiseClearScreen();
				bc_history_refresh(l);
				break;
			}

			// Delete previous word.
			case BC_ACTION_CTRL_W:
			{
				bc_history_edit_deletePrevWord(l);
				break;
			}

			default:
			{
				if (bc_history_edit_insert(l, cbuf, nread)) return -1;
				break;
			}
		}
	}

	return l->len;
}

/* This special mode is used by linenoise in order to print scan codes
 * on screen for debugging / development purposes. It is implemented
 * by the linenoise_example program using the --keycodes option. */
#ifndef NDEBUG
void bc_history_printKeyCodes(BcHistory *l) {

	char quit[4];

	printf("Linenoise key codes debugging mode.\n"
			"Press keys to see scan codes. Type 'quit' at any time to exit.\n");

	if (bc_history_enableRaw(l, STDIN_FILENO) == -1) return;
	memset(quit, ' ', 4);

	while(true) {

		char c;
		int nread;

		nread = read(STDIN_FILENO,&c,1);
		if (nread <= 0) continue;

		// Shift string to left.
		memmove(quit, quit + 1, sizeof(quit) - 1);

		// Insert current char on the right.
		quit[sizeof(quit) - 1] = c;
		if (memcmp(quit, "quit", sizeof(quit)) == 0) break;

		printf("'%c' %02x (%d) (type quit to exit)\n",
			isprint((int) c) ? c : '?', (int) c, (int) c);

		// Go left edge manually, we are in raw mode.
		printf("\r");
		fflush(stdout);
	}

	bc_history_disableRaw(l, STDIN_FILENO);
}
#endif // NDEBUG

/**
 * This function calls the line editing function linenoiseEdit() using
 * the STDIN file descriptor set in raw mode.
 */
static int bc_history_raw(BcHistory *l, char *buf, FILE *out,
                          size_t buflen, const char *prompt)
{
	int outfd, count;

	if (buflen == 0) {
		errno = EINVAL;
		return -1;
	}

	if ((outfd = fileno(out)) == -1) return -1;

	if (bc_history_enableRaw(l, STDIN_FILENO) == -1) return -1;

	count = bc_history_edit(l, STDIN_FILENO, outfd, buf, buflen, prompt);
	bc_history_disableRaw(l, STDIN_FILENO);
	fprintf(out, "\n");

	return count;
}

/**
 * This function is called when bc_history_line() is called with the standard
 * input file descriptor not attached to a TTY. So for example when the
 * program using linenoise is called in pipe or with a file redirected
 * to its standard input. In this case, we want to be able to return the
 * line regardless of its length (by default we are limited to 4k).
 */
static char* bc_history_notty() {

	char *line = NULL;
	size_t len = 0, maxlen = 0;

	while(true) {

		int c;

		if (len == maxlen) {

			char *oldval = line;

			if (maxlen == 0) maxlen = 16;
			maxlen *= 2;
			line = realloc(line, maxlen);

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
			}
			else {
				line[len] = '\0';
				return line;
			}
		}
		else {
			line[len] = c;
			len++;
		}
	}
}

/**
 * The high-level function that is the main API of the linenoise library.
 * This function checks if the terminal has basic capabilities, just checking
 * for a blacklist of stupid terminals, and later either calls the line
 * editing function or uses dummy fgets() so that you will be able to type
 * something even in the most desperate of the conditions.
 */
char* bc_history_line(BcHistory *l, const char *prompt) {

	char buf[BC_HISTORY_MAX_LINE];
	FILE *stream;
	int count;

	stream = isatty(STDOUT_FILENO) ? stdout : stderr;

	if (!vm->ttyin) {
		// Not a tty: read from file / pipe. In this mode we don't want any
		// limit to the line size, so we call a function to handle that.
		return bc_history_notty();
	}
	else if (bc_history_isBadTerm()) {

		size_t len;

		fprintf(stream, "%s", prompt);
		fflush(stream);

		if (fgets(buf, BC_HISTORY_MAX_LINE, stdin) == NULL) return NULL;

		len = strlen(buf);

		while(len && (buf[len-1] == '\n' || buf[len-1] == '\r')) {
			len--;
			buf[len] = '\0';
		}

		return strdup(buf);
	}
	else {
		count = bc_history_raw(l, buf, stream, BC_HISTORY_MAX_LINE, prompt);
		if (count == -1) return NULL;
		return strdup(buf);
	}
}

/* ================================ History ================================= */

/**
 * This is the API call to add a new entry in the bc history. It uses a
 * fixed array of char pointers that are shifted (memmoved) when the
 * history max length is reached in order to remove the older entry and
 * make room for the new one, so it is not exactly suitable for huge
 * histories, but will work well for a few hundred of entries.
 *
 * Using a circular buffer is smarter, but a bit more complex to handle.
 */
bool bc_history_add(BcHistory *l, const char *line) {

	char *linecopy;

	// Don't add duplicated lines.
	if (l->history_len && !strcmp(l->history[l->history_len - 1], line))
		return false;

	// Add an heap allocated copy of the line in the history.
	// If we reached the max length, remove the older line.
	linecopy = bc_vm_strdup(line);

	if (l->history_len == BC_HISTORY_MAX_LEN) {
		free(l->history[0]);
		memmove(l->history, l->history + 1, sizeof(char*) * (BC_HISTORY_MAX_LEN - 1));
		l->history_len--;
	}

	l->history[l->history_len] = linecopy;
	l->history_len++;

	return true;
}

void bc_history_init(BcHistory *l) {
	l->rawmode = false;
	l->history_len = 0;
	l->history = calloc(BC_HISTORY_MAX_LEN, sizeof(char*));
}

void bc_history_free(BcHistory *l) {

	int i;

	bc_history_disableRaw(l, STDIN_FILENO);

	for (i = 0; i < l->history_len; i++)
		free(l->history[i]);

	free(l->history);
}
#endif // BC_ENABLE_HISTORY
