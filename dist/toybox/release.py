#! /usr/bin/python3 -B

# Copyright 2018 Gavin D. Howard
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
# REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
# AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
# INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
# LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
# OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
# PERFORMANCE OF THIS SOFTWARE.
#
# Release script for bc (to put into toybox).

# ***WARNING***: This script is only intended to be used by the project
# maintainer to cut releases.

import sys
import os
import subprocess
import regex as re

cwd = os.path.realpath(os.getcwd())

script = os.path.realpath(sys.argv[0])

testdir = os.path.dirname(script)

if len(sys.argv) < 2:
	print("usage: {} toybox_dir".format(script))
	sys.exit(1)

toybox = sys.argv[1].rstrip(os.sep)

toybox_bc = toybox + "/toys/pending/bc.c"

os.chdir(testdir)
os.chdir("../..")

res = subprocess.run(["make", "version"], stdout=subprocess.PIPE)

if res.returncode != 0:
	sys.exit(res.returncode)

version = res.stdout.decode("utf-8")

res = subprocess.run(["make", "libcname"], stdout=subprocess.PIPE)

if res.returncode != 0:
	sys.exit(res.returncode)

libcname = res.stdout.decode("utf-8")

res = subprocess.run(["make", libcname])

if res.returncode != 0:
	sys.exit(res.returncode)

content = ""

with open(testdir + "/files.txt") as f:
	files = f.read().splitlines()

for name in files:

	if name == "gen/lib.c":
		header_end = 2
	else:
		header_end = 22

	with open(name) as f:
		lines = f.readlines()

	# Skip the header lines.
	for i in range(header_end, len(lines)):
		content += lines[i]

vm_c = "src/vm.c"
vm_c_stuff = ""

with open(vm_c) as f:
	vm_c_lines = f.readlines()

for i in range(22, len(vm_c_lines)):
	vm_c_stuff += vm_c_lines[i]

vm_c_replacements = [
	[ '\t', '  ' ],
	[ '\n  [ ]*assert\(.*?\);$', '' ],
	[ '^BcStatus bc_vm_exec\(unsigned int flags, BcVec \*exprs, BcVec \*files,' +
	  '\n[ ]*BcParseInit parse_init, BcParseRead parse_read\)\n\{',
	  'void bc_main(void) {\n' ],
	[ '^  bcg.posix = flags & BC_FLAG_S;$', '' ],
	[ '^  bcg.warn = flags & BC_FLAG_W;$', '' ],
	[ '([^_])flags ', r'\1toys.optflags ' ],
	[ 'files->len', 'toys.optc' ],
	[ '\*\(\(char\*\*\) bc_vec_item\(files, i\)\)', 'toys.optargs[i]' ],
	[ '^  bc_parse_free\(&vm\.parse\);', '  if (CFG_TOYBOX_FREE) bc_parse_free(&vm.parse);' ],
	[ '^  bc_program_free\(&vm\.prog\);', '  if (CFG_TOYBOX_FREE) bc_program_free(&vm.prog);' ],
	[ 'if \(\(s = bc_program_init\(&vm\.prog, len, parse_init, parse_read\)\)\) return s;',
	  'if ((toys.exitval = bc_program_init(&vm.prog, len))) return;' ],
	[ 'return s == BC_STATUS_QUIT \? BC_STATUS_SUCCESS : s;',
	  'toys.exitval = s == BC_STATUS_QUIT ? BC_STATUS_SUCCESS : s;' ],
	[ 'vm->parse\.parse\(&vm', 'bc_parse_parse(&vm' ],
	[ 'vm\.parse\.parse\(&vm', 'bc_parse_parse(&vm' ],
	[ '^    return BC_STATUS_EXEC_SIGACTION_FAIL;$',
	  '    toys.exitval = BC_STATUS_EXEC_SIGACTION_FAIL;\n    return;' ],
	[ '\(\(s = parse_init\(&vm\.parse, &vm\.prog\)\)\)', '((s = bc_parse_init(&vm.parse, &vm.prog)))' ],
	[ '^[ ]*if \(exprs->len > 1 && \(s = bc_vm_process\(&vm, exprs->v\)\)\) goto err;$', '' ],
	[ 'if \(exprs->len <= 1\) s = bc_vm_stdin\(&vm\);$', 's = bc_vm_stdin(&vm);' ],
	[ 'size_t len = strlen\(bcg.sig_msg\);', 'size_t len = strlen(bc_sig_msg);' ],
	[ 'write\(2, bcg.sig_msg, len\) == \(ssize_t\) len', 'write(2, bc_sig_msg, len) == (ssize_t) len' ],
	[ '\(!bcg\.bc && \(lenv = getenv\(\"DC_LINE_LENGTH\"\)\)\) \|\|', '' ],
	[ '\n[ ]*\(bcg\.bc && \(lenv = getenv\(\"BC_LINE_LENGTH\"\)\)\)\)\n[ ]*\{',
	  '(lenv = getenv("BC_LINE_LENGTH"))) {' ],
	[ '\n[ ]*if \(help && printf\(help, bcg\.name\) < 0\) return BC_STATUS_IO_ERR;$', '' ],
	[ 's = bc_vm_header\(NULL\)', 's = bc_vm_header()' ],
]

for rep in vm_c_replacements:
	r = re.compile(rep[0], re.M | re.DOTALL)
	vm_c_stuff = r.sub(rep[1], vm_c_stuff)

content += vm_c_stuff

regexes = [
	'^#include .*$',
	'^#ifndef BC.*_H$',
	'^#define BC.*_H$',
	'^#endif \/\/ BC.*_H$',
	'^extern.*$',
	'^static ',
	'^#define BC_FLAG_X \(1<<0\)$',
	'^#define BC_FLAG_W \(1<<1\)$',
	'^#define BC_FLAG_S \(1<<2\)$',
	'^#define BC_FLAG_Q \(1<<3\)$',
	'^#define BC_FLAG_L \(1<<4\)$',
	'^#define BC_FLAG_I \(1<<5\)$',
	'^#define BC_FLAG_C \(1<<6\)$',
	'^#define BC_INVALID_IDX \(\(size_t\) -1\)$',
	'^#define BC_MAX\(a, b\) \(\(a\) > \(b\) \? \(a\) : \(b\)\)$',
	'^#define BC_MIN\(a, b\) \(\(a\) < \(b\) \? \(a\) : \(b\)\)$',
]

regexes_all = [
	'\n#ifdef DC_ENABLED // Exclude.*?#endif // DC_ENABLED Exclude\n',
	'\n#ifdef DC_ENABLED.*?#else // DC_ENABLED\n',
	'\n#ifdef DC_ENABLED.*?#endif // DC_ENABLED\n',
	'\n#endif // DC_ENABLED\n',
	'\n#ifdef BC_ENABLED\n',
	'\n#else // BC_ENABLED.*?#endif // BC_ENABLED\n',
	'\n#endif // BC_ENABLED\n',
	'^#ifndef NDEBUG.*?^#endif \/\/ NDEBUG\n',
	'\n\t[\t]*assert\(.*?\);$',
	'\#if !defined\(BC_ENABLED\).*?\#endif\n',
	'\n[\t]*// \*\* Exclude start\. \*\*.*?^[\t]*// \*\* Exclude end\. \*\*$',
	'^\tBC_STATUS_INVALID_OPTION,$',
	'^[\t]*p->parse = parse;$',
	'^[\t]*p->parse_init = init;$',
	'^[\t]*p->parse_read = read;$',
	'^\tl->next = next;$',
	'^BcStatus bc_parse_init\(BcParse \*p, BcProgram \*prog\) \{.*?\}',
	'^BcStatus bc_parse_read\(BcParse \*p, BcVec \*code, uint8_t flags\) \{.*?\}'
	'!strcmp\(bcg\.name, bc_name\) && '
]

replacements = [
	[ '\t', '  ' ],
	[ 'bcg.posix', '(toys.optflags & FLAG_s)' ],
	[ 'bcg.warn', '(toys.optflags & FLAG_w)' ],
	[ 'bcg\.', 'TT.' ],
	[ 'TT\.name', 'toys.which->name' ],
	[ 'BC_FLAG_Q', 'FLAG_q' ],
	[ 'BC_FLAG_L', 'FLAG_l' ],
	[ 'BC_FLAG_I', 'FLAG_i' ],
	[ 'BC_FLAG_C', 'FLAG_c' ],
	[ 'BC_MAX\(', 'maxof(' ],
	[ 'BC_MIN\(', 'minof(' ],
	[ 'BC_INVALID_IDX', '((size_t) -1)' ],
	[ ' bool', ' int' ],
	[ '^bool ', 'int ' ],
	[ ' true', ' 1' ],
	[ ' false', ' 0' ],
	[ 'true', '1' ],
	[ 'BcStatus bc_lex_init\(BcLex \*l, BcLexNext next\)', 'BcStatus bc_lex_init(BcLex *l)' ],
	[ 'l->next\(l\)', 'bc_lex_token(l)' ],
	[ '^BcStatus bc_parse_create\(BcParse \*p, BcProgram \*prog,\n[ ]*BcParseParse parse, BcLexNext next\)\n\{',
	  'BcStatus bc_parse_init(BcParse *p, BcProgram *prog) {\n' ],
	[ 'BcStatus bc_program_init\(BcProgram \*p, size_t line_len,\n' +
	  '[ ]*BcParseInit init, BcParseRead read\)\n\{',
	  'BcStatus bc_program_init(BcProgram *p, size_t line_len) {\n' ],
	[ 'if \(\(s = bc_lex_init\(&p->l, next\)\)\) return s;', 'if ((s = bc_lex_init(&p->l))) return s;' ],
	[ 'p->parse_init\(', 'bc_parse_init(' ],
	[ 'p->parse_read\(&parse, &f->code\)', 'bc_parse_expr(&parse, &f->code, BC_PARSE_NOREAD, bc_parse_next_read)' ],
	[ '^BcStatus bc_program_pushVar\(BcProgram \*p, char \*code, size_t \*bgn, bool pop\)',
	  'BcStatus bc_program_pushVar(BcProgram *p, char *code, size_t *bgn)' ],
	[ 'BC_VERSION', '"' + version + '"' ],
	[ 'BcStatus bc_vm_header\(const char\* const help\)', 'BcStatus bc_vm_header()' ],
]

for reg in regexes:
	r = re.compile(reg, re.M)
	content = r.sub('', content)

for reg in regexes_all:
	r = re.compile(reg, re.M | re.DOTALL)
	content = r.sub('', content)

for rep in replacements:
	r = re.compile(rep[0], re.M)
	content = r.sub(rep[1], content)

with open(testdir + "/header.c") as f:
	content = f.read() + content

content = re.sub('\n\n\n+', '\n\n', content)

os.chdir(cwd)

with open(toybox_bc, 'w') as f:
	f.write(content)

