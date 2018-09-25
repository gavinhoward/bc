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
os.chdir("..")

res = subprocess.run(["make", "gen/lib.c"])

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
	[ '^BcStatus bc_vm_exec\(unsigned int flags, BcVec \*files\)',
	  'void bc_main(void)' ],
	[ '^  bcg.posix = flags & BC_FLAG_S;$', '' ],
	[ '^  bcg.warn = flags & BC_FLAG_W;$', '' ],
	[ '([^_])flags ', r'\1toys.optflags ' ],
	[ 'files->len', 'toys.optc' ],
	[ '\*\(\(char\*\*\) bc_vec_item\(files, i\)\)', 'toys.optargs[i]' ],
	[ '^  bc_parse_free\(&bc\.parse\);', '  if (CFG_TOYBOX_FREE) bc_parse_free(&bc.parse);' ],
	[ '^  bc_program_free\(&bc\.prog\);', '  if (CFG_TOYBOX_FREE) bc_program_free(&bc.prog);' ],
	[ 'if \(\(status = bc_vm_set_sig\(\)\)\) return status;',
	  'if ((toys.exitval = bc_vm_set_sig())) return;' ],
	[ 'if \(\(status = bc_program_init\(&vm\.prog, len\)\)\) return status;',
	  'if ((toys.exitval = bc_program_init(&vm.prog, len))) return;' ],
	[ 'return status == BC_STATUS_QUIT \? BC_STATUS_SUCCESS : status;',
	  'toys.exitval = status == BC_STATUS_QUIT ? BC_STATUS_SUCCESS : status;' ],
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
	'^#define BC_FLAG_W \(1<<0\)$',
	'^#define BC_FLAG_S \(1<<1\)$',
	'^#define BC_FLAG_Q \(1<<2\)$',
	'^#define BC_FLAG_L \(1<<3\)$',
	'^#define BC_FLAG_I \(1<<4\)$',
	'^#define BC_FLAG_C \(1<<5\)$',
	'^#define BC_INVALID_IDX \(\(size_t\) -1\)$',
	'^#define BC_MAX\(a, b\) \(\(a\) > \(b\) \? \(a\) : \(b\)\)$',
	'^#define BC_MIN\(a, b\) \(\(a\) < \(b\) \? \(a\) : \(b\)\)$',
	'^  \/\/ This is last so I can remove it for toybox.\n  BC_STATUS_INVALID_OPTION,$',
	'^#define BC_PROGRAM_CHECK_STACK.*$',
	'^#define BC_PROGRAM_CHECK_RESULTS.*$',
]

regexes_all = [
	'^// \*\* Exclude start. \*\*$.*?^// \*\* Exclude end. \*\*$',
	'^#ifndef NDEBUG.*?^#endif \/\/ NDEBUG$',
	'\n  [ ]*assert\(.*?\);$'
]

replacements = [
	[ 'bcg.posix', '(toys.optflags & FLAG_s)' ],
	[ 'bcg.warn', '(toys.optflags & FLAG_w)' ],
	[ 'bcg.', 'TT.' ],
	[ 'BC_FLAG_Q', 'FLAG_q' ],
	[ 'BC_FLAG_L', 'FLAG_l' ],
	[ 'BC_FLAG_I', 'FLAG_i' ],
	[ 'BC_FLAG_C', 'FLAG_c' ],
	[ 'BC_MAX\(', 'maxof(' ],
	[ 'BC_MIN\(', 'minof(' ],
	[ 'BC_INVALID_IDX', '-1' ],
	[ ' bool', ' int' ],
	[ '^bool ', 'int ' ],
	[ ' true', ' 1' ],
	[ ' false', ' 0' ],
	[ 'true', '1' ],
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

