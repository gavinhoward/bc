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
 * Definitions for bc.
 *
 */

#ifndef BC_DC_H
#define BC_DC_H

#if DC_ENABLED

#include <status.h>
#include <lex.h>
#include <parse.h>

// I use 32 as the length because I don't want
// to have to calculate log base 2 of n.
#define DC_PARSE_BUF_LEN ((int) (sizeof(uint32_t) * CHAR_BIT))

// ** Exclude start. **
// ** Busybox exclude start. **
int dc_main(int argc, char **argv);

extern const char dc_help[];
// ** Busybox exclude end. **
// ** Exclude end. **

BcStatus dc_lex_token(BcLex *l);
bool dc_lex_negCommand(BcLex *l);

#if BC_ENABLE_SIGNALS
extern const char dc_sig_msg[];
#endif // BC_ENABLE_SIGNALS

extern const uint8_t dc_lex_regs[];
extern const size_t dc_lex_regs_len;

extern const uint8_t dc_lex_tokens[];
extern const uint8_t dc_parse_insts[];

BcStatus dc_parse_parse(BcParse *p);
BcStatus dc_parse_expr(BcParse *p, uint8_t flags);

#endif // DC_ENABLED

#endif // BC_DC_H
