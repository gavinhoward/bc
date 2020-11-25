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
 * Tests for bcl(3).
 *
 */

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <bcl.h>

static void err(BclError e) {
	if (e != BCL_ERROR_NONE) abort();
}

int main(void) {

	BclError e;
	BclContext ctxt;
	size_t scale;
	BclNumber n, n2, n3, n4, n5, n6;
	char* res;

	e = bcl_init();
	err(e);

	e = bcl_init();
	err(e);

	if (bcl_abortOnFatalError()) err(BCL_ERROR_FATAL_UNKNOWN_ERR);

	bcl_setAbortOnFatalError(true);

	if (!bcl_abortOnFatalError()) err(BCL_ERROR_FATAL_UNKNOWN_ERR);

	ctxt = bcl_ctxt_create();

	bcl_pushContext(ctxt);

	ctxt = bcl_ctxt_create();

	bcl_pushContext(ctxt);

	scale = 10;

	bcl_ctxt_setScale(ctxt, scale);

	scale = bcl_ctxt_scale(ctxt);
	if (scale != 10) err(BCL_ERROR_FATAL_UNKNOWN_ERR);

	scale = 16;

	bcl_ctxt_setIbase(ctxt, scale);

	scale = bcl_ctxt_ibase(ctxt);
	if (scale != 16) err(BCL_ERROR_FATAL_UNKNOWN_ERR);

	bcl_ctxt_setObase(ctxt, scale);

	scale = bcl_ctxt_obase(ctxt);
	if (scale != 16) err(BCL_ERROR_FATAL_UNKNOWN_ERR);

	bcl_ctxt_setIbase(ctxt, 10);
	bcl_ctxt_setObase(ctxt, 10);

	n = bcl_num_create();

	n2 = bcl_dup(n);
	bcl_copy(n, n2);

	n3 = bcl_parse("2938");
	err(bcl_err(n3));

	n4 = bcl_parse("-28390.9108273");
	err(bcl_err(n4));

	if (!bcl_num_neg(n4)) err(BCL_ERROR_FATAL_UNKNOWN_ERR);

	n3 = bcl_add(n3, n4);
	err(bcl_err(n3));

	res = bcl_string(bcl_dup(n3));
	if (strcmp(res, "-25452.9108273")) err(BCL_ERROR_FATAL_UNKNOWN_ERR);

	free(res);

	n4 = bcl_parse("8937458902.2890347");
	err(bcl_err(n4));

	e = bcl_divmod(bcl_dup(n4), n3, &n5, &n6);
	err(e);

	res = bcl_string(n5);

	if (strcmp(res, "-351137.0060159482"))
		err(BCL_ERROR_FATAL_UNKNOWN_ERR);

	free(res);

	res = bcl_string(n6);

	if (strcmp(res, ".00000152374405414"))
		err(BCL_ERROR_FATAL_UNKNOWN_ERR);

	free(res);

	n4 = bcl_sqrt(n4);
	err(bcl_err(n4));

	res = bcl_string(n4);

	if (strcmp(res, "94538.1346457028"))
		err(BCL_ERROR_FATAL_UNKNOWN_ERR);

	free(res);

	bcl_num_free(n);

	bcl_ctxt_freeNums(ctxt);

	bcl_gc();

	bcl_popContext();

	bcl_ctxt_free(ctxt);

	ctxt = bcl_context();

	bcl_popContext();

	bcl_ctxt_free(ctxt);

	bcl_free();

	bcl_free();

	return 0;
}
