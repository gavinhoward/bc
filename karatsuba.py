#! /usr/bin/python3 -B
#
# Copyright (c) 2018-2019 Gavin D. Howard and contributors.
#
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# * Redistributions of source code must retain the above copyright notice, this
#   list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
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
print("Though it only needs to be run once per release/platform,")
print("it takes forever to run.")
print("You have been warned.\n")
print("Note: If you send an interrupt, it will report the current best number.\n")

if __name__ != "__main__":
	usage()

mx = 520
mx2 = mx // 2
mn = 16

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

indata = "for (i = 0; i < 100; ++i) {} * {}\n"
indata += "1.23456789^100000\n1.23456789^100000\nhalt"
indata = indata.format(num, num)

times = []
nums = []
runs = []
nruns = 5

for i in range(0, nruns):
	runs.append(0)

tests = [ "multiply", "modulus", "power", "sqrt" ]
scripts = [ "multiply" ]

if test_num != 0:
	mx2 = test_num

try:

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

			print("Running tests for Karatsuba Num: {}\n".format(i))

			for test in tests:

				cmd = [ "{}/tests/test.sh".format(testdir), "bc", test, "0", exe ]

				p = subprocess.run(cmd + sys.argv[3:], stderr=subprocess.PIPE)

				if p.returncode != 0:
					print("{} test failed:\n".format(test, p.returncode))
					print(p.stderr.decode())
					print("\nexiting...")
					sys.exit(p.returncode)

			print("")

			for script in scripts:

				cmd = [ "{}/tests/script.sh".format(testdir), "bc", script + ".bc",
				        "0", "1", "0", exe ]

				p = subprocess.run(cmd + sys.argv[3:], stderr=subprocess.PIPE)

				if p.returncode != 0:
					print("{} test failed:\n".format(test, p.returncode))
					print(p.stderr.decode())
					print("\nexiting...")
					sys.exit(p.returncode)

			print("")

		elif test_num == 0:

			print("Timing Karatsuba Num: {}".format(i), end='', flush=True)

			for j in range(0, nruns):

				cmd = [ exe, "{}/tests/bc/power.txt".format(testdir) ]

				start = time.perf_counter()
				p = subprocess.run(cmd, input=indata.encode(), stdout=subprocess.PIPE, stderr=subprocess.PIPE)
				end = time.perf_counter()

				if p.returncode != 0:
					print("bc returned an error; exiting...")
					sys.exit(p.returncode)

				runs[j] = end - start

			run_times = runs[1:]
			avg = sum(run_times) / len(run_times)

			times.append(avg)
			nums.append(i)
			print(", Time: {}".format(times[i - mn]))

except KeyboardInterrupt:
	nums = nums[0:i]
	times = times[0:i]

if test_num == 0:

	opt = nums[times.index(min(times))]

	print("\n\nOptimal Karatsuba Num (for this machine): {}".format(opt))
	print("Run the following:\n")
	print("./configure.sh -O3 -k {}".format(opt))
	print("make")
