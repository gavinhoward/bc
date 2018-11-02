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
 * Declarations for keeping history.
 *
 */

#ifndef BC_HISTORY_H
#define BC_HISTORY_H

#if BC_ENABLE_HISTORY

#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include <termios.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#include <vector.h>

#ifdef _WIN32
#error History is unsupported on Windows
#endif // _WIN32

#define BC_HISTORY_MAX (128)
#define BC_HISTORY_NEXT (0)
#define BC_HISTORY_PREV (1)

#define BC_HISTORY_BUFLEN(h) ((h)->buf->len - 1)

typedef struct BcHistory {

	// Terminal stdin file descriptor.
	int ifd;

	// Terminal stdout file descriptor.
	int ofd;

	// Prompt to display.
	const char *prompt;

	// Prompt length.
	size_t plen;

	// Current cursor position.
	size_t pos;

	// Previous refresh cursor position.
	size_t oldpos;

	// Number of columns in terminal.
	size_t cols;

	// The history index we are currently editing.
	size_t idx;

	// In order to restore at exit.
	struct termios orig_termios;

	bool unsupported;

	// For atexit() function to check if restore is needed.
	bool rawmode;
	bool new;

	BcVec *buf;
	BcVec history;

} BcHistory;

typedef enum BcHistoryAction {
	BC_HISTORY_ACTION_NULL = 0,
	BC_HISTORY_ACTION_CTRL_A = 1,
	BC_HISTORY_ACTION_CTRL_B = 2,
	BC_HISTORY_ACTION_CTRL_C = 3,
	BC_HISTORY_ACTION_CTRL_D = 4,
	BC_HISTORY_ACTION_CTRL_E = 5,
	BC_HISTORY_ACTION_CTRL_F = 6,
	BC_HISTORY_ACTION_CTRL_H = 8,
	BC_HISTORY_ACTION_TAB = 9,
	BC_HISTORY_ACTION_LINE_FEED = 10,
	BC_HISTORY_ACTION_CTRL_K = 11,
	BC_HISTORY_ACTION_CTRL_L = 12,
	BC_HISTORY_ACTION_ENTER = 13,
	BC_HISTORY_ACTION_CTRL_N = 14,
	BC_HISTORY_ACTION_CTRL_P = 16,
	BC_HISTORY_ACTION_CTRL_T = 20,
	BC_HISTORY_ACTION_CTRL_U = 21,
	BC_HISTORY_ACTION_CTRL_W = 23,
	BC_HISTORY_ACTION_ESC = 27,
	BC_HISTORY_ACTION_BACKSPACE = 127
} BcHistoryAction;

void bc_history_init(BcHistory *h);

struct BcVm;

BcStatus bc_history_line(BcHistory* h, BcVec* vec, const char *prompt);
BcStatus bc_history_clearScreen(BcHistory* h);
#ifndef NDEBUG
BcStatus bc_history_printKeyCodes(BcHistory* h);
#endif // NDEBUG

extern const char *bc_history_unsupportedTerms[];

#endif // BC_ENABLE_HISTORY

#endif // BC_HISTORY_H
