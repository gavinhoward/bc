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

mx = 520
mx2 = mx // 2
mn = 2

test_num = 0
exe = ""
exedir = ""

def usage():
	print("This script should not be run directly.")
	print("It is a library for karatsuba.py and burnzieg.py.")
	sys.exit(1)

def print_warning(name):
	print("\nWARNING: This script is for distro and package maintainers.")
	print("It is for finding the optimal {} number.".format(name))
	print("It takes forever to run.")
	print("You have been warned.\n")

def print_result(name, option, optimal):
	print("\nOptimal {} Num (for this machine): {}".format(name, optimal))
	print("Run the following:\n")
	print("./configure.sh -O3 -{} {}".format(option, optimal))
	print("make")

def parse_args():

	global test_num
	global exe
	global script
	global testdir
	global exedir

	if len(sys.argv) >= 2:
		test_num = int(sys.argv[1])
	else:
		test_num = 0

	if len(sys.argv) >= 3:
		exe = sys.argv[2]
	else:
		exe = testdir + "/bin/bc"

	script = sys.argv[0]
	testdir = os.path.dirname(script)
	exedir = os.path.dirname(exe)

def set_limits(maxi, mini):
	global mx
	global mx2
	global mn
	mx = maxi
	mx2 = mx // 2
	mn = mini

def run(name, indata, tests, option):

	times = []
	nums = []
	runs = []
	nruns = 5

	for i in range(0, nruns):
		runs.append(0)

	for i in range(mn, mx2 + 1):

		print("\nCompiling...\n")

		makecmd = [ "./configure.sh", "-O3", "-{}{}".format(option, i) ]
		p = subprocess.run(makecmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

		if p.returncode != 0:
			print("\nconfigure.sh returned an error ({}); exiting...".format(p.returncode))
			sys.exit(p.returncode)

		makecmd = [ "make" ]
		p = subprocess.run(makecmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

		if p.returncode != 0:
			print("\nmake returned an error ({}); exiting...".format(p.returncode))
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

		elif test_num == 0:

			print("Timing {} Num: {}".format(name, i), end='', flush=True)

			for j in range(0, nruns):

				cmd = [ exe, "{}/tests/bc/power.txt".format(testdir) ]

				start = time.perf_counter()
				p = subprocess.run(cmd, input=indata.encode(), stdout=subprocess.PIPE, stderr=subprocess.PIPE)
				end = time.perf_counter()

				if p.returncode != 0:
					print("\nbc returned an error; exiting...")
					sys.exit(p.returncode)

				runs[j] = end - start

			run_times = runs[1:]
			avg = sum(run_times) / len(run_times)

			times.append(avg)
			nums.append(i)
			print(", Time: {}".format(times[i - mn]))

	return nums[times.index(min(times))]

if __name__ == "__main__":
	usage()
