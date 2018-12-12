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

#include <stddef.h>

#if BC_ENABLE_HISTORY

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
	BC_ACTION_ESC = 27,
	BC_ACTION_BACKSPACE =  127
} BcHistoryAction;

char *linenoise(const char *prompt);
int linenoiseHistoryAdd(const char *line);
int linenoiseHistorySetMaxLen(int len);
int linenoiseHistoryGetMaxLen(void);
int linenoiseHistoryCopy(char** dest, int destlen);
void linenoiseClearScreen(void);
void linenoisePrintKeyCodes(void);

typedef size_t (linenoisePrevCharLen)(const char*, size_t, size_t, size_t*);
typedef size_t (linenoiseNextCharLen)(const char*, size_t, size_t, size_t*);
typedef size_t (linenoiseReadCode)(int, char*, size_t, BcHistoryAction*);

void linenoiseSetEncodingFunctions(
    linenoisePrevCharLen *prevCharLenFunc,
    linenoiseNextCharLen *nextCharLenFunc,
    linenoiseReadCode *readCodeFunc);

#endif // BC_ENABLE_HISTORY

#endif // BC_HISTORY_H
