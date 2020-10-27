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

static void libbc_num_destruct(void *num);

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

	vm.jmp_bufs.v = NULL;
	vm.out.v = NULL;

	vm.scale = 0;
	vm.ibase = 10;
	vm.obase= 10;

	vm.abrt = abortOnFatal;

	bc_vm_init();

	bc_vec_init(&vm.nums, sizeof(BcNum), libbc_num_destruct);
	bc_vec_init(&vm.free_nums, sizeof(BcNumber), NULL);

	bc_vec_init(&vm.jmp_bufs, sizeof(sigjmp_buf), NULL);
	bc_vec_init(&vm.out, sizeof(uchar), NULL);
	bc_rand_init(&vm.rng);

err:
	if (BC_ERR(vm.err)) {
		if (vm.out.v != NULL) bc_vec_free(vm.out.v);
		if (vm.jmp_bufs.v != NULL) bc_vec_free(vm.jmp_bufs.v);
	}

	BC_FUNC_FOOTER_UNLOCK(e);

	return e;
}

void libbc_dtor(void) {

	BC_SIG_LOCK;

#ifndef NDEBUG
	bc_rand_free(&vm.rng);
	bc_vec_free(&vm.out);
	bc_vec_free(&vm.jmp_bufs);

	bc_vec_free(&vm.free_nums);
	bc_vec_free(&vm.nums);
#endif // NDEBUG

	bc_vm_shutdown();

#ifndef NDEBUG
	memset(&vm, 0, sizeof(BcVm));
#endif // NDEBUG

	BC_SIG_UNLOCK;
}

void libbc_gc(void) {
	bc_vec_npop(&vm.nums, vm.nums.len);
	bc_vec_npop(&vm.free_nums, vm.free_nums.len);
	bc_vm_freeTemps();
}

size_t libbc_scale(void) {
	return vm.scale;
}

void libbc_setScale(size_t scale) {
	vm.scale = scale;
}

size_t libbc_ibase(void) {
	return vm.ibase;
}

void libbc_setIbase(size_t ibase) {
	vm.ibase = ibase;
}

size_t libbc_obase(void) {
	return vm.obase;
}

void libbc_setObase(size_t obase) {
	vm.obase = obase;
}

bool libbc_abortOnFatalError(void) {
	return vm.abrt;
}

void libbc_setAbortOnFatalError(bool abrt) {
	vm.abrt = abrt;
}

BcError libbc_num_error(const BcNumber n) {
	if (n >= vm.nums.len) {
		if (n > 0 - (BcNumber) BC_ERROR_NELEMS) return (BcError) (0 - n);
		else return BC_ERROR_INVALID_NUM;
	}
	else return BC_ERROR_SUCCESS;
}

static BcNumber libbc_num_insert(BcNum *restrict n) {

	BcNumber idx;

	if (vm.free_nums.len) {

		BcNum *ptr;

		idx = *((BcNumber*) bc_vec_top(&vm.free_nums));

		bc_vec_pop(&vm.free_nums);

		ptr = bc_vec_item(&vm.nums, idx);
		memcpy(ptr, n, sizeof(BcNum));
	}
	else {
		idx = vm.nums.len;
		bc_vec_push(&vm.nums, n);
	}

	return idx;
}

BcNumber libbc_num_init(void) {
	return libbc_num_initReq(BC_NUM_DEF_SIZE);
}

BcNumber libbc_num_initReq(size_t req) {

	BcError e = BC_ERROR_SUCCESS;
	BcNum n;
	BcNumber idx;

	BC_FUNC_HEADER_LOCK(err);

	bc_vec_grow(&vm.nums, 1);

	bc_num_init(&n, req);

err:
	BC_FUNC_FOOTER_UNLOCK(e);
	BC_MAYBE_SETUP(e, n, idx);
	return idx;
}

BcError libbc_num_copy(const BcNumber d, const BcNumber s) {

	BcError e = BC_ERROR_SUCCESS;
	BcNum *dest, *src;

	BC_FUNC_HEADER_LOCK(err);

	assert(d < vm.nums.len && s < vm.nums.len);

	dest = BC_NUM(d);
	src = BC_NUM(s);

	assert(dest != NULL && src != NULL);
	assert(dest->num != NULL && src->num != NULL);

	bc_num_copy(dest, src);

err:
	BC_FUNC_FOOTER_UNLOCK(e);
	return e;
}

BcNumber libbc_num_dup(const BcNumber s) {

	BcError e = BC_ERROR_SUCCESS;
	BcNum *src, dest;
	BcNumber idx;

	BC_FUNC_HEADER_LOCK(err);

	bc_vec_grow(&vm.nums, 1);

	assert(s < vm.nums.len);

	src = BC_NUM(s);

	assert(src != NULL && src->num != NULL);

	bc_num_clear(&dest);

	bc_num_createCopy(&dest, src);

err:
	BC_FUNC_FOOTER_UNLOCK(e);
	BC_MAYBE_SETUP(e, dest, idx);
	return idx;
}

static void libbc_num_destruct(void *num) {

	BcNum *n = (BcNum*) num;

	assert(n != NULL);

	if (n->num == NULL) return;

	bc_num_free(num);
	bc_num_clear(num);
}

static void libbc_num_dtor(BcNumber n, BcNum *restrict num) {

	BC_SIG_ASSERT_LOCKED;

	assert(num != NULL && num->num != NULL);

	libbc_num_destruct(num);
	bc_vec_push(&vm.free_nums, &n);
}

void libbc_num_free(BcNumber n) {

	BcNum *num;

	BC_SIG_LOCK;

	assert(n < vm.nums.len);

	num = BC_NUM(n);

	libbc_num_dtor(n, num);

	BC_SIG_UNLOCK;
}

bool libbc_num_neg(const BcNumber n) {

	BcNum *num;

	assert(n < vm.nums.len);

	num = BC_NUM(n);

	assert(num != NULL && num->num != NULL);

	return num->neg;
}

size_t libbc_num_scale(const BcNumber n) {

	BcNum *num;

	assert(n < vm.nums.len);

	num = BC_NUM(n);

	assert(num != NULL && num->num != NULL);

	return bc_num_scale(num);
}

size_t libbc_num_len(const BcNumber n) {

	BcNum *num;

	assert(n < vm.nums.len);

	num = BC_NUM(n);

	assert(num != NULL && num->num != NULL);

	return bc_num_len(num);
}

BcError libbc_num_bigdig(const BcNumber n, BcBigDig *result) {

	BcError e = BC_ERROR_SUCCESS;
	BcNum *num;

	BC_FUNC_HEADER_LOCK(err);

	assert(n < vm.nums.len);
	assert(result != NULL);

	num = BC_NUM(n);

	assert(num != NULL && num->num != NULL);

	bc_num_bigdig(num, result);

err:
	BC_FUNC_FOOTER_UNLOCK(e);
	return e;
}

BcNumber libbc_num_bigdig2num(const BcBigDig val) {

	BcError e = BC_ERROR_SUCCESS;
	BcNum n;
	BcNumber idx;

	BC_FUNC_HEADER_LOCK(err);

	bc_vec_grow(&vm.nums, 1);

	bc_num_createFromBigdig(&n, val);

err:
	BC_FUNC_FOOTER_UNLOCK(e);
	BC_MAYBE_SETUP(e, n, idx);
	return idx;
}

BcError libbc_num_bigdig2num_err(const BcNumber n, const BcBigDig val) {

	BcError e = BC_ERROR_SUCCESS;
	BcNum *num;

	BC_CHECK_NUM_ERR(n);

	BC_FUNC_HEADER_LOCK(err);

	assert(n < vm.nums.len);

	num = BC_NUM(n);

	assert(num != NULL && num->num != NULL);

	bc_num_bigdig2num(num, val);

err:
	BC_FUNC_FOOTER_UNLOCK(e);
	return e;
}

static BcNumber libbc_num_binary(const BcNumber a, const BcNumber b,
                                 const BcNumBinaryOp op,
                                 const BcNumBinaryOpReq req)
{
	BcError e = BC_ERROR_SUCCESS;
	BcNum *aptr, *bptr;
	BcNum c;
	BcNumber idx;

	BC_CHECK_NUM(a);
	BC_CHECK_NUM(b);

	BC_FUNC_HEADER_LOCK(err);

	bc_vec_grow(&vm.nums, 1);

	assert(a < vm.nums.len && b < vm.nums.len);

	aptr = BC_NUM(a);
	bptr = BC_NUM(b);

	assert(aptr != NULL && bptr != NULL);
	assert(aptr->num != NULL && bptr->num != NULL);

	bc_num_clear(&c);

	bc_num_init(&c, req(aptr, bptr, vm.scale));

	BC_SIG_UNLOCK;

	op(aptr, bptr, &c, vm.scale);

err:
	BC_SIG_MAYLOCK;
	libbc_num_dtor(a, aptr);
	if (b != a) libbc_num_dtor(b, bptr);
	BC_MAYBE_SETUP(e, c, idx);

	return idx;
}

static BcError libbc_num_binary_err(const BcNumber a, const BcNumber b,
                                    const BcNumber c, const BcNumBinaryOp op)
{
	BcError e = BC_ERROR_SUCCESS;
	BcNum *aptr, *bptr, *cptr;

	BC_CHECK_NUM_ERR(a);
	BC_CHECK_NUM_ERR(b);
	BC_CHECK_NUM_ERR(c);

	BC_FUNC_HEADER(err);

	assert(a < vm.nums.len && b < vm.nums.len && c < vm.nums.len);

	aptr = BC_NUM(a);
	bptr = BC_NUM(b);
	cptr = BC_NUM(c);

	assert(aptr->num != NULL && bptr->num != NULL && cptr->num != NULL);

	op(aptr, bptr, cptr, vm.scale);

err:
	BC_SIG_MAYLOCK;
	BC_FUNC_FOOTER(e);
	return e;
}

BcNumber libbc_num_add(const BcNumber a, const BcNumber b) {
	return libbc_num_binary(a, b, bc_num_add, bc_num_addReq);
}

BcError libbc_num_add_err(const BcNumber a, const BcNumber b, const BcNumber c)
{
	return libbc_num_binary_err(a, b, c, bc_num_add);
}

BcNumber libbc_num_sub(const BcNumber a, const BcNumber b) {
	return libbc_num_binary(a, b, bc_num_sub, bc_num_addReq);
}

BcError libbc_num_sub_err(const BcNumber a, const BcNumber b, const BcNumber c)
{
	return libbc_num_binary_err(a, b, c, bc_num_sub);
}

BcNumber libbc_num_mul(const BcNumber a, const BcNumber b) {
	return libbc_num_binary(a, b, bc_num_mul, bc_num_mulReq);
}

BcError libbc_num_mul_err(const BcNumber a, const BcNumber b, const BcNumber c)
{
	return libbc_num_binary_err(a, b, c, bc_num_mul);
}

BcNumber libbc_num_div(const BcNumber a, const BcNumber b) {
	return libbc_num_binary(a, b, bc_num_div, bc_num_divReq);
}

BcError libbc_num_div_err(const BcNumber a, const BcNumber b, const BcNumber c)
{
	return libbc_num_binary_err(a, b, c, bc_num_div);
}

BcNumber libbc_num_mod(const BcNumber a, const BcNumber b) {
	return libbc_num_binary(a, b, bc_num_mod, bc_num_divReq);
}

BcError libbc_num_mod_err(const BcNumber a, const BcNumber b, const BcNumber c)
{
	return libbc_num_binary_err(a, b, c, bc_num_mod);
}

BcNumber libbc_num_pow(const BcNumber a, const BcNumber b) {
	return libbc_num_binary(a, b, bc_num_pow, bc_num_powReq);
}

BcError libbc_num_pow_err(const BcNumber a, const BcNumber b, const BcNumber c)
{
	return libbc_num_binary_err(a, b, c, bc_num_pow);
}

#if BC_ENABLE_EXTRA_MATH
BcNumber libbc_num_places(const BcNumber a, const BcNumber b) {
	return libbc_num_binary(a, b, bc_num_places, bc_num_placesReq);
}

BcError libbc_num_places_err(const BcNumber a, const BcNumber b, const BcNumber c)
{
	return libbc_num_binary_err(a, b, c, bc_num_places);
}

BcNumber libbc_num_lshift(const BcNumber a, const BcNumber b) {
	return libbc_num_binary(a, b, bc_num_lshift, bc_num_placesReq);
}

BcError libbc_num_lshift_err(const BcNumber a, const BcNumber b, const BcNumber c)
{
	return libbc_num_binary_err(a, b, c, bc_num_lshift);
}

BcNumber libbc_num_rshift(const BcNumber a, const BcNumber b)
{
	return libbc_num_binary(a, b, bc_num_rshift, bc_num_placesReq);
}

BcError libbc_num_rshift_err(const BcNumber a, const BcNumber b, const BcNumber c)
{
	return libbc_num_binary_err(a, b, c, bc_num_lshift);
}
#endif // BC_ENABLE_EXTRA_MATH

BcNumber libbc_num_sqrt(const BcNumber a) {

	BcError e = BC_ERROR_SUCCESS;
	BcNum *aptr;
	BcNum b;
	BcNumber idx;

	BC_CHECK_NUM(a);

	BC_FUNC_HEADER(err);

	bc_vec_grow(&vm.nums, 1);

	assert(a < vm.nums.len);

	aptr = BC_NUM(a);

	bc_num_sqrt(aptr, &b, vm.scale);

err:
	BC_SIG_MAYLOCK;
	BC_FUNC_FOOTER(e);
	libbc_num_dtor(a, aptr);
	BC_MAYBE_SETUP(e, b, idx);

	return idx;
}

BcError libbc_num_sqrt_err(const BcNumber a, const BcNumber b) {

	BcError e = BC_ERROR_SUCCESS;
	BcNum *aptr, *bptr;

	BC_CHECK_NUM_ERR(a);
	BC_CHECK_NUM_ERR(b);

	BC_FUNC_HEADER(err);

	assert(a < vm.nums.len && b < vm.nums.len);

	aptr = BC_NUM(a);
	bptr = BC_NUM(b);

	assert(aptr != NULL && bptr != NULL);
	assert(aptr->num != NULL && bptr->num != NULL);

	bc_num_sr(aptr, bptr, vm.scale);

err:
	BC_SIG_MAYLOCK;
	BC_FUNC_FOOTER(e);
	return e;
}

BcError libbc_num_divmod(const BcNumber a, const BcNumber b,
                         BcNumber *c, BcNumber *d)
{
	BcError e = BC_ERROR_SUCCESS;
	size_t req;
	BcNum *aptr, *bptr;
	BcNum cnum, dnum;

	BC_CHECK_NUM_ERR(a);
	BC_CHECK_NUM_ERR(b);

	BC_FUNC_HEADER_LOCK(err);

	bc_vec_grow(&vm.nums, 2);

	assert(c != NULL && d != NULL);

	aptr = BC_NUM(a);
	bptr = BC_NUM(b);

	assert(aptr != NULL && bptr != NULL);
	assert(aptr->num != NULL && bptr->num != NULL);

	bc_num_clear(&cnum);
	bc_num_clear(&dnum);

	req = bc_num_divReq(aptr, bptr, vm.scale);

	bc_num_init(&cnum, req);
	bc_num_init(&dnum, req);

	BC_SIG_UNLOCK;

	bc_num_divmod(aptr, bptr, &cnum, &dnum, vm.scale);

err:
	BC_SIG_MAYLOCK;

	libbc_num_dtor(a, aptr);
	if (b != a) libbc_num_dtor(b, bptr);

	if (BC_ERR(vm.err)) {
		if (cnum.num != NULL) bc_num_free(&cnum);
		if (dnum.num != NULL) bc_num_free(&dnum);
	}
	else {
		*c = libbc_num_insert(&cnum);
		*d = libbc_num_insert(&dnum);
	}

	BC_FUNC_FOOTER(e);

	return e;
}

BcError libbc_num_divmod_err(const BcNumber a, const BcNumber b,
                             const BcNumber c, const BcNumber d)
{
	BcError e = BC_ERROR_SUCCESS;
	BcNum *aptr, *bptr, *cptr, *dptr;

	BC_CHECK_NUM_ERR(a);
	BC_CHECK_NUM_ERR(b);
	BC_CHECK_NUM_ERR(c);
	BC_CHECK_NUM_ERR(d);

	BC_FUNC_HEADER(err);

	assert(a < vm.nums.len && b < vm.nums.len);
	assert(c < vm.nums.len && d < vm.nums.len);

	aptr = BC_NUM(a);
	bptr = BC_NUM(b);
	cptr = BC_NUM(c);
	dptr = BC_NUM(d);

	assert(aptr != NULL && bptr != NULL && cptr != NULL && dptr != NULL);
	assert(aptr != cptr && aptr != dptr && bptr != cptr && bptr != dptr);
	assert(cptr != dptr);
	assert(aptr->num != NULL && bptr->num != NULL);
	assert(cptr->num != NULL && dptr->num != NULL);

	bc_num_divmod(aptr, bptr, cptr, dptr, vm.scale);

err:
	BC_SIG_MAYLOCK;
	BC_FUNC_FOOTER(e);
	return e;
}

BcNumber libbc_num_modexp(const BcNumber a, const BcNumber b, const BcNumber c)
{
	BcError e = BC_ERROR_SUCCESS;
	size_t req;
	BcNum *aptr, *bptr, *cptr;
	BcNum d;
	BcNumber idx;

	BC_CHECK_NUM(a);
	BC_CHECK_NUM(b);
	BC_CHECK_NUM(c);

	BC_FUNC_HEADER_LOCK(err);

	bc_vec_grow(&vm.nums, 1);

	assert(a < vm.nums.len && b < vm.nums.len && c < vm.nums.len);

	aptr = BC_NUM(a);
	bptr = BC_NUM(b);
	cptr = BC_NUM(c);

	assert(aptr != NULL && bptr != NULL && cptr != NULL);
	assert(aptr->num != NULL && bptr->num != NULL && cptr->num != NULL);

	bc_num_clear(&d);

	req = bc_num_divReq(aptr, cptr, 0);

	bc_num_init(&d, req);

	BC_SIG_UNLOCK;

	bc_num_modexp(aptr, bptr, cptr, &d);

err:
	BC_SIG_MAYLOCK;

	libbc_num_dtor(a, aptr);
	if (b != a) libbc_num_dtor(b, bptr);
	if (c != a && c != b) libbc_num_dtor(c, cptr);

	BC_MAYBE_SETUP(e, d, idx);

	return idx;
}

BcError libbc_num_modexp_err(const BcNumber a, const BcNumber b,
                             const BcNumber c, const BcNumber d)
{
	BcError e = BC_ERROR_SUCCESS;
	BcNum *aptr, *bptr, *cptr, *dptr;

	BC_CHECK_NUM_ERR(a);
	BC_CHECK_NUM_ERR(b);
	BC_CHECK_NUM_ERR(c);
	BC_CHECK_NUM_ERR(d);

	BC_FUNC_HEADER(err);

	assert(a < vm.nums.len && b < vm.nums.len && c < vm.nums.len);
	assert(d < vm.nums.len);

	aptr = BC_NUM(a);
	bptr = BC_NUM(b);
	cptr = BC_NUM(c);
	dptr = BC_NUM(d);

	assert(aptr != NULL && bptr != NULL && cptr != NULL && dptr != NULL);
	assert(aptr != dptr && bptr != dptr && cptr != dptr);
	assert(aptr->num != NULL && bptr->num != NULL && cptr->num != NULL);
	assert(dptr->num != NULL);

	bc_num_modexp(aptr, bptr, cptr, dptr);

err:
	BC_SIG_MAYLOCK;
	BC_FUNC_FOOTER(e);
	return e;
}

static size_t libbc_num_req(const BcNumber a, const BcNumber b,
                            const BcReqOp op)
{
	BcNum *aptr, *bptr;

	assert(a < vm.nums.len && b < vm.nums.len);

	aptr = BC_NUM(a);
	bptr = BC_NUM(b);

	assert(aptr != NULL && bptr != NULL);
	assert(aptr->num != NULL && bptr->num != NULL);

	return op(aptr, bptr, vm.scale);
}

size_t libbc_num_addReq(const BcNumber a, const BcNumber b) {
	return libbc_num_req(a, b, bc_num_addReq);
}

size_t libbc_num_mulReq(const BcNumber a, const BcNumber b) {
	return libbc_num_req(a, b, bc_num_mulReq);
}

size_t libbc_num_divReq(const BcNumber a, const BcNumber b) {
	return libbc_num_req(a, b, bc_num_divReq);
}

size_t libbc_num_powReq(const BcNumber a, const BcNumber b) {
	return libbc_num_req(a, b, bc_num_powReq);
}

#if BC_ENABLE_EXTRA_MATH
size_t libbc_num_placesReq(const BcNumber a, const BcNumber b) {
	return libbc_num_req(a, b, bc_num_placesReq);
}
#endif // BC_ENABLE_EXTRA_MATH

BcError libbc_num_setScale(const BcNumber n, size_t scale) {

	BcError e = BC_ERROR_SUCCESS;
	BcNum *nptr;

	BC_CHECK_NUM_ERR(n);

	BC_FUNC_HEADER(err);

	assert(n < vm.nums.len);

	nptr = BC_NUM(n);

	assert(nptr != NULL && nptr->num != NULL);

	if (scale > nptr->scale) bc_num_extend(nptr, scale - nptr->scale);
	else if (scale < nptr->scale) bc_num_truncate(nptr, nptr->scale - scale);

err:
	BC_SIG_MAYLOCK;
	BC_FUNC_FOOTER(e);
	return e;
}

ssize_t libbc_num_cmp(const BcNumber a, const BcNumber b) {

	BcNum *aptr, *bptr;

	assert(a < vm.nums.len && b < vm.nums.len);

	aptr = BC_NUM(a);
	bptr = BC_NUM(b);

	assert(aptr != NULL && bptr != NULL);
	assert(aptr->num != NULL && bptr->num != NULL);

	return bc_num_cmp(aptr, bptr);
}

void libbc_num_zero(const BcNumber n) {

	BcNum *nptr;

	assert(n < vm.nums.len);

	nptr = BC_NUM(n);

	assert(nptr != NULL && nptr->num != NULL);

	bc_num_zero(nptr);
}

void libbc_num_one(const BcNumber n) {

	BcNum *nptr;

	assert(n < vm.nums.len);

	nptr = BC_NUM(n);

	assert(nptr != NULL && nptr->num != NULL);

	bc_num_one(nptr);
}

ssize_t libbc_num_cmpZero(const BcNumber n) {

	BcNum *nptr;

	assert(n < vm.nums.len);

	nptr = BC_NUM(n);

	assert(nptr != NULL && nptr->num != NULL);

	return bc_num_cmpZero(nptr);
}

BcNumber libbc_num_parse(const char *restrict val, const BcBigDig base) {

	BcError e = BC_ERROR_SUCCESS;
	BcNum n;
	BcNumber idx;

	BC_FUNC_HEADER_LOCK(err);

	bc_vec_grow(&vm.nums, 1);

	assert(val != NULL);

	bc_num_clear(&n);

	bc_num_init(&n, BC_NUM_DEF_SIZE);

	BC_SIG_UNLOCK;

	bc_num_parse(&n, val, base);

err:
	BC_SIG_MAYLOCK;
	BC_MAYBE_SETUP(e, n, idx);

	return idx;
}

BcError libbc_num_parse_err(const BcNumber n, const char *restrict val,
                            const BcBigDig base)
{
	BcError e = BC_ERROR_SUCCESS;
	BcNum *nptr;

	BC_CHECK_NUM_ERR(n);

	BC_FUNC_HEADER(err);

	assert(val != NULL);
	assert(n < vm.nums.len);

	if (!bc_num_strValid(val)) {
		vm.err = BC_ERROR_PARSE_INVALID_NUM;
		goto err;
	}

	nptr = BC_NUM(n);

	assert(nptr != NULL && nptr->num != NULL);

	bc_num_parse(nptr, val, base);

err:
	BC_SIG_MAYLOCK;
	BC_FUNC_FOOTER(e);
	return e;
}

char* libbc_num_string(const BcNumber n, const BcBigDig base) {

	BcNum *nptr;
	char *str = NULL;

	if (BC_ERR(n >= vm.nums.len)) return str;

	BC_FUNC_HEADER(err);

	assert(n < vm.nums.len);

	nptr = BC_NUM(n);

	assert(nptr != NULL && nptr->num != NULL);

	bc_num_print(nptr, base, false);

	str = bc_vm_strdup(vm.out.v);

err:
	BC_SIG_MAYLOCK;
	BC_FUNC_FOOTER_NO_ERR;

	return str;
}

#if BC_ENABLE_EXTRA_MATH
BcNumber libbc_num_irand(const BcNumber a) {

	BcError e = BC_ERROR_SUCCESS;
	BcNum *aptr;
	BcNum b;
	BcNumber idx;

	BC_CHECK_NUM(a);

	BC_FUNC_HEADER_LOCK(err);

	bc_vec_grow(&vm.nums, 1);

	assert(a < vm.nums.len);

	aptr = BC_NUM(a);

	assert(aptr != NULL && aptr->num != NULL);

	bc_num_clear(&b);

	bc_num_init(&b, BC_NUM_DEF_SIZE);

	BC_SIG_UNLOCK;

	bc_num_irand(aptr, &b, &vm.rng);

err:
	BC_SIG_MAYLOCK;
	libbc_num_dtor(a, aptr);
	BC_MAYBE_SETUP(e, b, idx);

	return idx;
}

BcError libbc_num_irand_err(const BcNumber a, const BcNumber b) {

	BcError e = BC_ERROR_SUCCESS;
	BcNum *aptr, *bptr;

	BC_CHECK_NUM_ERR(a);
	BC_CHECK_NUM_ERR(b);

	BC_FUNC_HEADER(err);

	assert(a < vm.nums.len && b < vm.nums.len);

	aptr = BC_NUM(a);
	bptr = BC_NUM(b);

	assert(aptr != NULL && aptr->num != NULL);
	assert(bptr != NULL && bptr->num != NULL);

	bc_num_irand(aptr, bptr, &vm.rng);

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

BcNumber libbc_num_frand(size_t places) {

	BcError e = BC_ERROR_SUCCESS;
	BcNum n;
	BcNumber idx;

	BC_FUNC_HEADER_LOCK(err);

	bc_vec_grow(&vm.nums, 1);

	bc_num_clear(&n);

	bc_num_init(&n, BC_NUM_DEF_SIZE);

	BC_SIG_UNLOCK;

	libbc_num_frandHelper(&n, places);

err:
	BC_SIG_MAYLOCK;
	BC_MAYBE_SETUP(e, n, idx);

	return idx;
}

BcError libbc_num_frand_err(const BcNumber n, size_t places) {

	BcError e = BC_ERROR_SUCCESS;
	BcNum *nptr;

	BC_CHECK_NUM_ERR(n);

	BC_FUNC_HEADER(err);

	assert(n < vm.nums.len);

	nptr = BC_NUM(n);

	assert(nptr != NULL && nptr->num != NULL);

	libbc_num_frandHelper(nptr, places);

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
	libbc_num_frandHelper(&fr, places);

	bc_num_add(&ir, &fr, b, 0);

err:
	BC_SIG_MAYLOCK;
	bc_num_free(&fr);
	bc_num_free(&ir);
	BC_LONGJMP_CONT;
}

BcNumber libbc_num_ifrand(const BcNumber a, size_t places) {

	BcError e = BC_ERROR_SUCCESS;
	BcNum *aptr;
	BcNum b;
	BcNumber idx;

	BC_CHECK_NUM(a);

	BC_FUNC_HEADER_LOCK(err);

	bc_vec_grow(&vm.nums, 1);

	assert(a < vm.nums.len);

	aptr = BC_NUM(a);

	assert(aptr != NULL && aptr->num != NULL);

	bc_num_clear(&b);

	bc_num_init(&b, BC_NUM_DEF_SIZE);

	BC_SIG_UNLOCK;

	libbc_num_ifrandHelper(aptr, &b, places);

err:
	BC_SIG_MAYLOCK;
	libbc_num_dtor(a, aptr);
	BC_MAYBE_SETUP(e, b, idx);

	return idx;
}

BcError libbc_num_ifrand_err(const BcNumber a, size_t places, const BcNumber b)
{
	BcError e = BC_ERROR_SUCCESS;
	BcNum *aptr, *bptr;

	BC_CHECK_NUM_ERR(a);
	BC_CHECK_NUM_ERR(b);

	BC_FUNC_HEADER(err);

	assert(a < vm.nums.len && b < vm.nums.len);

	aptr = BC_NUM(a);
	bptr = BC_NUM(b);

	assert(aptr != NULL && aptr->num != NULL);
	assert(bptr != NULL && bptr->num != NULL);

	libbc_num_ifrandHelper(aptr, bptr, places);

err:
	BC_SIG_MAYLOCK;
	BC_FUNC_FOOTER(e);
	return e;
}

BcError libbc_num_seedWithNum(const BcNumber n) {

	BcError e = BC_ERROR_SUCCESS;
	BcNum *nptr;

	BC_CHECK_NUM_ERR(n);

	BC_FUNC_HEADER(err);

	assert(n < vm.nums.len);

	nptr = BC_NUM(n);

	assert(nptr != NULL && nptr->num != NULL);

	bc_num_rng(nptr, &vm.rng);

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

BcNumber libbc_num_seed2num(void) {

	BcError e = BC_ERROR_SUCCESS;
	BcNum n;
	BcNumber idx;

	BC_FUNC_HEADER_LOCK(err);

	bc_num_clear(&n);

	bc_num_init(&n, BC_NUM_DEF_SIZE);

	BC_SIG_UNLOCK;

	bc_num_createFromRNG(&n, bc_vec_top(&vm.rng.v));

err:
	BC_SIG_MAYLOCK;
	BC_MAYBE_SETUP(e, n, idx);

	return idx;
}

BcError libbc_num_seed2num_err(const BcNumber n) {

	BcError e = BC_ERROR_SUCCESS;
	BcNum *nptr;

	BC_CHECK_NUM_ERR(n);

	BC_FUNC_HEADER(err);

	assert(n < vm.nums.len);

	nptr = BC_NUM(n);

	assert(nptr != NULL && nptr->num != NULL);

	bc_num_createFromRNG(nptr, bc_vec_top(&vm.rng.v));

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
