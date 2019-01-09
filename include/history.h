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
 * *****************************************************************************
 *
 * Definitions for line history.
 *
 */

#ifndef BC_HISTORY_H
#define BC_HISTORY_H

#if BC_ENABLE_HISTORY

#ifdef _WIN32
#error History is not supported on Windows.
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include <termios.h>
#include <unistd.h>

#include <status.h>
#include <vector.h>

#define BC_HISTORY_DEF_COLS (80)
#define BC_HISTORY_MAX_LEN (128)
#define BC_HISTORY_MAX_LINE (4095)

#define BC_HISTORY_BUF_LEN(h) ((h)->buf.len - 1)
#define BC_HISTORY_WRITE(s, n) (write(STDERR_FILENO, (s), (n)) != (ssize_t) (n))

#define BC_HISTORY_NEXT (false)
#define BC_HISTORY_PREV (true)

#ifndef NDEBUG
#define lndebug(...)                                               \
	do {                                                           \
		if (bc_history_debug_fp == NULL) {                         \
			bc_history_debug_fp = fopen("/tmp/lndebug.txt","a");   \
			fprintf(bc_history_debug_fp,                           \
			       "[%zu %zu %zu] p: %d, rows: %d, "               \
			       "rpos: %d, max: %zu, oldmax: %d\n",             \
			       l->len, l->pos, l->oldcolpos, plen, rows, rpos, \
			       l->maxrows, old_rows);                          \
		}                                                          \
		fprintf(bc_history_debug_fp, ", " __VA_ARGS__);            \
		fflush(bc_history_debug_fp);                               \
	} while (0)
#else // NDEBUG
#define lndebug(fmt, ...)
#endif // NDEBUG

typedef enum BcHistoryAction {

	BC_ACTION_NULL = 0,
	BC_ACTION_CTRL_A = 1,
	BC_ACTION_CTRL_B = 2,
	BC_ACTION_CTRL_C = 3,
	BC_ACTION_CTRL_D = 4,
	BC_ACTION_CTRL_E = 5,
	BC_ACTION_CTRL_F = 6,
	BC_ACTION_CTRL_H = 8,
	BC_ACTION_LINE_FEED = 10,
	BC_ACTION_CTRL_K = 11,
	BC_ACTION_CTRL_L = 12,
	BC_ACTION_ENTER = 13,
	BC_ACTION_CTRL_N = 14,
	BC_ACTION_CTRL_P = 16,
	BC_ACTION_CTRL_T = 20,
	BC_ACTION_CTRL_U = 21,
	BC_ACTION_CTRL_W = 23,
	BC_ACTION_CTRL_Z = 26,
	BC_ACTION_ESC = 27,
	BC_ACTION_BACKSPACE =  127

} BcHistoryAction;

/**
 * This represents the state during line editing. We pass this state
 * to functions implementing specific editing functionalities.
 */
typedef struct BcHistory {

	/// Edited line buffer.
	BcVec buf;

	/// The history.
	BcVec history;

	/// A temporary buffer for refresh. Using this
	/// prevents an allocation on every refresh.
	BcVec refresh;

	/// Prompt to display.
	const char *prompt;

	/// Prompt length.
	size_t plen;

	/// Current cursor position.
	size_t pos;

	/// Previous refresh cursor column position.
	size_t oldcolpos;

	/// Number of columns in terminal.
	size_t cols;

	/// The history index we are currently editing.
	size_t idx;

	/// The original terminal state.
	struct termios orig_termios;

	/// Whether we are in rawmode.
	bool rawMode;

	/// Whether the terminal is bad.
	bool badTerm;

} BcHistory;

BcStatus bc_history_line(BcHistory *h, BcVec *vec, const char *prompt);
void bc_history_add(BcHistory *h, char *line);

void bc_history_init(BcHistory *h);
void bc_history_free(BcHistory *h);

extern const char *bc_history_bad_terms[];
extern const char bc_history_ctrlc[];
extern const unsigned long bc_history_wchars[][2];
extern const size_t bc_history_wchars_len;
extern const unsigned long bc_history_combo_chars[];
extern const size_t bc_history_combo_chars_len;
#ifndef NDEBUG
extern FILE *bc_history_debug_fp;
BcStatus bc_history_printKeyCodes(BcHistory* l);
#endif // NDEBUG

#endif // BC_ENABLE_HISTORY

#endif // BC_HISTORY_H
