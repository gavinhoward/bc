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
 * Based on linenoise.c -- guerrilla line editing library against the idea that
 * a line editing lib needs to be 20,000 lines of C code.
 *
 * You can find the original source code at:
 *   http://github.com/antirez/linenoise
 *
 * Does a number of crazy assumptions that happen to be true in 99.9999% of
 * the 2010 UNIX computers around.
 *
 * The code is also under the following copyrights and license.
 *
 * *****************************************************************************
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
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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
 * *****************************************************************************
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
 *			where n is the row and m is the column
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
 * Code to keep history.
 *
 */

#if BC_ENABLE_HISTORY

#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

#include <termios.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#include <read.h>
#include <history.h>
#include <vm.h>

static BcStatus bc_history_refresh(BcHistory *h);

void bc_history_init(BcHistory *h) {

	int j;
	char *term = getenv("TERM");

	h->unsupported = false;

	if (term != NULL) {
		for (j = 0; !h->unsupported && bc_history_unsupportedTerms[j]; ++j)
			h->unsupported = !strcasecmp(term, bc_history_unsupportedTerms[j]);
	}

	h->ifd = STDIN_FILENO;
	h->ofd = STDOUT_FILENO;
	h->rawmode = false;
	h->buf = NULL;
	bc_vec_init(&h->history, sizeof(BcVec), bc_vec_free);
	bc_vec_expand(&h->history, BC_HISTORY_MAX);
}

static BcStatus bc_history_enableRaw(BcHistory *h) {

	struct termios raw;

	if (tcgetattr(h->ifd, &h->orig_termios) == -1) return BC_STATUS_TERM_ERR;

	// Modify the original mode.
	raw = h->orig_termios;

	// Input modes: no break, no CR to NL, no parity check, no strip char,
	// no start/stop output control.
	raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);

	// Output modes: disable post processing.
	raw.c_oflag &= ~(OPOST);

	// Control modes: set 8 bit chars.
	raw.c_cflag |= (CS8);

	// local modes: choing off, canonical off, no extended functions,
	// no signal chars (^Z,^C).
	raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);

	// Control chars: set return condition as min number of bytes and timer.
	// We want read to return every single byte, without timeout.
	// 1 byte, no timer.
	raw.c_cc[VMIN] = 1;
	raw.c_cc[VTIME] = 0;

	// Put terminal in raw mode after flushing.
	if (tcsetattr(h->ifd, TCSAFLUSH, &raw) < 0) return BC_STATUS_TERM_ERR;
	h->rawmode = true;

	return BC_STATUS_SUCCESS;
}

static BcStatus bc_history_disableRaw(BcHistory *h) {
	// Don't even check the return value as it's too late.
	if (h->rawmode && tcsetattr(h->ifd, TCSAFLUSH, &h->orig_termios) != -1) {
		h->rawmode = false;
		return BC_STATUS_SUCCESS;
	}
	return h->rawmode ? BC_STATUS_TERM_ERR : BC_STATUS_SUCCESS;
}

/*
 * Use the ESC [6n escape sequence to query the horizontal cursor position
 * and return it. On error -1 is returned, on success the position of the
 * cursor.
*/
static BcStatus bc_history_cursorPos(int ifd, int ofd, size_t *pos) {

	char buf[32];
	int rows;
	unsigned int i = 0;

	// Report cursor location.
	if (write(ofd, "\x1b[6n", 4) != 4) return BC_STATUS_IO_ERR;

	// Read the response: ESC [ rows ; cols R.
	while (i < sizeof(buf) - 1) {
		if (read(ifd, buf + i, 1) != 1) break;
		if (buf[i] == 'R') break;
		i++;
	}

	buf[i] = '\0';

	// Parse it.
	if (buf[0] != BC_HISTORY_ACTION_ESC || buf[1] != '[')
		return BC_STATUS_TERM_ERR;
	if (sscanf(buf + 2, "%d;%u", &rows, &i) != 2) return BC_STATUS_TERM_ERR;

	*pos = i;

	return BC_STATUS_SUCCESS;
}

/*
 * Try to get the number of columns in the current terminal, or assume 80
 * if it fails.
*/
static BcStatus bc_history_columns(int ifd, int ofd, size_t *cols) {

	BcStatus s;
	struct winsize ws;

	*cols = 80;

	// If ioctl() fails, try to query the terminal itself.
	if (ioctl(1, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {

		size_t start;

		// Get the initial position so we can restore it later.
		s = bc_history_cursorPos(ifd, ofd, &start);
		if (s) return s;

		// Go to right margin and get position.
		if (write(ofd, "\x1b[999C", 6) != 6) return BC_STATUS_IO_ERR;
		s = bc_history_cursorPos(ifd, ofd, cols);
		if (s) return s;

		// Restore position.
		if (*cols > start) {

			char seq[33];
			size_t len;

			snprintf(seq, 32, "\x1b[%zuD", *cols - start);
			len = strlen(seq);

			if ((size_t) write(ofd, seq, len) != len) return BC_STATUS_IO_ERR;
		}

	}
	else *cols = ws.ws_col;

	return BC_STATUS_SUCCESS;
}

/*
 * Clear the screen.
 */
BcStatus bc_history_clearScreen(BcHistory *h) {
	if (write(h->ofd, "\x1b[H\x1b[2J", 7) <= 0) return BC_STATUS_IO_ERR;
	else return BC_STATUS_SUCCESS;
}

/*
 * Single line low level line refresh.
 *
 * Rewrite the currently edited line accordingly to the buffer content,
 * cursor position, and number of columns of the terminal.
*/
static BcStatus bc_history_refresh(BcHistory *h) {

	BcStatus s = BC_STATUS_SUCCESS;
	char seq[64];
	size_t plen = strlen(h->prompt);
	char *buf = h->buf->v;
	size_t len = h->buf->len - 1;
	int pos = h->pos;
	BcVec ab;

	while(plen + pos >= h->cols) {
		buf++;
		len--;
		pos--;
	}

	while (len + len > h->cols) len--;

	bc_vec_init(&ab, sizeof(char), NULL);

	// Cursor to left edge.
	snprintf(seq, 64, "\r");
	bc_vec_concat(&ab, seq);

	// Write the prompt and the current buffer content.
	bc_vec_concat(&ab, h->prompt);
	bc_vec_concat(&ab, buf);

	// Erase to right.
	snprintf(seq, 64, "\x1b[0K");
	bc_vec_concat(&ab, seq);

	// Move cursor to original position.
	snprintf(seq, 64, "\r\x1b[%dC", (int) (pos + plen));
	bc_vec_concat(&ab, seq);

	if (write(h->ofd, ab.v, ab.len - 1) == -1) s = BC_STATUS_IO_ERR;
	bc_vec_free(&ab);

	return s;
}

/*
 * This is the API call to add a new entry in the linenoise history.
 * It uses a fixed array of char pointers that are shifted (memmoved)
 * when the history max length is reached in order to remove the older
 * entry and make room for the new one, so it is not exactly suitable for huge
 * histories, but will work well for a few hundred of entries.
 *
 * Using a circular buffer is smarter, but a bit more complex to handle.
*/
static void bc_history_add(BcHistory *h, const char *line) {

	BcVec temp;

	if (h->history.len == BC_HISTORY_MAX) {
		bc_vec_free(bc_vec_item(&h->history, 0));
		memmove(h->history.v, h->history.v + sizeof(BcVec),
		        sizeof(BcVec) * (BC_HISTORY_MAX - 1));
		--h->history.len;
	}

	bc_vec_init(&temp, sizeof(char), NULL);
	bc_vec_string(&temp, strlen(line), line);
	bc_vec_push(&h->history, &temp);

	h->buf = bc_vec_top(&h->history);
}

static void bc_history_clear(BcHistory *h) {
	h->pos = h->idx = 0;
	h->new = true;
	bc_history_add(h, "");
}

void bc_history_startEdit(BcHistory *h) {

	if (h->new && h->idx != 0) {
		BcVec *item = bc_vec_item_rev(&h->history, h->idx);
		bc_history_add(h, item->v);
		h->idx += 1;
	}

	assert(h->buf == bc_vec_top(&h->history));

	h->new = false;
}

/*
 * Insert the character 'c' at cursor current position.
 * On error writing to the terminal -1 is returned, otherwise 0.
*/
BcStatus bc_history_insert(BcHistory *h, char c) {

	BcStatus s;

	bc_history_startEdit(h);

	if (h->buf->len == h->buf->cap) bc_vec_grow(h->buf, 1);

	if (BC_HISTORY_BUFLEN(h) == (size_t) h->pos) {

		h->buf->v[h->pos] = c;
		++h->pos;
		++h->buf->len;
		h->buf->v[h->buf->len] = '\0';

		// Avoid a full update of the line in the trivial case.
		if (h->plen + BC_HISTORY_BUFLEN(h) < h->cols) {
			if (write(h->ofd, &c, 1) == -1) s = BC_STATUS_IO_ERR;
			else s = BC_STATUS_SUCCESS;
		}
		else s = bc_history_refresh(h);
	}
	else {
		bc_vec_pushAt(h->buf, &c, h->pos);
		s = bc_history_refresh(h);
	}

	return s;
}

/*
 * Move cursor to the left.
 */
BcStatus bc_history_moveLeft(BcHistory *h) {

	BcStatus s = BC_STATUS_SUCCESS;

	if (h->pos > 0) {
		--h->pos;
		s = bc_history_refresh(h);
	}

	return s;
}

/*
 * Move cursor to the right.
 */
BcStatus bc_history_moveRight(BcHistory *h) {

	BcStatus s = BC_STATUS_SUCCESS;

	if (BC_HISTORY_BUFLEN(h) != (size_t) h->pos) {
		++h->pos;
		s = bc_history_refresh(h);
	}

	return s;
}

/*
 * Move cursor to the start of the line.
 */
BcStatus bc_history_moveHome(BcHistory *h) {

	BcStatus s = BC_STATUS_SUCCESS;

	if (h->pos != 0) {
		h->pos = 0;
		s = bc_history_refresh(h);
	}

	return s;
}

/*
 * Move cursor to the end of the line.
 */
BcStatus bc_history_moveEnd(BcHistory *h) {

	BcStatus s = BC_STATUS_SUCCESS;

	if (BC_HISTORY_BUFLEN(h) != (size_t) h->pos) {
		h->pos = BC_HISTORY_BUFLEN(h);
		s = bc_history_refresh(h);
	}

	return s;
}

/*
 * Substitute the currently edited line with the next or previous history
 * entry as specified by 'dir'.
 */
BcStatus bc_history_next(BcHistory *h, int dir) {

	BcStatus s = BC_STATUS_SUCCESS;

	if (h->history.len != 0) {

		// Show the new entry.
		h->idx += (dir == BC_HISTORY_PREV) ? 1 : -1;
		h->idx = h->idx == ((size_t) -1) ? 0 : h->idx;
		h->idx = h->idx >= h->history.len ? h->history.len : h->idx;

		h->buf = bc_vec_item_rev(&h->history, h->idx);
		h->pos = BC_HISTORY_BUFLEN(h);
		h->new = true;
		s = bc_history_refresh(h);
	}

	return s;
}

/*
 * Delete the character at the right of the cursor without altering the cursor
 * position. Basically this is what happens with the "Delete" keyboard key.
*/
BcStatus bc_history_delete(BcHistory *h) {

	BcStatus s = BC_STATUS_SUCCESS;

	if (BC_HISTORY_BUFLEN(h) > 0 && BC_HISTORY_BUFLEN(h) > (size_t) h->pos) {

		bc_history_startEdit(h);

		memmove(h->buf->v + h->pos, h->buf->v + h->pos + 1,
		        BC_HISTORY_BUFLEN(h) - h->pos - 1);

		--h->buf->len;
		h->buf->v[h->buf->len] = '\0';

		s = bc_history_refresh(h);
	}

	return s;
}

/*
 * Backspace implementation.
 */
BcStatus bc_history_backspace(BcHistory *h) {

	BcStatus s = BC_STATUS_SUCCESS;

	if (h->pos > 0 && BC_HISTORY_BUFLEN(h) > 0) {

		bc_history_startEdit(h);

		memmove(h->buf->v + h->pos - 1, h->buf->v + h->pos,
		        BC_HISTORY_BUFLEN(h) - h->pos);

		--h->pos;
		--h->buf->len;
		h->buf->v[h->buf->len - 1] = '\0';

		s = bc_history_refresh(h);
	}

	return s;
}

/*
 * Delete the previous word, maintaining the cursor at the start of the
 * current word.
 */
BcStatus bc_history_deletePrevWord(BcHistory *h) {

	size_t old_pos = h->pos;
	size_t diff;

	bc_history_startEdit(h);

	while (h->pos > 0 && h->buf->v[h->pos - 1] == ' ') --h->pos;
	while (h->pos > 0 && h->buf->v[h->pos - 1] != ' ') --h->pos;

	diff = old_pos - h->pos;
	memmove(h->buf->v + h->pos, h->buf->v + old_pos,
	        BC_HISTORY_BUFLEN(h) - old_pos + 1);
	h->buf->len -= diff;

	return bc_history_refresh(h);
}

/*
 * This function is the core of the line editing capability of linenoise.
 * It expects 'fd' to be already in "raw mode" so that every key pressed
 * will be returned ASAP to read().
 *
 * The resulting string is put into 'buf' when the user type enter, or
 * when ctrl+d is typed.
 *
 * The function returns the length of the current buffer.
 */
static BcStatus bc_history_edit(BcHistory *h, BcVec *vec, const char *prompt) {

	BcStatus s;
	bool done = false;

	// Populate the linenoise state that we pass to functions implementing
	// specific editing functionalities.
	h->prompt = prompt;
	h->plen = strlen(prompt);
	h->oldpos = 0;

	s = bc_history_columns(h->ifd, h->ofd, &h->cols);
	if (s) return s;

	// Buffer starts empty.
	bc_history_clear(h);

	if (write(h->ofd, prompt, h->plen) == -1) return BC_STATUS_TERM_ERR;

	while(!s && !done) {

		char c;
		int nread;
		char seq[3];

		nread = read(h->ifd, &c, 1);
		if (nread <= 0) break;

		switch(c) {

			case BC_HISTORY_ACTION_ENTER:
			{
				BcVec *h1 = bc_vec_item_rev(&h->history, 0);

				if (!strcmp(h1->v, "")) bc_vec_pop(&h->history);
				else if (h->history.len >= 2) {
					BcVec *h2 = bc_vec_item_rev(&h->history, 1);
					if (!strcmp(h1->v, h2->v)) bc_vec_pop(&h->history);
				}

				done = true;

				break;
			}

			case BC_HISTORY_ACTION_CTRL_C:
			{
				if (write(h->ofd, "^C", 2) != 2) return BC_STATUS_IO_ERR;

#if BC_ENABLE_SIGNALS
				size_t len = strlen(bc_program_ready_msg);

				if (write(h->ofd, "\r", 1) != 1) return BC_STATUS_IO_ERR;

				bc_vm_sig(SIGINT);
				bcg.sigc = bcg.sig;
				bcg.signe = 0;

				if (write(h->ofd, "\r", 1) != 1) return BC_STATUS_IO_ERR;
				if (write(2, bc_program_ready_msg, len) != (ssize_t) len)
					return BC_STATUS_IO_ERR;

				if (!h->new) bc_history_clear(h);

				s = bc_history_refresh(h);
#else // BC_ENABLE_SIGNALS
				bc_history_disableRaw(h);
				exit(BC_STATUS_SUCCESS);
#endif // BC_ENABLE_SIGNALS

				break;
			}

			case BC_HISTORY_ACTION_BACKSPACE:
			case BC_HISTORY_ACTION_CTRL_H:
			{
				s = bc_history_backspace(h);
				break;
			}

			// Remove char at right of cursor, or if the
			// line is empty, act as end-of-file.
			case BC_HISTORY_ACTION_CTRL_D:
			{
				if (BC_HISTORY_BUFLEN(h) > 0) bc_history_delete(h);
				else {
					bc_vec_pop(&h->history);
					return BC_STATUS_IO_ERR;
				}
				break;
			}

			// Swaps current character with previous.
			case BC_HISTORY_ACTION_CTRL_T:
			{
				if (h->pos > 0 && h->buf->len > (size_t) h->pos) {

					bc_history_startEdit(h);

					int aux = h->buf->v[h->pos - 1];

					h->buf->v[h->pos - 1] = h->buf->v[h->pos];
					h->buf->v[h->pos] = aux;

					if (BC_HISTORY_BUFLEN(h) != (size_t) h->pos) ++h->pos;
					s = bc_history_refresh(h);
				}
				break;
			}

			case BC_HISTORY_ACTION_CTRL_B:
			{
				s = bc_history_moveLeft(h);
				break;
			}

			case BC_HISTORY_ACTION_CTRL_F:
			{
				bc_history_moveRight(h);
				break;
			}

			case BC_HISTORY_ACTION_CTRL_P:
			{
				bc_history_next(h, BC_HISTORY_PREV);
				break;
			}

			case BC_HISTORY_ACTION_CTRL_N:
			{
				bc_history_next(h, BC_HISTORY_NEXT);
				break;
			}

			case BC_HISTORY_ACTION_ESC:
			{
				// Read the next two bytes representing the escape sequence.
				// Use two calls to handle slow terminals returning the two
				// chars at different times.
				if (read(h->ifd, seq, 1) == -1) break;
				if (read(h->ifd, seq + 1, 1) == -1) break;

				// ESC [ sequences.
				if (seq[0] == '[') {

					if (seq[1] >= '0' && seq[1] <= '9') {

						// Extended escape, read additional byte.
						if (read(h->ifd, seq + 2, 1) == -1) break;

						if (seq[2] == '~' && seq[1] == '3')
							s = bc_history_delete(h);
					}
					else {

						switch(seq[1]) {

							case 'A':
							{
								s = bc_history_next(h, BC_HISTORY_PREV);
								break;
							}

							case 'B':
							{
								s = bc_history_next(h, BC_HISTORY_NEXT);
								break;
							}

							case 'C':
							{
								s = bc_history_moveRight(h);
								break;
							}

							case 'D':
							{
								s = bc_history_moveLeft(h);
								break;
							}

							case 'H':
							{
								s = bc_history_moveHome(h);
								break;
							}

							case 'F':
							{
								s = bc_history_moveEnd(h);
								break;
							}
						}
					}
				}
				else if (seq[0] == 'O') {

					switch(seq[1]) {

						case 'H':
						{
							bc_history_moveHome(h);
							break;
						}

						case 'F':
						{
							bc_history_moveEnd(h);
							break;
						}
					}
				}

				break;
			}

			// Delete the whole line.
			case BC_HISTORY_ACTION_CTRL_U:
			{
				bc_history_startEdit(h);
				h->buf->v[0] = '\0';
				h->pos = 0;
				h->buf->len = 1;
				s = bc_history_refresh(h);
				break;
			}

			// Delete from current to end of line.
			case BC_HISTORY_ACTION_CTRL_K:
			{
				bc_history_startEdit(h);
				h->buf->v[h->pos] = '\0';
				h->buf->len = h->pos + 1;
				s = bc_history_refresh(h);
				break;
			}

			// Go to the start of the line.
			case BC_HISTORY_ACTION_CTRL_A:
			{
				s = bc_history_moveHome(h);
				break;
			}

			// Go to the end of the line.
			case BC_HISTORY_ACTION_CTRL_E:
			{
				s = bc_history_moveEnd(h);
				break;
			}

			// Clear screen.
			case BC_HISTORY_ACTION_CTRL_L:
			{
				s = bc_history_clearScreen(h);
				if (!s) bc_history_refresh(h);
				break;
			}

			// Delete previous word.
			case BC_HISTORY_ACTION_CTRL_W:
			{
				s = bc_history_deletePrevWord(h);
				break;
			}

			default:
			{
				s = bc_history_insert(h, c);
				break;
			}
		}
	}

	bc_vec_string(vec, BC_HISTORY_BUFLEN(h), h->buf->v);
	bc_vec_concat(vec, "\n");

	return s;
}

/*
 * This special mode is used in order to print scan codes
 * on screen for debugging/development purposes.
 */
#ifndef NDEBUG
BcStatus bc_history_printKeyCodes(BcHistory *h) {

	BcStatus s;
	char quit[4];

	bc_vm_printf(stdout, "History key codes debugging mode.\n");
	bc_vm_printf(stdout, "Press keys to see scan codes.\n");
	bc_vm_printf(stdout, "Type 'quit' at any time to exit.\n");

	s = bc_history_enableRaw(h);
	if (s) return s;

	memset(quit,' ',4);

	while(1) {

		char c;
		int nread;

		nread = read(h->ifd, &c, 1);
		if (nread <= 0) continue;

		// shift string to left.
		memmove(quit, quit + 1, sizeof(quit) - 1);

		// Insert current char on the right.
		quit[sizeof(quit) - 1] = c;

		if (memcmp(quit, "quit", sizeof(quit)) == 0) break;

		bc_vm_printf(stdout, "'%c' %02x (%d) (type quit to exit)\n",
		             isprint(c) ? c : '?', (int) c, (int) c);

		// Go left edge manually, we are in raw mode.
		bc_vm_printf(stdout, "\r");
		bc_vm_fflush(stdout);
	}

	return bc_history_disableRaw(h);
}
#endif // NDEBUG

/*
 * This function calls the line editing function linenoiseEdit() using
 * the STDIN file descriptor set in raw mode.
*/
static BcStatus bc_history_raw(BcHistory *h, BcVec *vec, const char *prompt) {

	BcStatus s;

	s = bc_history_enableRaw(h);
	if (s) return s;

	s = bc_history_edit(h, vec, prompt);
	if (s) return s;

	s = bc_history_disableRaw(h);
	bc_vm_putchar('\n');

	return s;
}

BcStatus bc_history_line(BcHistory *h, BcVec *vec, const char *prompt) {

	BcStatus s;

	if (!bcg.ttyin || h->unsupported) s = bc_read_line(vec, prompt);
	else s = bc_history_raw(h, vec, prompt);

	return s;
}

void bc_history_free(BcHistory *h) {
	bc_history_disableRaw(h);
	bc_vec_free(&h->history);
}
#endif // BC_ENABLE_HISTORY
