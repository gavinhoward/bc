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
	print("usage: {} [test_num exe]".format(script))
	print("\n    test_num is the last Karatsuba number to run through tests")
	sys.exit(1)

script = sys.argv[0]
testdir = os.path.dirname(script)

print("\nWARNING: This script is for distro and package maintainers.")
print("It is for finding the optimal Karatsuba number.")
print("It takes forever to run.")
print("You have been warned.\n")

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

	print("\nCompiling...\n")

	makecmd = [ "./configure.sh", "-O3", "-k{}".format(i) ]
	p = subprocess.run(makecmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

	if p.returncode != 0:
		print("configure.sh returned an error ({}); exiting...".format(p.returncode))
		sys.exit(p.returncode)

	makecmd = [ "make" ]
	p = subprocess.run(makecmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

	if p.returncode != 0:
		print("make returned an error ({}); exiting...".format(p.returncode))
		sys.exit(p.returncode)

	if (test_num >= i):

		print("Running tests...\n")

		for test in tests:

			cmd = [ "{}/tests/test.sh".format(testdir), "bc", test, "0", exe ]

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

opt = nums[times.index(min(times))]

print("\nOptimal Karatsuba Num (for this machine): {}".format(opt))
print("Run the following:\n")
print("./configure.sh -O3 -k {}".format(opt))
print("make")
