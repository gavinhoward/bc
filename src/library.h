/*
 * *****************************************************************************
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2018-2020 Gavin D. Howard and contributors.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * *****************************************************************************
 *
 * The private header for libbc.
 *
 */

#ifndef LIBBC_PRIVATE_H
#define LIBBC_PRIVATE_H

#include <bc.h>

#include "num.h"

#define BC_FUNC_HEADER_LOCK(l) \
	do {                       \
		BC_SIG_LOCK;           \
		vm.running = 0;        \
		BC_SETJMP_LOCKED(l);   \
	} while (0)

#define BC_FUNC_FOOTER_UNLOCK(e) \
	do {                         \
		BC_SIG_ASSERT_LOCKED;    \
		e = vm.err;              \
		vm.running = 0;          \
		BC_LONGJMP_STOP;         \
		BC_UNSETJMP;             \
		vm.sig_lock = 0;         \
	} while (0)

#define BC_FUNC_HEADER(l) \
	do {                  \
		vm.running = 0;   \
		BC_SETJMP(l);     \
	} while (0)

#define BC_FUNC_FOOTER(e) \
	do {                  \
		e = vm.err;       \
		vm.running = 0;   \
		BC_LONGJMP_STOP;  \
		BC_UNSETJMP;      \
		vm.sig_lock = 0;  \
	} while (0)

#define BC_FUNC_RESETJMP(l)   \
	do {                      \
		BC_SIG_ASSERT_LOCKED; \
		BC_UNSETJMP;          \
		BC_SETJMP_LOCKED(l);  \
	} while (0)

#endif // LIBBC_PRIVATE_H

typedef void (*BcNumSqrtOp)(BcNum*, BcNum*, size_t scale);
