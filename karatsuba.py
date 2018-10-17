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
import numpy as np

def usage():
	print("usage: {} [test_num exe]".format(script))
	print("\n    test_num is the last Karatsuba number to run through tests")
	sys.exit(1)

script = sys.argv[0]
testdir = os.path.dirname(script)

if __name__ != "__main__":
	usage()

mx = 200
mx2 = mx // 2
mn = 2

num = "9" * mx

if len(sys.argv) >= 2:
	test_num = int(sys.argv[1])
else:
	test_num = 0

if len(sys.argv) >= 3:
	exe = sys.argv[2]
else:
	exe = testdir + "/bin/bc"

exedir = os.path.dirname(exe)

indata = "for (i = 0; i < 100; ++i) {} * {}\nhalt".format(num, num)

times = []
nums = []

tests = [ "multiply", "modulus", "power", "sqrt" ]

for i in range(mn, mx2 + 1):

	makecmd = [ "make", "BC_NUM_KARATSUBA_LEN={}".format(i) ]
	p = subprocess.run(makecmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

	if p.returncode != 0:
		print("make returned an error ({}); exiting...".format(p.returncode))
		sys.exit(p.returncode)

	if (test_num >= i):

		print("\nRunning tests...\n")

		for test in tests:

			cmd = [ "{}/tests/test.sh".format(testdir), "bc", test, exe ]

			p = subprocess.run(cmd + sys.argv[3:], stderr=subprocess.PIPE)

			if p.returncode != 0:
				print("{} test failed:\n".format(test, p.returncode))
				print(p.stderr.decode())
				print("\nexiting...")
				sys.exit(p.returncode)

		print("")

	print("Timing Karatsuba Num: {}".format(i), end='', flush=True)

	cmd = [ exe, "{}/tests/bc/power.txt".format(testdir) ]

	start = time.perf_counter()
	p = subprocess.run(cmd, input=indata.encode(), stdout=subprocess.PIPE, stderr=subprocess.PIPE)
	end = time.perf_counter()

	if p.returncode != 0:
		print("bc returned an error; exiting...")
		sys.exit(p.returncode)

	nums.append(i)
	times.append(end - start)
	print(", Time: {}".format(times[i - mn]))

# Code found at: https://stackoverflow.com/questions/29634217/get-minimum-points-of-numpy-poly1d-curve#29635450
poly = np.polyfit(np.array(nums), np.array(times), 2)
c = np.poly1d(poly)
crit = c.deriv().r
r_crit = crit[crit.imag==0].real
tan = c.deriv(2)(r_crit)
x_min = r_crit[tan>0]

print("\nOptimal Karatsuba Num (for this machine): {}".format(int(round(x_min[0]))))
