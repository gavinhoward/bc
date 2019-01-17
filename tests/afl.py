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
import shutil
import subprocess

def usage():
	print("usage: {} dir [exe results_dir]".format(script))
	sys.exit(1)

def check_crash(exebase, out, error, file, type, test):
	if error < 0:
		print("\n{} crashed ({}) on {}:\n".format(exebase, -error, type))
		print("    {}".format(test))
		print("\nCopying to \"{}\"".format(out))
		shutil.copy2(file, out)
		print("\nexiting...")
		sys.exit(error)

def run_test(exe, exebase, tout, indata, out, file, type, test):
	try:
		p = subprocess.run(exe, timeout=tout, input=indata, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
		check_crash(exebase, out, p.returncode, file, type, test)
	except subprocess.TimeoutExpired:
		print("\n    {} timed out. Continuing...\n".format(exebase))

def get_children(dir, get_files):
	dirs = []
	with os.scandir(dir) as it:
		for entry in it:
			if not entry.name.startswith('.') and     \
			   ((entry.is_dir() and not get_files) or \
			    (entry.is_file() and get_files)):
				dirs.append(entry.name)
	dirs.sort()
	return dirs

script = sys.argv[0]
testdir = os.path.dirname(script)

if __name__ != "__main__":
	usage()

tout = 3

if len(sys.argv) < 2:
	usage()

exedir = sys.argv[1]

if len(sys.argv) >= 3:
	exe = sys.argv[2]
else:
	exe = testdir + "/../bin/" + exedir

exebase = os.path.basename(exe)

if exebase == "bc":
	halt = "halt\n"
	options = "-lq"
else:
	halt = "q\n"
	options = "-x"

if len(sys.argv) >= 4:
	resultsdir = sys.argv[3]
else:
	if exedir == "bc":
		resultsdir = testdir + "/../../results"
	else:
		resultsdir = testdir + "/../../results_dc"

exe = [ exe, options ]
for i in range(4, len(sys.argv)):
	exe.append(sys.argv[i])

out = testdir + "/../.test.txt"

print(os.path.realpath(os.getcwd()))

dirs = get_children(resultsdir, False)

for d in dirs:

	d = resultsdir + "/" + d

	print(d)

	files = get_children(d + "/crashes/", True)

	for file in files:

		file = d + "/crashes/" + file

		print("    {}".format(file))

		base = os.path.basename(file)

		if base == "README.txt":
			continue

		with open(file, "rb") as f:
			lines = f.readlines()

		for l in lines:
			run_test(exe, exebase, tout, l, out, file, "test", l)

		print("        Running whole file...")

		run_test(exe + [ file ], exebase, tout, halt.encode(), out, file, "file", file)

		print("        Running file through stdin...")

		with open(file, "rb") as f:
			content = f.read()

		run_test(exe, exebase, tout, content, out, file, "running {} through stdin".format(file), file)

print("Done")

