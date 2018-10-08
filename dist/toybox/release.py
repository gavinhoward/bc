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
	[ '^BcStatus bc_vm_run\(int argc, char \*argv\[\], BcVmExe exe, const char \*env_len\)',
	  'void bc_main(void)' ],
	[ '^  return st;$', 'exit:\n  toys.exitval = (int) st;' ],
	[ '^[ ]*if \(\(s = bc_vec_init\(&vm->files, sizeof\(char\*\), NULL\)\)\) return s;$', '' ],
	[ '^[ ]*if \(\(s = bc_vec_init\(&vm->exprs, sizeof\(char\), NULL\)\)\) goto exprs_err;$', '' ],
	[ 'if \(\(s = bc_program_init\(&vm->prog, len, exe\.init, exe\.exp\)\)\) goto prog_err;$',
	  'if ((s = bc_program_init(&vm->prog, len))) return s;' ],
	[ 'if \(\(s = exe\.init\(&vm->prs, &vm->prog, BC_PROG_MAIN\)\)\) goto parse_err;$',
	  'if ((s = bc_parse_init(&vm->prs, &vm->prog, BC_PROG_MAIN))) goto parse_err;' ],
	[ '^[ ]*if \(\(st = bc_args\(argc, argv, &vm\.flags, &vm\.exprs, &vm\.files\)\)\) goto err;$', '' ],
	[ '^prog_err:\n[ ]*bc_vec_free\(&vm->exprs\);$', '' ],
	[ '^exprs_err:\n[ ]*bc_vec_free\(&vm->files\);$', '' ],
	[ '^err:\n[ ]*bc_parse_free\(&vm->prs\);$', '' ],
	[ '\n[ ]*bc_vec_free\(&vm->exprs\);$', '' ],
	[ '\n[ ]*bc_vec_free\(&vm->files\);$', '' ],
	[ 'if \(\(st = bc_vm_init\(&vm, exe, env_len\)\)\) return st;$',
	  'if ((st = bc_vm_init(&vm))) goto exit;' ],
	[ '^[ ]*if \(\(st = bc_args\(argc, argv, &vm\)\)\) goto err;$', '' ],
	[ '^[ ]*bcg\.posix = vm\.flags & BC_FLAG_S;$', '' ],
	[ '^[ ]*bcg\.warn = vm\.flags & BC_FLAG_W;$', '' ],
	[ '([^_])vm\.flags ', r'\1toys.optflags ' ],
	[ '([^_])vm->flags ', r'\1toys.optflags ' ],
	[ 'vm->files\.len', 'toys.optc' ],
	[ '\*\(\(char\*\*\) bc_vec_item\(&vm->files, i\)\)', 'toys.optargs[i]' ],
	[ '^  bc_vm_free\(&vm\);', '  if (CFG_TOYBOX_FREE) bc_vm_free(&vm);' ],
	[ 'vm->prs\.parse\(&vm', 'bc_parse_parse(&vm' ],
	[ 'vm\.prs\.parse\(&vm', 'bc_parse_parse(&vm' ],
	[ '\(\(s = parse_init\(&vm\.prs, &vm\.prog\)\)\)', '((s = bc_parse_init(&vm.prs, &vm.prog)))' ],
	[ '^[ ]*if \(vm->exprs\.len > 1 && \(s = bc_vm_process\(vm, vm->exprs\.v\)\)\) return s;$', '' ],
	[ 'if \(vm->exprs.\len <= 1\) s = bc_vm_stdin\(vm\);$', 's = bc_vm_stdin(vm);' ],
	[ 'size_t len = strlen\(bcg.sig_msg\);', 'size_t len = strlen(bc_sig_msg);' ],
	[ 'write\(2, bcg.sig_msg, len\) == \(ssize_t\) len', 'write(2, bc_sig_msg, len) == (ssize_t) len' ],
	[ '\n[ ]*if \(help && printf\(help, bcg\.name\) < 0\) return BC_STATUS_IO_ERR;$', '' ],
	[ 'st = bc_vm_info\(NULL\)', 'st = bc_vm_info()' ],
	[ '^[ ]*vm->exe = exe;$', '' ],
	[ 'vm->exe\.strbgn', '\'"\'' ],
	[ 'vm->exe\.strend', '\'"\'' ],
	[ 'BcStatus bc_vm_init\(BcVm \*vm, BcVmExe exe, const char \*env_len\)',
	  'BcStatus bc_vm_init(BcVm *vm)' ],
	[ '\(env_len\)', '("BC_LINE_LENGTH")' ],
	[ '^[ ]*if \(bcg.bc && \(s = bc_args_env\(&vm->flags, &vm->exprs, &vm->files\)\)\)\n[ ]*goto err;$', '' ],
	[ 'bcg.bc \* ', '' ],
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
	'\n#ifdef DC_ENABLED // Exclude.*?#endif // DC_ENABLED Exclude',
	'\n#ifdef DC_ENABLED.*?#else // DC_ENABLED',
	'\n#ifdef DC_ENABLED.*?#endif // DC_ENABLED',
	'\n#ifndef DC_ENABLED',
	'\n#endif // DC_ENABLED',
	'\n#ifdef BC_ENABLED',
	'\n#else // BC_ENABLED.*?#endif // BC_ENABLED',
	'\n#endif // BC_ENABLED',
	'^#ifndef NDEBUG.*?^#endif \/\/ NDEBUG\n',
	'\n\t[\t]*assert\(.*?\);$',
	'\#if !defined\(BC_ENABLED\).*?\#endif\n',
	'\n[\t]*// \*\* Exclude start\. \*\*.*?^[\t]*// \*\* Exclude end\. \*\*$',
	'^\tBC_STATUS_INVALID_OPTION,$',
	'^[\t]*p->parse = parse;$',
	'^[\t]*p->parse_init = init;$',
	'^[\t]*p->parse_expr = expr;$',
	'^\tl->next = next;$',
	'^BcStatus bc_parse_init\(BcParse \*p, BcProgram \*prog, size_t func\) \{.*?\}',
	'^BcStatus bc_parse_read\(BcParse \*p, BcVec \*code, uint8_t flags\) \{.*?\}'
	'!strcmp\(bcg\.name, bc_name\) && '
]

replacements = [
	[ '\t', '  ' ],
	[ 'bcg.posix', '(toys.optflags & FLAG_s)' ],
	[ 'bcg.warn', '(toys.optflags & FLAG_w)' ],
	[ 'bcg\.', 'TT.' ],
	[ 'TT\.name', 'toys.which->name' ],
	[ 'BC_FLAG_S', 'FLAG_s' ],
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
	[ '^BcStatus bc_parse_create\(BcParse \*p, BcProgram \*prog, size_t func,\n' +
	  '[ ]*BcParseParse parse, BcLexNext next\)\n\{',
	  'BcStatus bc_parse_init(BcParse *p, BcProgram *prog, size_t func) {\n' ],
	[ 'BcStatus bc_program_init\(BcProgram \*p, size_t line_len,\n' +
	  '[ ]*BcParseInit init, BcParseExpr expr\)\n\{',
	  'BcStatus bc_program_init(BcProgram *p, size_t line_len) {\n' ],
	[ 'if \(\(s = bc_lex_init\(&p->l, next\)\)\) return s;', 'if ((s = bc_lex_init(&p->l))) return s;' ],
	[ 'p->parse_init\(', 'bc_parse_init(' ],
	[ 'p->parse_expr\(&parse, BC_PARSE_NOREAD\)', 'bc_parse_expr(&parse, BC_PARSE_NOREAD, bc_parse_next_read)' ],
	[ '^BcStatus bc_program_pushVar\(BcProgram \*p, char \*code, size_t \*bgn, bool pop\)',
	  'BcStatus bc_program_pushVar(BcProgram *p, char *code, size_t *bgn)' ],
	[ 'BC_VERSION', '"' + version + '"' ],
	[ 'BcStatus bc_vm_info\(const char\* const help\)', 'BcStatus bc_vm_info()' ],
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

