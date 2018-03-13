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

res = subprocess.run(["make", "bc_mathlib"])

if res.returncode != 0:
	sys.exit(res.returncode)

content = ""

with open(testdir + "/files.txt") as f:
	files = f.read().splitlines()

for name in files:

	with open(name) as f:
		lines = f.readlines()

	# Skip the header lines.
	for i in range(22, len(lines)):
		content += lines[i]

regexes = [
	'^#include .*$',
	'^#ifndef BC.*_H$',
	'^#define BC.*_H$',
	'^#endif \/\/ BC.*_H$',
	'^extern.*$',
	'^static ',
	'^#define BC_FLAG_WARN \(1<<0\)$',
	'^#define BC_FLAG_STANDARD \(1<<1\)$',
	'^#define BC_FLAG_QUIET \(1<<2\)$',
	'^#define BC_FLAG_MATHLIB \(1<<3\)$',
	'^#define BC_FLAG_INTERACTIVE \(1<<4\)$',
	'^#define BC_FLAG_CODE \(1<<5\)$',
]

regexes_all = [
	'^// \*\* Exclude start. \*\*$.*?^// \*\* Exclude end. \*\*$'
]

replacements = [
	[ 'bcg.', 'TT.' ],
	[ 'BC_FLAG_WARN', 'FLAG_w' ],
	[ 'BC_FLAG_STANDARD', 'FLAG_s' ],
	[ 'BC_FLAG_QUIET', 'FLAG_q' ],
	[ 'BC_FLAG_MATHLIB', 'FLAG_l' ],
	[ 'BC_FLAG_INTERACTIVE', 'FLAG_i' ],
	[ 'BC_FLAG_CODE', 'FLAG_c' ],
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

with open(testdir + "/footer.c") as f:
	content += f.read()

content = re.sub('\n\n\n+', '\n\n', content)

os.chdir(cwd)

with open(toybox_bc, 'w') as f:
	f.write(content)

