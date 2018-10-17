#! /usr/bin/python3 -B
#
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

import os
import sys
import subprocess
import time

def usage():
	print("usage: {} [exe]".format(script))
	sys.exit(1)

script = sys.argv[0]
testdir = os.path.dirname(script)

if __name__ != "__main__":
	usage()

mx = 2000

num = "9" * mx

if len(sys.argv) >= 2:
	exe = sys.argv[1]
else:
	exe = testdir + "/../bin/bc"

indata = "for (i = 0; i < 10; ++i) {} * {}\nhalt".format(num, num)
print(indata)

times = [-1]

for i in range(1, mx):

	print("Testing Karatsuba Num: {}".format(i), end='', flush=True)

	makecmd = [ "make", "BC_NUM_KARATSUBA_LEN={}".format(i) ]
	p = subprocess.run(makecmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

	if p.returncode != 0:
		print("make returned an error ({}); exiting...".format(p.returncode))
		sys.exit(p.returncode)

	start = time.process_time()
	p = subprocess.run([ exe ], input=indata.encode(), stdout=subprocess.PIPE, stderr=subprocess.PIPE)
	end = time.process_time()

	if p.returncode != 0:
		print("bc returned an error; exiting...")
		sys.exit(p.returncode)

	times.append(end - start)
	print(", Time: {}".format(times[i]))

m = min(i for i in times if i > 0)

print("\nOptimal Karatsuba Num (for this machine): {}".format(times.index(m)))
