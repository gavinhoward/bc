/*
 * *****************************************************************************
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2018-2021 Gavin D. Howard and contributors.
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
 * All bc status codes and cross-platform portability.
 *
 */

#ifndef BC_STATUS_H
#define BC_STATUS_H

#include <stdint.h>

#ifdef BC_TEST_OPENBSD
#ifdef __OpenBSD__
#error On OpenBSD without _BSD_SOURCE
#endif // __OpenBSD__
#endif // BC_TEST_OPENBSD

#ifndef BC_ENABLED
#define BC_ENABLED (1)
#endif // BC_ENABLED

#ifndef DC_ENABLED
#define DC_ENABLED (1)
#endif // DC_ENABLED

#if BC_ENABLE_AFL
#ifndef __AFL_HAVE_MANUAL_CONTROL
#error Must compile with afl-clang-fast or afl-clang-lto for fuzzing
#endif // __AFL_HAVE_MANUAL_CONTROL
#endif // BC_ENABLE_AFL

#ifndef BC_ENABLE_MEMCHECK
#define BC_ENABLE_MEMCHECK (0)
#endif // BC_ENABLE_MEMCHECK

#define BC_UNUSED(e) ((void) (e))

#ifndef BC_LIKELY
#define BC_LIKELY(e) (e)
#endif // BC_LIKELY

#ifndef BC_UNLIKELY
#define BC_UNLIKELY(e) (e)
#endif // BC_UNLIKELY

#define BC_ERR(e) BC_UNLIKELY(e)
#define BC_NO_ERR(s) BC_LIKELY(s)

#ifndef BC_DEBUG_CODE
#define BC_DEBUG_CODE (0)
#endif // BC_DEBUG_CODE

#if __STDC_VERSION__ >= 201100L
#include <stdnoreturn.h>
#define BC_NORETURN _Noreturn
#else // __STDC_VERSION__
#define BC_NORETURN
#define BC_MUST_RETURN
#endif // __STDC_VERSION__

#if defined(__clang__) || defined(__GNUC__)
#if defined(__has_attribute)
#if __has_attribute(fallthrough)
#define BC_FALLTHROUGH __attribute__((fallthrough));
#else // __has_attribute(fallthrough)
#define BC_FALLTHROUGH
#endif // __has_attribute(fallthrough)
#else // defined(__has_attribute)
#define BC_FALLTHROUGH
#endif // defined(__has_attribute)
#else // defined(__clang__) || defined(__GNUC__)
#define BC_FALLTHROUGH
#endif // defined(__clang__) || defined(__GNUC__)

#ifdef __GNUC__
#ifdef __OpenBSD__
// The OpenBSD GCC doesn't like inline.
#define inline
#endif // __OpenBSD__
#endif // __GNUC__

// Workarounds for AIX's POSIX incompatibility.
#ifndef SIZE_MAX
#define SIZE_MAX __SIZE_MAX__
#endif // SIZE_MAX
#ifndef UINTMAX_C
#define UINTMAX_C __UINTMAX_C
#endif // UINTMAX_C
#ifndef UINT32_C
#define UINT32_C __UINT32_C
#endif // UINT32_C
#ifndef UINT_FAST32_MAX
#define UINT_FAST32_MAX __UINT_FAST32_MAX__
#endif // UINT_FAST32_MAX
#ifndef UINT16_MAX
#define UINT16_MAX __UINT16_MAX__
#endif // UINT16_MAX
#ifndef SIG_ATOMIC_MAX
#define SIG_ATOMIC_MAX __SIG_ATOMIC_MAX__
#endif // SIG_ATOMIC_MAX

#include <bcl.h>

#if BC_ENABLED

#ifndef BC_DEFAULT_BANNER
#define BC_DEFAULT_BANNER (0)
#endif // BC_DEFAULT_BANNER

#ifndef BC_DEFAULT_SIGINT_RESET
#define BC_DEFAULT_SIGINT_RESET (1)
#endif // BC_DEFAULT_SIGINT_RESET

#ifndef BC_DEFAULT_TTY_MODE
#define BC_DEFAULT_TTY_MODE (1)
#endif // BC_DEFAULT_TTY_MODE

#ifndef BC_DEFAULT_PROMPT
#define BC_DEFAULT_PROMPT BC_DEFAULT_TTY_MODE
#endif // BC_DEFAULT_PROMPT

#endif // BC_ENABLED

#if DC_ENABLED

#ifndef DC_DEFAULT_SIGINT_RESET
#define DC_DEFAULT_SIGINT_RESET (1)
#endif // DC_DEFAULT_SIGINT_RESET

#ifndef DC_DEFAULT_TTY_MODE
#define DC_DEFAULT_TTY_MODE (0)
#endif // DC_DEFAULT_TTY_MODE

#ifndef DC_DEFAULT_HISTORY
#define DC_DEFAULT_HISTORY DC_DEFAULT_TTY_MODE
#endif // DC_DEFAULT_HISTORY

#ifndef DC_DEFAULT_PROMPT
#define DC_DEFAULT_PROMPT DC_DEFAULT_TTY_MODE
#endif // DC_DEFAULT_PROMPT

#endif // DC_ENABLED

typedef enum BcStatus {

	BC_STATUS_SUCCESS = 0,
	BC_STATUS_ERROR_MATH,
	BC_STATUS_ERROR_PARSE,
	BC_STATUS_ERROR_EXEC,
	BC_STATUS_ERROR_FATAL,
	BC_STATUS_EOF,
	BC_STATUS_QUIT,

} BcStatus;

typedef enum BcErr {

	BC_ERR_MATH_NEGATIVE,
	BC_ERR_MATH_NON_INTEGER,
	BC_ERR_MATH_OVERFLOW,
	BC_ERR_MATH_DIVIDE_BY_ZERO,

	BC_ERR_FATAL_ALLOC_ERR,
	BC_ERR_FATAL_IO_ERR,
	BC_ERR_FATAL_FILE_ERR,
	BC_ERR_FATAL_BIN_FILE,
	BC_ERR_FATAL_PATH_DIR,
	BC_ERR_FATAL_OPTION,
	BC_ERR_FATAL_OPTION_NO_ARG,
	BC_ERR_FATAL_OPTION_ARG,

	BC_ERR_EXEC_IBASE,
	BC_ERR_EXEC_OBASE,
	BC_ERR_EXEC_SCALE,
	BC_ERR_EXEC_READ_EXPR,
	BC_ERR_EXEC_REC_READ,
	BC_ERR_EXEC_TYPE,

	BC_ERR_EXEC_STACK,
	BC_ERR_EXEC_STACK_REGISTER,

	BC_ERR_EXEC_PARAMS,
	BC_ERR_EXEC_UNDEF_FUNC,
	BC_ERR_EXEC_VOID_VAL,

	BC_ERR_PARSE_EOF,
	BC_ERR_PARSE_CHAR,
	BC_ERR_PARSE_STRING,
	BC_ERR_PARSE_COMMENT,
	BC_ERR_PARSE_TOKEN,
#if BC_ENABLED
	BC_ERR_PARSE_EXPR,
	BC_ERR_PARSE_EMPTY_EXPR,
	BC_ERR_PARSE_PRINT,
	BC_ERR_PARSE_FUNC,
	BC_ERR_PARSE_ASSIGN,
	BC_ERR_PARSE_NO_AUTO,
	BC_ERR_PARSE_DUP_LOCAL,
	BC_ERR_PARSE_BLOCK,
	BC_ERR_PARSE_RET_VOID,
	BC_ERR_PARSE_REF_VAR,

	BC_ERR_POSIX_NAME_LEN,
	BC_ERR_POSIX_COMMENT,
	BC_ERR_POSIX_KW,
	BC_ERR_POSIX_DOT,
	BC_ERR_POSIX_RET,
	BC_ERR_POSIX_BOOL,
	BC_ERR_POSIX_REL_POS,
	BC_ERR_POSIX_MULTIREL,
	BC_ERR_POSIX_FOR,
	BC_ERR_POSIX_EXP_NUM,
	BC_ERR_POSIX_REF,
	BC_ERR_POSIX_VOID,
	BC_ERR_POSIX_BRACE,
#endif // BC_ENABLED

	BC_ERR_NELEMS,

#if BC_ENABLED
	BC_ERR_POSIX_START = BC_ERR_POSIX_NAME_LEN,
	BC_ERR_POSIX_END = BC_ERR_POSIX_BRACE,
#endif // BC_ENABLED

} BcErr;

// The indices of each category of error in bc_errs[], and used in bc_err_ids[]
// to associate actual errors with their categories.
#define BC_ERR_IDX_MATH (0)
#define BC_ERR_IDX_PARSE (1)
#define BC_ERR_IDX_EXEC (2)
#define BC_ERR_IDX_FATAL (3)
#define BC_ERR_IDX_NELEMS (4)

// If bc is enabled, we add an extra category for POSIX warnings.
#if BC_ENABLED
#define BC_ERR_IDX_WARN (BC_ERR_IDX_NELEMS)
#endif // BC_ENABLED

// BC_JMP is what to use when activating an "exception", i.e., a longjmp(). With
// debug code, it will print the name of the function it jumped from.
#if BC_DEBUG_CODE
#define BC_JMP bc_vm_jmp(__func__)
#else // BC_DEBUG_CODE
#define BC_JMP bc_vm_jmp()
#endif // BC_DEBUG_CODE

/// Returns true if an exception is in flight, false otherwise.
#define BC_SIG_EXC \
	BC_UNLIKELY(vm.status != (sig_atomic_t) BC_STATUS_SUCCESS || vm.sig)

/// Returns true if there is *no* exception in flight, false otherwise.
#define BC_NO_SIG_EXC \
	BC_LIKELY(vm.status == (sig_atomic_t) BC_STATUS_SUCCESS && !vm.sig)

// These two assert whether signals are locked or not, respectively. There are
// non-async-signal-safe functions in bc, and they *must* have signals locked.
// Other functions are expected to *not* have signals locked, for reasons. So
// these are pre-built asserts (no-ops in non-debug mode) that check that
// signals are locked or not.
#ifndef NDEBUG
#define BC_SIG_ASSERT_LOCKED do { assert(vm.sig_lock); } while (0)
#define BC_SIG_ASSERT_NOT_LOCKED do { assert(vm.sig_lock == 0); } while (0)
#else // NDEBUG
#define BC_SIG_ASSERT_LOCKED
#define BC_SIG_ASSERT_NOT_LOCKED
#endif // NDEBUG

/// Locks signals.
#define BC_SIG_LOCK               \
	do {                          \
		BC_SIG_ASSERT_NOT_LOCKED; \
		vm.sig_lock = 1;          \
	} while (0)

/// Unlocks signals. If a signal happened, then this will cause a jump.
#define BC_SIG_UNLOCK           \
	do {                        \
		BC_SIG_ASSERT_LOCKED;   \
		vm.sig_lock = 0;        \
		if (BC_SIG_EXC) BC_JMP; \
	} while (0)

/// Locks signals, regardless of if they are already locked. This is really only
/// used after labels that longjmp() goes to after the jump because the cleanup
/// code must have signals locked, and BC_LONGJMP_CONT will unlock signals if it
/// doesn't jump.
#define BC_SIG_MAYLOCK   \
	do {                 \
		vm.sig_lock = 1; \
	} while (0)

/// Unlocks signals, regardless of if they were already unlocked. If a signal
/// happened, then this will cause a jump.
#define BC_SIG_MAYUNLOCK        \
	do {                        \
		vm.sig_lock = 0;        \
		if (BC_SIG_EXC) BC_JMP; \
	} while (0)

/// Locks signals, but stores the old lock state, to be restored later by
/// BC_SIG_TRYUNLOCK.
#define BC_SIG_TRYLOCK(v) \
	do {                  \
		v = vm.sig_lock;  \
		vm.sig_lock = 1;  \
	} while (0)

/// Restores the previous state of a signal lock, and if it is now unlocked,
/// initiates an exception/jump.
#define BC_SIG_TRYUNLOCK(v)             \
	do {                                \
		vm.sig_lock = (v);              \
		if (!(v) && BC_SIG_EXC) BC_JMP; \
	} while (0)

/// Sets a jump, and sets it up as well so that if a longjmp() happens, bc will
/// immediately goto a label where some cleanup code is. This one assumes that
/// signals are not locked and will lock them, set the jump, and unlock them.
/// Setting the jump also includes pushing the jmp_buf onto the jmp_buf stack.
#define BC_SETJMP(l)                     \
	do {                                 \
		sigjmp_buf sjb;                  \
		BC_SIG_LOCK;                     \
		if (sigsetjmp(sjb, 0)) {         \
			assert(BC_SIG_EXC);          \
			goto l;                      \
		}                                \
		bc_vec_push(&vm.jmp_bufs, &sjb); \
		BC_SIG_UNLOCK;                   \
	} while (0)

/// Sets a jump like BC_SETJMP, but unlike BC_SETJMP, it assumes signals are
/// locked and will just set the jump.
#define BC_SETJMP_LOCKED(l)               \
	do {                                  \
		sigjmp_buf sjb;                   \
		BC_SIG_ASSERT_LOCKED;             \
		if (sigsetjmp(sjb, 0)) {          \
			assert(BC_SIG_EXC);           \
			goto l;                       \
		}                                 \
		bc_vec_push(&vm.jmp_bufs, &sjb);  \
	} while (0)

/// Used after cleanup labels set by BC_SETJMP and BC_SETJMP_LOCKED to jump to
/// the next place. This is what continues the stack unwinding.
#define BC_LONGJMP_CONT                             \
	do {                                            \
		BC_SIG_ASSERT_LOCKED;                       \
		if (!vm.sig_pop) bc_vec_pop(&vm.jmp_bufs);  \
		BC_SIG_UNLOCK;                              \
	} while (0)

/// Unsets a jump. It always assumes signals are locked. This basically just
/// pops a jmp_buf off of the stack of jmp_bufs, and since the jump mechanism
/// always jumps to the location at the top of the stack, this effectively
/// undoes a setjmp().
#define BC_UNSETJMP               \
	do {                          \
		BC_SIG_ASSERT_LOCKED;     \
		bc_vec_pop(&vm.jmp_bufs); \
	} while (0)

/// Stops a stack unwinding. Technically, a stack unwinding needs to be done
/// manually, but it will always be done unless certain flags are cleared. This
/// clears the flags.
#define BC_LONGJMP_STOP    \
	do {                   \
		vm.sig_pop = 0;    \
		vm.sig = 0;        \
	} while (0)

// Various convenience macros for calling the bc's error handling routine.
#if BC_ENABLE_LIBRARY
#define bc_error(e, l, ...) (bc_vm_handleError((e)))
#define bc_err(e) (bc_vm_handleError((e)))
#define bc_verr(e, ...) (bc_vm_handleError((e)))
#else // BC_ENABLE_LIBRARY
#define bc_error(e, l, ...) (bc_vm_handleError((e), (l), __VA_ARGS__))
#define bc_err(e) (bc_vm_handleError((e), 0))
#define bc_verr(e, ...) (bc_vm_handleError((e), 0, __VA_ARGS__))
#endif // BC_ENABLE_LIBRARY

/// Returns true if status s is an error, false otherwise.
#define BC_STATUS_IS_ERROR(s) \
	((s) >= BC_STATUS_ERROR_MATH && (s) <= BC_STATUS_ERROR_FATAL)

// Convenience macros that can be placed at the beginning and exits of functions
// for easy marking of where functions are entered and exited.
#if BC_DEBUG_CODE
#define BC_FUNC_ENTER                                              \
	do {                                                           \
		size_t bc_func_enter_i;                                    \
		for (bc_func_enter_i = 0; bc_func_enter_i < vm.func_depth; \
		     ++bc_func_enter_i)                                    \
		{                                                          \
			bc_file_puts(&vm.ferr, bc_flush_none, "  ");           \
		}                                                          \
		vm.func_depth += 1;                                        \
		bc_file_printf(&vm.ferr, "Entering %s\n", __func__);       \
		bc_file_flush(&vm.ferr, bc_flush_none);                    \
	} while (0);

#define BC_FUNC_EXIT                                               \
	do {                                                           \
		size_t bc_func_enter_i;                                    \
		vm.func_depth -= 1;                                        \
		for (bc_func_enter_i = 0; bc_func_enter_i < vm.func_depth; \
		     ++bc_func_enter_i)                                    \
		{                                                          \
			bc_file_puts(&vm.ferr, bc_flush_none, "  ");           \
		}                                                          \
		bc_file_printf(&vm.ferr, "Leaving %s\n", __func__);        \
		bc_file_flush(&vm.ferr, bc_flush_none);                    \
	} while (0);
#else // BC_DEBUG_CODE
#define BC_FUNC_ENTER
#define BC_FUNC_EXIT
#endif // BC_DEBUG_CODE

#endif // BC_STATUS_H
