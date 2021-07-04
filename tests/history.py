#! /usr/bin/python

import os, sys
import time

try:
	import pexpect
except ImportError:
	print("Could not find pexpect. Skipping...")
	sys.exit(0)

# Housekeeping.
script = sys.argv[0]
testdir = os.path.dirname(script)

prompt = ">>> "

escapes = [
	']',
	'[',
	'+',
]


# Print the usage and exit with an error.
def usage():
	print("usage: {} dir test_idx [exe options...]".format(script))
	print("       The valid values for dir are: 'bc' and 'dc'.")
	print("       The max test_idx for bc is 2.")
	print("       The max test_idx for dc is 1.")
	sys.exit(1)


def check_line(child, expected, prompt=">>> ", history=True):
	child.send("\n")
	prefix = "\r\n" if history else ""
	child.expect(prefix + expected + "\r\n" + prompt)


def write_str(child, s):
	for c in s:
		child.send(c)
		if c in escapes:
			child.expect("\\{}".format(c))
		else:
			child.expect(c)


def bc_banner(child):
	bc_banner1 = "bc [0-9]+\.[0-9]+\.[0-9]+\r\n"
	bc_banner2 = "Copyright \(c\) 2018-[2-9][0-9][0-9][0-9] Gavin D. Howard and contributors\r\n"
	bc_banner3 = "Report bugs at: https://git.yzena.com/gavin/bc\r\n\r\n"
	bc_banner4 = "This is free software with ABSOLUTELY NO WARRANTY.\r\n\r\n"
	child.expect(bc_banner1)
	child.expect(bc_banner2)
	child.expect(bc_banner3)
	child.expect(bc_banner4)
	child.expect(prompt)


def test_bc1(exe, args, env):

	child = pexpect.spawn(exe, args=args, env=env)

	try:
		bc_banner(child)
		write_str(child, "1")
		check_line(child, "1")
		write_str(child, "1")
		check_line(child, "1")
		write_str(child, "quit")
		child.send("\n")
	except pexpect.TIMEOUT:
		print("timed out")
		print(str(child))
		sys.exit(2)

	return child


def test_bc2(exe, args, env):

	env["TERM"] = "dumb"

	child = pexpect.spawn(exe, args=args, env=env)

	try:
		bc_banner(child)
		child.sendline("1")
		check_line(child, "1", history=False)
		time.sleep(1)
		child.sendintr()
		child.sendline("quit")
	except pexpect.TIMEOUT:
		print("timed out")
		print(str(child))
		sys.exit(2)

	return child


def test_bc3(exe, args, env):

	child = pexpect.spawn(exe, args=args, env=env, encoding='utf-8')

	try:
		bc_banner(child)
		write_str(child, "\xc3\xa1\x61\xcc\x81\xcc\xb5\xcc\x97\xf0\x9f\x88\x90")
		child.send("\x1b[D\x1b[D\xe2\x84\x90")
		child.send("\n")
		time.sleep(1)
		if child.isalive():
			print("child did not give a fatal error")
			print(str(child))
			print(str(child.buffer))
			sys.exit(1)
		child.wait()
	except pexpect.TIMEOUT:
		print("timed out")
		print(str(child))
		sys.exit(2)

	return child


def test_bc4(exe, args, env):

	child = pexpect.spawn(exe, args=args, env=env)

	try:
		bc_banner(child)
		write_str(child, "15")
		check_line(child, "15")
		write_str(child, "2^16")
		check_line(child, "65536")
		child.send("\x1b[A\x1b[A")
		child.send("\n")
		check_line(child, "15")
		child.send("\x1b[A\x1b[A\x1b[A\x1b[B")
		child.send("\n")
		check_line(child, "65536")
		write_str(child, "quit")
		child.send("\n")
	except pexpect.TIMEOUT:
		print("timed out")
		print(str(child))
		sys.exit(2)

	return child


def test_dc1(exe, args, env):

	child = pexpect.spawn(exe, args=args, env=env)

	try:
		write_str(child, "1pR")
		check_line(child, "1")
		write_str(child, "1pR")
		check_line(child, "1")
		write_str(child, "q")
		child.send("\n")
	except pexpect.TIMEOUT:
		print("timed out")
		print(str(child))
		sys.exit(2)

	return child


def test_dc2(exe, args, env):

	env["TERM"] = "dumb"

	child = pexpect.spawn(exe, args=args, env=env)

	try:
		child.sendline("1pR")
		check_line(child, "1", history=False)
		time.sleep(1)
		child.sendintr()
		child.sendline("q")
	except pexpect.TIMEOUT:
		print("timed out")
		print(str(child))
		sys.exit(2)

	return child


def test_dc3(exe, args, env):

	child = pexpect.spawn(exe, args=args, env=env)

	try:
		write_str(child, "[1 15+pR]x")
		check_line(child, "16")
		write_str(child, "1pR")
		check_line(child, "1")
		write_str(child, "q")
		child.send("\n")
	except pexpect.TIMEOUT:
		print("timed out")
		print(str(child))
		sys.exit(2)

	return child


bc_tests = [
	test_bc1,
	test_bc2,
	test_bc3,
	test_bc4,
]

bc_expected_codes = [
	0,
	0,
	4,
	0,
]

dc_tests = [
	test_dc1,
	test_dc2,
	test_dc3,
]

dc_expected_codes = [
	0,
	0,
	0,
]

# Must run this script alone.
if __name__ != "__main__":
	usage()

if len(sys.argv) < 2:
	usage()

idx = 1

exedir = sys.argv[idx]

idx += 1

test_idx = int(sys.argv[idx])

idx += 1

if len(sys.argv) >= idx + 1:
	exe = sys.argv[idx]
else:
	exe = testdir + "/../bin/" + exedir

exebase = os.path.basename(exe)

# Use the correct options.
if exebase == "bc":
	halt = "halt\n"
	options = "-lq"
	test_array = bc_tests
	expected_codes = bc_expected_codes
else:
	halt = "q\n"
	options = "-x"
	test_array = dc_tests
	expected_codes = dc_expected_codes

# More command-line processing.
if len(sys.argv) > idx + 1:
	exe = [ exe, sys.argv[idx + 1:], options ]
else:
	exe = [ exe, options ]

env = {
	"BC_BANNER": "1",
	"BC_PROMPT": "1",
	"DC_PROMPT": "1",
	"BC_TTY_MODE": "1",
	"DC_TTY_MODE": "1",
}

env.update(os.environ)

child = test_array[test_idx - 1](exe[0], exe[1:], env)

child.close()

exp = expected_codes[test_idx - 1]
exit = child.exitstatus

if exit != exp:
	print("child failed; expected exit code {}, got {}".format(exp, exit))
	print(str(child.buffer))
	sys.exit(1)
