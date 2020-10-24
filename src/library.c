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
 * The public functions for libbc.
 *
 */

#if BC_ENABLE_LIBRARY

#include <setjmp.h>

#include <string.h>

#include <bc.h>

#include "library.h"
#include "num.h"
#include "vm.h"

void libbc_handleSignal(void) {

	// Signal already in flight.
	if (vm.sig) return;

	vm.sig = 1;

	assert(vm.jmp_bufs.len);

	if (!vm.sig_lock && vm.running) BC_VM_JMP;
}

BcError libbc_init(bool abortOnFatal) {

	BcError e = BC_ERROR_SUCCESS;

	BC_FUNC_HEADER_LOCK(err);

	memset(&vm, 0, sizeof(BcVm));

	vm.jmp_bufs.v = NULL;
	vm.out.v = NULL;

	vm.abrt = abortOnFatal;

	bc_vm_init();

	bc_vec_init(&vm.jmp_bufs, sizeof(sigjmp_buf), NULL);
	bc_vec_init(&vm.out, sizeof(uchar), NULL);
	bc_rand_init(&vm.rng);

err:
	if (vm.err) {
		if (vm.out.v != NULL) bc_vec_free(vm.out.v);
		if (vm.jmp_bufs.v != NULL) bc_vec_free(vm.jmp_bufs.v);
	}

	BC_FUNC_FOOTER_UNLOCK(e);

	return e;
}

void libbc_dtor(void) {

	BC_SIG_LOCK;

	bc_vm_shutdown();

#ifndef NDEBUG
	bc_rand_free(&vm.rng);
	bc_vec_free(&vm.out);
	bc_vec_free(&vm.jmp_bufs);
#endif // NDEBUG

	BC_SIG_UNLOCK;
}

void libbc_abortOnFatalError(bool abrt) {
	vm.abrt = abrt;
}

BcError libbc_num_init(BcNum *restrict n) {
	return libbc_num_initReq(n, BC_NUM_DEF_SIZE);
}

BcError libbc_num_initReq(BcNum *restrict n, size_t req) {

	BcError e = BC_ERROR_SUCCESS;

	BC_FUNC_HEADER_LOCK(err);

	bc_num_init(n, req);

err:
	BC_FUNC_FOOTER_UNLOCK(e);
	return e;
}

BcError libbc_num_copy(BcNum *d, const BcNum *s) {

	BcError e = BC_ERROR_SUCCESS;

	assert(d->num != NULL && s->num != NULL);

	BC_FUNC_HEADER_LOCK(err);

	bc_num_copy(d, s);

err:
	BC_FUNC_FOOTER_UNLOCK(e);
	return e;
}

BcError libbc_num_copy_create(BcNum *d, const BcNum *s) {

	BcError e = BC_ERROR_SUCCESS;

	assert(s->num != NULL);

	BC_FUNC_HEADER_LOCK(err);

	bc_num_createCopy(d, s);

err:
	BC_FUNC_FOOTER_UNLOCK(e);
	return e;
}

void libbc_num_clear(BcNum *restrict n) {
	assert(n->num != NULL);
	bc_num_clear(n);
}

void libbc_num_free(BcNum *num) {
	BC_SIG_LOCK;
	bc_num_free(num);
	BC_SIG_UNLOCK;
}

void libbc_num_dtor(void *num) {
	BC_SIG_LOCK;
	bc_num_free(num);
	BC_SIG_UNLOCK;
}

size_t libbc_num_scale(const BcNum *restrict n) {
	assert(n->num != NULL);
	return bc_num_scale(n);
}

size_t libbc_num_len(const BcNum *restrict n) {
	assert(n->num != NULL);
	return bc_num_len(n);
}

BcError libbc_num_bigdig(const BcNum *restrict n, BcBigDig *result) {

	BcError e = BC_ERROR_SUCCESS;

	assert(n != NULL);
	assert(n->num != NULL);
	assert(result != NULL);

	BC_FUNC_HEADER_LOCK(err);

	bc_num_bigdig(n, result);

err:
	BC_FUNC_FOOTER_UNLOCK(e);
	return e;
}

BcError libbc_num_bigdig2num_create(BcNum *n, BcBigDig val) {

	BcError e = BC_ERROR_SUCCESS;

	assert(n != NULL);

	BC_FUNC_HEADER_LOCK(err);

	bc_num_createFromBigdig(n, val);

err:
	BC_FUNC_FOOTER_UNLOCK(e);
	return e;
}

BcError libbc_num_bigdig2num(BcNum *restrict n, BcBigDig val) {

	BcError e = BC_ERROR_SUCCESS;

	assert(n != NULL);
	assert(n->num != NULL);

	BC_FUNC_HEADER_LOCK(err);

	bc_num_bigdig2num(n, val);

err:
	BC_FUNC_FOOTER_UNLOCK(e);
	return e;
}

static BcError libbc_num_binary_create(BcNum *a, BcNum *b, BcNum *c,
                                       size_t scale, BcNumBinaryOp op,
                                       BcNumBinaryOpReq req)
{
	BcError e = BC_ERROR_SUCCESS;

	assert(a != NULL && b != NULL && c != NULL);
	assert(a->num != NULL && b->num != NULL);

	bc_num_clear(c);

	BC_FUNC_HEADER_LOCK(err);

	bc_num_init(c, req(a, b, scale));

	BC_SIG_UNLOCK;

	op(a, b, c, scale);

err:
	BC_SIG_MAYLOCK;

	if (vm.err && c->num != NULL) bc_num_free(c);

	BC_FUNC_FOOTER(e);

	return e;
}

static BcError libbc_num_binary(BcNum *a, BcNum *b, BcNum *c, size_t scale,
                                BcNumBinaryOp op)
{
	BcError e = BC_ERROR_SUCCESS;

	assert(a != NULL && b != NULL && c != NULL);
	assert(a->num != NULL && b->num != NULL);

	BC_FUNC_HEADER(err);

	op(a, b, c, scale);

err:
	BC_SIG_MAYLOCK;
	BC_FUNC_FOOTER(e);
	return e;
}

BcError libbc_num_add_create(BcNum *a, BcNum *b, BcNum *c, size_t scale) {
	return libbc_num_binary_create(a, b, c, scale, bc_num_add, bc_num_addReq);
}

BcError libbc_num_add(BcNum *a, BcNum *b, BcNum *c, size_t scale) {
	return libbc_num_binary(a, b, c, scale, bc_num_add);
}

BcError libbc_num_sub_create(BcNum *a, BcNum *b, BcNum *c, size_t scale) {
	return libbc_num_binary_create(a, b, c, scale, bc_num_sub, bc_num_addReq);
}

BcError libbc_num_sub(BcNum *a, BcNum *b, BcNum *c, size_t scale) {
	return libbc_num_binary(a, b, c, scale, bc_num_sub);
}

BcError libbc_num_mul_create(BcNum *a, BcNum *b, BcNum *c, size_t scale) {
	return libbc_num_binary_create(a, b, c, scale, bc_num_mul, bc_num_mulReq);
}

BcError libbc_num_mul(BcNum *a, BcNum *b, BcNum *c, size_t scale) {
	return libbc_num_binary(a, b, c, scale, bc_num_mul);
}

BcError libbc_num_div_create(BcNum *a, BcNum *b, BcNum *c, size_t scale) {
	return libbc_num_binary_create(a, b, c, scale, bc_num_div, bc_num_divReq);
}

BcError libbc_num_div(BcNum *a, BcNum *b, BcNum *c, size_t scale) {
	return libbc_num_binary(a, b, c, scale, bc_num_div);
}

BcError libbc_num_mod_create(BcNum *a, BcNum *b, BcNum *c, size_t scale) {
	return libbc_num_binary_create(a, b, c, scale, bc_num_mod, bc_num_divReq);
}

BcError libbc_num_mod(BcNum *a, BcNum *b, BcNum *c, size_t scale) {
	return libbc_num_binary(a, b, c, scale, bc_num_mod);
}

BcError libbc_num_pow_create(BcNum *a, BcNum *b, BcNum *c, size_t scale) {
	return libbc_num_binary_create(a, b, c, scale, bc_num_pow, bc_num_powReq);
}

BcError libbc_num_pow(BcNum *a, BcNum *b, BcNum *c, size_t scale) {
	return libbc_num_binary(a, b, c, scale, bc_num_pow);
}

#if BC_ENABLE_EXTRA_MATH
BcError libbc_num_places_create(BcNum *a, BcNum *b, BcNum *c, size_t scale) {
	return libbc_num_binary_create(a, b, c, scale, bc_num_places,
	                               bc_num_placesReq);
}

BcError libbc_num_places(BcNum *a, BcNum *b, BcNum *c, size_t scale) {
	return libbc_num_binary(a, b, c, scale, bc_num_places);
}

BcError libbc_num_lshift_create(BcNum *a, BcNum *b, BcNum *c, size_t scale) {
	return libbc_num_binary_create(a, b, c, scale, bc_num_lshift,
	                               bc_num_placesReq);
}

BcError libbc_num_lshift(BcNum *a, BcNum *b, BcNum *c, size_t scale) {
	return libbc_num_binary(a, b, c, scale, bc_num_lshift);
}

BcError libbc_num_rshift_create(BcNum *a, BcNum *b, BcNum *c, size_t scale) {
	return libbc_num_binary_create(a, b, c, scale, bc_num_rshift,
	                               bc_num_placesReq);
}

BcError libbc_num_rshift(BcNum *a, BcNum *b, BcNum *c, size_t scale) {
	return libbc_num_binary(a, b, c, scale, bc_num_lshift);
}
#endif // BC_ENABLE_EXTRA_MATH

static BcError libbc_num_sqrtHelper(BcNum *restrict a, BcNum *restrict b,
                                    size_t scale, BcNumSqrtOp op)
{
	BcError e = BC_ERROR_SUCCESS;

	assert(a != NULL && b != NULL && a != b);
	assert(a->num != NULL && b->num != NULL);

	BC_FUNC_HEADER(err);

	op(a, b, scale);

err:
	BC_SIG_MAYLOCK;
	BC_FUNC_FOOTER(e);
	return e;
}

BcError libbc_num_sqrt_create(BcNum *restrict a, BcNum *restrict b,
                              size_t scale)
{
	return libbc_num_sqrtHelper(a, b, scale, bc_num_sqrt);
}

BcError libbc_num_sqrt(BcNum *restrict a, BcNum *restrict b, size_t scale)
{
	return libbc_num_sqrtHelper(a, b, scale, bc_num_sr);
}

BcError libbc_num_divmod_create(BcNum *a, BcNum *b,
                                BcNum *c, BcNum *d, size_t scale)
{
	BcError e = BC_ERROR_SUCCESS;
	size_t req;

	assert(a != NULL && b != NULL && c != NULL && d != NULL);
	assert(a != c && a != d && b != c && b != d && c != d);
	assert(a->num != NULL && b->num != NULL);

	bc_num_clear(c);
	bc_num_clear(d);

	req = bc_num_divReq(a, b, scale);

	BC_FUNC_HEADER_LOCK(err);

	bc_num_init(c, req);
	bc_num_init(d, req);

	BC_SIG_UNLOCK;

	bc_num_divmod(a, b, c, d, scale);

err:
	BC_SIG_MAYLOCK;

	if (vm.err) {
		if(c->num != NULL) bc_num_free(c);
		if(d->num != NULL) bc_num_free(d);
	}

	BC_FUNC_FOOTER(e);

	return e;
}

BcError libbc_num_divmod(BcNum *a, BcNum *b, BcNum *c, BcNum *d, size_t scale) {

	BcError e = BC_ERROR_SUCCESS;

	assert(a != NULL && b != NULL && c != NULL && d != NULL);
	assert(a != c && a != d && b != c && b != d && c != d);
	assert(a->num != NULL && b->num != NULL);
	assert(c->num != NULL && d->num != NULL);

	BC_FUNC_HEADER(err);

	bc_num_divmod(a, b, c, d, scale);

err:
	BC_SIG_MAYLOCK;
	BC_FUNC_FOOTER(e);
	return e;
}

BcError libbc_num_modexp_create(BcNum *a, BcNum *b, BcNum *c, BcNum *restrict d)
{
	BcError e = BC_ERROR_SUCCESS;
	size_t req;

	assert(a != NULL && b != NULL && c != NULL && d != NULL);
	assert(a != c && a != d && b != c && b != d && c != d);
	assert(a->num != NULL && b->num != NULL && c->num != NULL);

	bc_num_clear(d);

	req = bc_num_divReq(a, c, 0);

	BC_FUNC_HEADER_LOCK(err);

	bc_num_init(d, req);

	BC_SIG_UNLOCK;

	bc_num_modexp(a, b, c, d);

err:
	BC_SIG_MAYLOCK;

	if (vm.err && d->num != NULL) bc_num_free(d);

	BC_FUNC_FOOTER(e);

	return e;
}

BcError libbc_num_modexp(BcNum *a, BcNum *b, BcNum *c, BcNum *restrict d) {

	BcError e = BC_ERROR_SUCCESS;

	assert(a != NULL && b != NULL && c != NULL && d != NULL);
	assert(a != d && b != d && c != d);
	assert(a->num != NULL && b->num != NULL && c->num != NULL);
	assert(d->num != NULL);

	BC_FUNC_HEADER(err);

	bc_num_modexp(a, b, c, d);

err:
	BC_SIG_MAYLOCK;
	BC_FUNC_FOOTER(e);
	return e;
}

size_t libbc_num_addReq(const BcNum* a, const BcNum* b, size_t scale) {
	return bc_num_addReq(a, b, scale);
}

size_t libbc_num_mulReq(const BcNum *a, const BcNum *b, size_t scale) {
	return bc_num_mulReq(a, b, scale);
}

size_t libbc_num_divReq(const BcNum *a, const BcNum *b, size_t scale) {
	return bc_num_divReq(a, b, scale);
}

size_t libbc_num_powReq(const BcNum *a, const BcNum *b, size_t scale) {
	return bc_num_powReq(a, b, scale);
}

#if BC_ENABLE_EXTRA_MATH
size_t libbc_num_placesReq(const BcNum *a, const BcNum *b, size_t scale) {
	return bc_num_placesReq(a, b, scale);
}
#endif // BC_ENABLE_EXTRA_MATH

BcError libbc_num_setScale(struct BcNum *restrict n, size_t scale) {

	BcError e = BC_ERROR_SUCCESS;

	assert(n != NULL && n->num != NULL);

	BC_FUNC_HEADER(err);

	if (scale > n->scale) bc_num_extend(n, scale - n->scale);
	else if (scale < n->scale) bc_num_truncate(n, n->scale - scale);

err:
	BC_SIG_MAYLOCK;
	BC_FUNC_FOOTER(e);
	return e;
}

ssize_t libbc_num_cmp(const struct BcNum *a, const struct BcNum *b) {
	return bc_num_cmp(a, b);
}

void libbc_num_one(struct BcNum *restrict n) {
	bc_num_one(n);
}

ssize_t libbc_num_cmpZero(const struct BcNum *n) {
	return bc_num_cmpZero(n);
}

BcError libbc_num_parse_create(BcNum *restrict n, const char *restrict val,
                               BcBigDig base)
{
	BcError e = BC_ERROR_SUCCESS;

	assert(n != NULL && n->num != NULL);
	assert(val != NULL);

	bc_num_clear(n);

	BC_FUNC_HEADER_LOCK(err);

	bc_num_init(n, BC_NUM_DEF_SIZE);

	BC_SIG_UNLOCK;

	bc_num_parse(n, val, base);

err:
	BC_SIG_MAYLOCK;

	if (vm.err && n->num != NULL) bc_num_free(n);

	BC_FUNC_FOOTER(e);

	return e;
}

BcError libbc_num_parse(BcNum *restrict n, const char *restrict val,
                        BcBigDig base)
{
	BcError e = BC_ERROR_SUCCESS;

	assert(n != NULL && n->num != NULL);
	assert(val != NULL);

	BC_FUNC_HEADER(err);

	bc_num_parse(n, val, base);

err:
	BC_SIG_MAYLOCK;
	BC_FUNC_FOOTER(e);
	return e;
}

BcError libbc_num_string(BcNum *restrict n, BcBigDig base, char** str) {

	BcError e = BC_ERROR_SUCCESS;

	assert(n != NULL && n->num != NULL);
	assert(str != NULL);

	*str = NULL;

	BC_FUNC_HEADER(err);

	bc_num_print(n, base, false);

	*str = bc_vm_strdup(vm.out.v);

err:
	BC_SIG_MAYLOCK;

	if (vm.err && *str != NULL) free(*str);

	BC_FUNC_FOOTER(e);

	return e;
}

#if BC_ENABLE_EXTRA_MATH
BcError libbc_num_irand_create(const BcNum *restrict a, BcNum *restrict b) {

	BcError e = BC_ERROR_SUCCESS;

	assert(a != NULL && a->num != NULL);
	assert(b != NULL);

	bc_num_clear(b);

	BC_FUNC_HEADER_LOCK(err);

	bc_num_init(b, BC_NUM_DEF_SIZE);

	BC_SIG_UNLOCK;

	bc_num_irand(a, b, &vm.rng);

err:
	BC_SIG_MAYLOCK;

	if (vm.err && b->num != NULL) bc_num_free(b);

	BC_FUNC_FOOTER(e);

	return e;
}

BcError libbc_num_irand(const BcNum *restrict a, BcNum *restrict b) {

	BcError e = BC_ERROR_SUCCESS;

	assert(a != NULL && a->num != NULL);
	assert(b != NULL && b->num != NULL);

	BC_FUNC_HEADER(err);

	bc_num_irand(a, b, &vm.rng);

err:
	BC_SIG_MAYLOCK;
	BC_FUNC_FOOTER(e);
	return e;
}

static void libbc_num_frandHelper(BcNum *restrict b, size_t places) {

	BcNum exp, pow, ten;
	BcDig exp_digs[BC_NUM_BIGDIG_LOG10];
	BcDig ten_digs[BC_NUM_BIGDIG_LOG10];

	bc_num_setup(&exp, exp_digs, BC_NUM_BIGDIG_LOG10);
	bc_num_setup(&ten, ten_digs, BC_NUM_BIGDIG_LOG10);

	ten.num[0] = 10;
	ten.len = 1;

	bc_num_bigdig2num(&exp, (unsigned long) places);

	bc_num_clear(&pow);

	BC_SIG_LOCK;

	BC_SETJMP_LOCKED(err);

	bc_num_init(&pow, bc_num_powReq(&ten, &exp, 0));

	BC_SIG_UNLOCK;

	bc_num_pow(&ten, &exp, &pow, 0);

	bc_num_irand(&pow, b, &vm.rng);

	bc_num_shiftRight(b, places);

err:
	BC_SIG_MAYLOCK;
	bc_num_free(&pow);
	BC_LONGJMP_CONT;
}

BcError libbc_num_frand_create(BcNum *restrict b, size_t places) {

	BcError e = BC_ERROR_SUCCESS;

	assert(b != NULL);

	bc_num_clear(b);

	BC_FUNC_HEADER_LOCK(err);

	bc_num_init(b, BC_NUM_DEF_SIZE);

	BC_SIG_UNLOCK;

	libbc_num_frandHelper(b, places);

err:
	BC_SIG_MAYLOCK;

	if (vm.err && b->num != NULL) bc_num_free(b);

	BC_FUNC_FOOTER(e);

	return e;
}

BcError libbc_num_frand(BcNum *restrict b, size_t places) {

	BcError e = BC_ERROR_SUCCESS;

	assert(b != NULL && b->num != NULL);

	BC_FUNC_HEADER(err);

	libbc_num_frandHelper(b, places);

err:
	BC_SIG_MAYLOCK;
	BC_FUNC_FOOTER(e);
	return e;
}

static void libbc_num_ifrandHelper(BcNum *restrict a, BcNum *restrict b,
                                   size_t places)
{
	BcNum ir, fr;

	bc_num_clear(&ir);
	bc_num_clear(&fr);

	BC_SIG_LOCK;

	BC_SETJMP_LOCKED(err);

	bc_num_init(&ir, BC_NUM_DEF_SIZE);
	bc_num_init(&fr, BC_NUM_DEF_SIZE);

	BC_SIG_UNLOCK;

	bc_num_irand(a, &ir, &vm.rng);
	libbc_num_frand(&fr, places);

	bc_num_add(&ir, &fr, b, 0);

err:
	BC_SIG_MAYLOCK;
	bc_num_free(&fr);
	bc_num_free(&ir);
	BC_LONGJMP_CONT;
}

BcError libbc_num_ifrand_create(BcNum *restrict a, BcNum *restrict b,
                                size_t places)
{
	BcError e = BC_ERROR_SUCCESS;

	assert(a != NULL && a->num != NULL);
	assert(b != NULL);

	bc_num_clear(b);

	BC_FUNC_HEADER_LOCK(err);

	bc_num_init(b, BC_NUM_DEF_SIZE);

	BC_SIG_UNLOCK;

	libbc_num_ifrandHelper(a, b, places);

err:
	BC_SIG_MAYLOCK;

	if (vm.err && b->num != NULL) bc_num_free(b);

	BC_FUNC_FOOTER(e);

	return e;
}

BcError libbc_num_ifrand(BcNum *restrict a, BcNum *restrict b, size_t places) {

	BcError e = BC_ERROR_SUCCESS;

	assert(a != NULL && a->num != NULL);
	assert(b != NULL && b->num != NULL);

	BC_FUNC_HEADER(err);

	libbc_num_ifrandHelper(a, b, places);

err:
	BC_SIG_MAYLOCK;
	BC_FUNC_FOOTER(e);
	return e;
}

BcError libbc_num_seedWithNum(const BcNum *restrict n) {

	BcError e = BC_ERROR_SUCCESS;

	assert(n != NULL && n->num != NULL);

	BC_FUNC_HEADER(err);

	bc_num_rng(n, &vm.rng);

err:
	BC_SIG_MAYLOCK;
	BC_FUNC_FOOTER(e);
	return e;
}

BcError libbc_num_seedWithUlongs(unsigned long state1, unsigned long state2,
                                 unsigned long inc1, unsigned long inc2)
{
	BcError e = BC_ERROR_SUCCESS;

	BC_FUNC_HEADER(err);

	bc_rand_seed(&vm.rng, state1, state2, inc1, inc2);

err:
	BC_SIG_MAYLOCK;
	BC_FUNC_FOOTER(e);
	return e;
}

BcError libbc_num_reseed(void) {

	BcError e = BC_ERROR_SUCCESS;

	BC_FUNC_HEADER(err);

	bc_rand_srand(bc_vec_top(&vm.rng.v));

err:
	BC_SIG_MAYLOCK;
	BC_FUNC_FOOTER(e);
	return e;
}

BcError libbc_num_seed2num_create(BcNum *restrict n) {

	BcError e = BC_ERROR_SUCCESS;

	assert(n != NULL);

	bc_num_clear(n);

	BC_FUNC_HEADER_LOCK(err);

	bc_num_init(n, BC_NUM_DEF_SIZE);

	BC_SIG_UNLOCK;

	bc_num_createFromRNG(n, bc_vec_top(&vm.rng.v));

err:
	BC_SIG_MAYLOCK;

	if (vm.err && n->num != NULL) bc_num_free(n);

	BC_FUNC_FOOTER(e);

	return e;
}

BcError libbc_num_seed2num(BcNum *restrict n) {

	BcError e = BC_ERROR_SUCCESS;

	assert(n != NULL && n->num != NULL);

	BC_FUNC_HEADER(err);

	bc_num_createFromRNG(n, bc_vec_top(&vm.rng.v));

err:
	BC_SIG_MAYLOCK;
	BC_FUNC_FOOTER(e);
	return e;
}

BcRandInt libbc_rand_int(void) {
	return (BcRandInt) bc_rand_int(&vm.rng);
}

BcRandInt libbc_rand_bounded(BcRandInt bound) {
	return (BcRandInt) bc_rand_bounded(&vm.rng, (BcRand) bound);
}

#endif // BC_ENABLE_EXTRA_MATH

#endif // BC_ENABLE_LIBRARY
