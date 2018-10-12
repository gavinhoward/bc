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

import os, errno
import random
import sys
import subprocess

def silentremove(filename):
	try:
		os.remove(filename)
	except OSError as e:
		if e.errno != errno.ENOENT:
			raise

def finish():
	silentremove(math)
	silentremove(opfile)

def gen(limit=4):
	return random.randint(0, 2 ** (8 * limit))

def negative():
	return random.randint(0, 1) == 1

def zero():
	return random.randint(0, 2 ** (8) - 1) == 0

def num(op, neg, real, z, limit=4):

	if z:
		z = zero()
	else:
		z = False

	if z:
		return 0

	if neg:
		neg = negative()

	g = gen(limit)

	if real and negative():
		n = str(gen(25))
		length = gen(1)
		if len(n) < length:
			n = ("0" * (length - len(n))) + n
	else:
		n = "0"

	g = str(g)
	if n != "0":
		g = g + "." + n

	if neg and g != "0":
		if op != modexp:
			g = "-" + g
		else:
			g = "_" + g

	return g


def add(test, op):

	with open(math, "a") as f:
		f.write(test + "\n")

	with open(opfile, "a") as f:
		f.write(str(op) + "\n")

def compare(exe, options, p, test, halt, expected, op, do_add=True):

	if p.returncode != 0:

		print("    {} returned an error ({})".format(exe, p.returncode))

		if do_add:
			print("    adding {} to checklist...".format(test))
			add(test, op)

		return

	actual = p.stdout.decode()

	if actual != expected:

		if op >= exponent:

			indata = "scale += 10; {}; {}".format(test, halt)
			args = [ exe, options ]
			p2 = subprocess.run(args, input=indata.encode(), stdout=subprocess.PIPE, stderr=subprocess.PIPE)
			expected = p2.stdout[:-10].decode()

			if actual == expected:
				print("    failed because of bug in other {}".format(exe))
				print("    continuing...")
				return

		print("   failed {}".format(test))
		print("    expected:")
		print("        {}".format(expected))
		print("    actual:")
		print("        {}".format(actual))

		if do_add:
			print("   adding to checklist...")
			add(test, op)


def gen_test(op):

	scale = num(op, False, False, True, 5 / 8)

	if op < div:
		s = fmts[op].format(scale, num(op, True, True, True), num(op, True, True, True))
	elif op == div or op == mod:
		s = fmts[op].format(scale, num(op, True, True, True), num(op, True, True, False))
	elif op == power:
		s = fmts[op].format(scale, num(op, True, True, True, 7 / 8), num(op, True, False, True, 6 / 8))
	elif op == modexp:
		s = fmts[op].format(scale, num(op, True, False, True), num(op, True, False, True),
		                    num(op, True, False, False))
	elif op == sqrt:
		s = "1"
		while s == "1":
			s = num(op, False, True, True, 1)
		s = fmts[op].format(scale, s)
	else:

		if op == exponent:
			first = num(op, True, True, True, 6 / 8)
		elif op == bessel:
			first = num(op, False, True, True, 6 / 8)
		else:
			first = num(op, True, True, True)

		if op != bessel:
			s = fmts[op].format(scale, first)
		else:
			s = fmts[op].format(scale, first, 6 / 8)

	return s

def run_test(t):

	op = random.randrange(bessel + 1)

	if op != modexp:
		exe = "bc"
		halt = "halt"
		options = "-lq"
	else:
		exe = "dc"
		halt = "q"
		options = ""

	test = gen_test(op)

	bcexe = exedir + "/" + exe
	indata = test + "\n" + halt

	print("Test {}: {}".format(t, test))

	args = [ exe, options ]

	p = subprocess.run(args, input=indata.encode(), stdout=subprocess.PIPE, stderr=subprocess.PIPE)

	output1 = p.stdout.decode()

	if p.returncode != 0 or output1 == "":
		print("    other {} returned an error ({}); continuing...".format(exe, p.returncode))
		return

	args = [ bcexe, options ]

	p = subprocess.run(args, input=indata.encode(), stdout=subprocess.PIPE, stderr=subprocess.PIPE)
	compare(exe, options, p, test, halt, output1, op)


if __name__ != "__main__":
	sys.exit(1)

script = sys.argv[0]
testdir = os.path.dirname(script)

exedir = testdir + "/.."
math = exedir + "/.math.txt"
opfile = exedir + "/.ops.txt"

ops = [ '+', '-', '*', '/', '%', '^', '|' ]
files = [ "add", "subtract", "multiply", "divide", "modulus", "power", "modexp",
          "sqrt", "exponent", "log", "arctangent", "sine", "cosine", "bessel" ]
funcs = [ "sqrt", "e", "l", "a", "s", "c", "j" ]

fmts = [ "scale = {}; {} + {}", "scale = {}; {} - {}", "scale = {}; {} * {}",
         "scale = {}; {} / {}", "scale = {}; {} % {}", "scale = {}; {} ^ {}",
         "{}k {} {} {}|pR", "scale = {}; sqrt({})", "scale = {}; e({})",
         "scale = {}; l({})", "scale = {}; a({})", "scale = {}; s({})",
         "scale = {}; c({})", "scale = {}; j({}, {})" ]

div = 3
mod = 4
power = 5
modexp = 6
sqrt = 7
exponent = 8
bessel = 13

finish()

try:
	i = 0
	while True:
		run_test(i)
		i = i + 1
except KeyboardInterrupt:
	pass

if not os.path.exists(math):
	print("\nNo items in checklist.")
	print("Exiting")
	sys.exit(0)

print("\nGoing through the checklist...\n")

with open(math, "r") as f:
	tests = f.readlines()

with open(opfile, "r") as f:
	ops = f.readlines()

if len(tests) != len(ops):
	print("Corrupted checklist!")
	print("Exiting...")
	sys.exit(1)

for i in range(0, len(tests)):

	print("\n{}".format(tests[i]))

	op = int(ops[i])

	if op != modexp:
		exe = "bc"
		halt = "halt"
		options = "-lq"
	else:
		exe = "dc"
		halt = "q"
		options = ""

	indata = tests[i] + "\n" + halt

	args = [ exe, options ]

	p = subprocess.run(args, input=indata.encode(), stdout=subprocess.PIPE, stderr=subprocess.PIPE)

	expected = p.stdout.decode()

	bcexe = exedir + "/" + exe
	args = [ bcexe, options ]

	p = subprocess.run(args, input=indata.encode(), stdout=subprocess.PIPE, stderr=subprocess.PIPE)

	compare(exe, options, p, tests[i], halt, expected, op, False)

	answer = input("\nAdd test to test suite? [y/N]: ")

	if 'Y' in answer or 'y' in answer:
		print("Yes")
		continue

		name = testdir + "/" + exe + "/" + files[op]

		with open(name + ".txt", "a") as f:
			f.write(tests[i])

		with open(name + "_results.txt", "a") as f:
			f.write(expected)

	else:
		print("No")


finish()
