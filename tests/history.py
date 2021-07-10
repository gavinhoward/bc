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

# This array is for escaping characters that are necessary to escape when
# outputting to pexpect. Since pexpect takes regexes, these characters confuse
# it unless we escape them.
escapes = [
	']',
	'[',
	'+',
]

# UTF-8 stress tests.
utf8_stress1 = "\xe1\x86\xac\xe1\xb8\xb0\xe4\x8b\x94\xe4\x97\x85\xe3\x9c\xb2\xe0\xb2\xa4\xe5\x92\xa1\xe4\x92\xa2\xe5\xb2\xa4\xe4\xb3\xb0\xe7\xa8\xa8\xe2\xa3\xa1\xe5\xb6\xa3\xe3\xb7\xa1\xe5\xb6\x8f\xe2\xb5\x90\xe4\x84\xba\xe5\xb5\x95\xe0\xa8\x85\xe5\xa5\xb0\xe7\x97\x9a\xe3\x86\x9c\xe4\x8a\x9b\xe6\x8b\x82\xe4\x85\x99\xe0\xab\xa9\xe2\x9e\x8b\xe4\x9b\xbf\xe1\x89\xac\xe7\xab\xb3\xcd\xbf\xe1\x85\xa0\xe2\x9d\x84\xe4\xba\xa7\xe7\xbf\xb7\xe4\xae\x8a\xe0\xaf\xb7\xe1\xbb\x88\xe4\xb7\x92\xe4\xb3\x9c\xe3\x9b\xa0\xe2\x9e\x95\xe5\x82\x8e\xe1\x97\x8b\xe1\x8f\xaf\xe0\xa8\x95\xe4\x86\x90\xe6\x82\x99\xe7\x99\x90\xe3\xba\xa8"
utf8_stress2 = "\xe9\x9f\xa0\xec\x8b\xa7\xeb\x8f\xb3\xeb\x84\xa8\xed\x81\x9a\xee\x88\x96\xea\x89\xbf\xeb\xae\xb4\xee\xb5\x80\xed\x94\xb7\xea\x89\xb2\xea\xb8\x8c\xef\xbf\xbd\xee\x88\x83\xec\xb5\x9c\xeb\xa6\x99\xee\xb9\xa6\xea\xb1\x86\xe9\xb3\xac\xeb\x82\xbd\xea\xaa\x81\xed\x8d\xbc\xee\x97\xb9\xef\xa6\xb1\xed\x95\x90\xee\xa3\xb3\xef\xa0\xa5\xe9\xbb\x99\xed\x97\xb6\xea\xaa\x88\xef\x9f\x88\xeb\xae\xa9\xec\xad\x80\xee\xbe\xad\xe9\x94\xbb\xeb\x81\xa5\xe9\x89\x97\xea\xb2\x89\xec\x9a\x9e\xeb\xa9\xb0\xeb\x9b\xaf\xea\xac\x90\xef\x9e\xbb\xef\xbf\xbd\xef\xbb\xbc\xef\xbf\xbd\xef\x9a\xa3\xef\xa8\x81\xe9\x8c\x90\xef\xbf\xbd"
utf8_stress3 = "\xea\xb3\xbb\xef\xbf\xbd\xe4\xa3\xb9\xe6\x98\xb2\xef\x91\xa7\xe8\x9c\xb4\xe1\xbd\x9b\xe6\xa1\xa2\xe3\x8e\x8f\xe2\x9a\xa6\xef\x84\x8a\xe7\x8f\xa2\xe7\x95\xa3\xea\xb0\xb4\xef\xad\xb1\xe9\xb6\xb6\xe0\xb9\x85\xe2\xb6\x9b\xeb\x80\x81\xe5\xbd\xbb\xea\x96\x92\xe4\x94\xbe\xea\xa2\x9a\xef\xb1\xa4\xee\x96\xb0\xed\x96\x94\xed\x96\x9e\xe3\x90\xb9\xef\xbf\xbd\xe9\xbc\xb3\xeb\xb5\xa1\xee\xb1\x80\xe2\x96\xbf\xe2\xb6\xbe\xea\xa0\xa9\xef\xbf\xbd\xe7\xba\x9e\xe2\x8a\x90\xe4\xbd\xa7\xef\xbf\xbd\xe2\xb5\x9f\xe9\x9c\x98\xe7\xb4\xb3\xe3\xb1\x94\xe7\xb1\xa0\xeb\x8e\xbc\xe2\x8a\x93\xe6\x90\xa7\xef\x93\xa7\xe7\xa1\xa4"
utf8_stress4 = "\xe1\xa0\xb4\xe1\xa1\xaa\xe1\xa3\xb7\xe1\xa1\x8f\xe1\xa0\x87\xe1\xa1\xab\xe1\xa2\xb9\xe1\xa2\x82\xe1\xa3\x88\xe1\xa1\x9c\xe1\xa3\xa7\xe1\xa1\x87\xe1\xa0\x8e\xe1\xa3\xa8\xe1\xa1\xaf\xe1\xa0\xa8\xe1\xa3\x8f\xe1\xa0\xba\xe1\xa1\x90\xe1\xa1\xad\xe1\xa2\xad\xe1\xa0\x91\xe1\xa0\xbf\xe1\xa1\xaf\xe1\xa3\xb5\xe1\xa0\x96\xe1\xa1\x89\xe1\xa0\xbe\xe1\xa1\xa9\xe1\xa0\x86\xe1\xa3\x8b\xe1\xa0\x96\xe1\xa2\x9d\xe1\xa2\x88\xe1\xa0\x9b\xe1\xa1\x88\xe1\xa0\xb8\xe1\xa2\x98\xe1\xa2\xa8\xe1\xa0\x96\xe1\xa3\x98\xe1\xa1\xb6\xe1\xa0\xa4\xe1\xa3\xac\xe1\xa2\xbc\xe1\xa2\xb6\xe1\xa1\xae\xe1\xa0\xbd\xe1\xa0\x86\xe1\xa3\xaa"

# An easy array for UTF-8 tests.
utf8_stress_strs = [
	utf8_stress1,
	utf8_stress2,
	utf8_stress3,
	utf8_stress4,
]


def spawn(exe, args, env, encoding=None, codec_errors='strict'):
	if do_test:
		f = open(testdir + "/" + exedir + "_outputs/history_test.txt", "wb")
		return pexpect.popen_spawn.PopenSpawn([ exe ] + args, env=env,
				encoding=encoding, codec_errors=codec_errors, stderr=f)
	else:
		return pexpect.spawn(exe, args, env=env, encoding=encoding,
				codec_errors=codec_errors)


# Check that the child output the expected line. If history is false, then
# the output should change.
def check_line(child, expected, prompt=">>> ", history=True):
	child.send("\n")
	prefix = "\r\n" if history else ""
	child.expect(prefix + expected + "\r\n" + prompt)


# Write a string to output, checking all of the characters are output,
# one-by-one.
def write_str(child, s):
	for c in s:
		child.send(c)
		if c in escapes:
			child.expect("\\{}".format(c))
		else:
			child.expect(c)


# Check the bc banner.
# @param child  The child process.
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


# Common UTF-8 testing function. The index is the index into utf8_stress_strs
# for which stress string to use.
# @param exe   The executable.
# @param args  The arguments to pass to the executable.
# @param env   The environment.
# @param idx   The index of the UTF-8 stress string.
def test_utf8(exe, args, env, idx):

	# Because both bc and dc use this, make sure the banner doesn't pop.
	env["BC_BANNER"] = "0"

	child = pexpect.spawn(exe, args=args, env=env, encoding='utf-8', codec_errors='ignore')

	try:

		# Write the stress string.
		child.send(utf8_stress_strs[idx])
		child.send("\n")
		time.sleep(1)

		# Sleeping ensure the child has time to die.
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
	except pexpect.EOF:
		print("EOF")
		print(str(child))
		print(str(child.buffer))
		print(str(child.before))
		sys.exit(2)

	return child

# A random UTF-8 test with insert.
# @param exe   The executable.
# @param args  The arguments to pass to the executable.
# @param env   The environment.
def test_utf8_0(exe, args, env):

	# Because both bc and dc use this, make sure the banner doesn't pop.
	env["BC_BANNER"] = "0"

	child = pexpect.spawn(exe, args=args, env=env, encoding='utf-8')

	try:

		# Just random UTF-8 I generated somewhow, plus ensuring that insert works.
		write_str(child, "\xef\xb4\xaa\xc3\xa1\x61\xcc\x81\xcc\xb5\xcc\x97\xf0\x9f\x88\x90\x61\xcc\x83")
		child.send("\x1b[D\x1b[D\x1b[D\x1b\x1b[A\xe2\x84\x90")
		child.send("\n")

		# Sleeping ensure the child has time to die.
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
	except pexpect.EOF:
		print("EOF")
		print(str(child))
		print(str(child.buffer))
		print(str(child.before))
		sys.exit(2)

	return child


def test_utf8_1(exe, args, env):
	return test_utf8(exe, args, env, 0)


def test_utf8_2(exe, args, env):
	return test_utf8(exe, args, env, 1)


def test_utf8_3(exe, args, env):
	return test_utf8(exe, args, env, 2)


def test_utf8_4(exe, args, env):
	return test_utf8(exe, args, env, 3)


# This tests a SIGINT with reset followed by a SIGQUIT.
# @param exe   The executable.
# @param args  The arguments to pass to the executable.
# @param env   The environment.
def test_sigint_sigquit(exe, args, env):

	child = pexpect.spawn(exe, args=args, env=env)

	try:
		child.send("\t")
		child.expect("        ")
		child.send("\x03")
		child.send("\x1c")
	except pexpect.TIMEOUT:
		print("timed out")
		print(str(child))
		sys.exit(2)
	except pexpect.EOF:
		print("EOF")
		print(str(child))
		print(str(child.buffer))
		print(str(child.before))
		sys.exit(2)

	return child


# Test for EOF.
# @param exe   The executable.
# @param args  The arguments to pass to the executable.
# @param env   The environment.
def test_eof(exe, args, env):

	child = pexpect.spawn(exe, args=args, env=env)

	try:
		child.send("\t")
		child.expect("        ")
		child.send("\x04")
	except pexpect.TIMEOUT:
		print("timed out")
		print(str(child))
		sys.exit(2)
	except pexpect.EOF:
		print("EOF")
		print(str(child))
		print(str(child.buffer))
		print(str(child.before))
		sys.exit(2)

	return child


# Test for quiting SIGINT.
# @param exe   The executable.
# @param args  The arguments to pass to the executable.
# @param env   The environment.
def test_sigint(exe, args, env):

	env["BC_SIGINT_RESET"] = "0"
	env["DC_SIGINT_RESET"] = "0"

	child = pexpect.spawn(exe, args=args, env=env)

	try:
		child.send("\t")
		child.expect("        ")
		child.send("\x03")
	except pexpect.TIMEOUT:
		print("timed out")
		print(str(child))
		sys.exit(2)
	except pexpect.EOF:
		print("EOF")
		print(str(child))
		print(str(child.buffer))
		print(str(child.before))
		sys.exit(2)

	return child


# Basic bc test.
# @param exe   The executable.
# @param args  The arguments to pass to the executable.
# @param env   The environment.
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
	except pexpect.EOF:
		print("EOF")
		print(str(child))
		print(str(child.buffer))
		print(str(child.before))
		sys.exit(2)

	return child


# SIGINT with no history.
# @param exe   The executable.
# @param args  The arguments to pass to the executable.
# @param env   The environment.
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
	except pexpect.EOF:
		print("EOF")
		print(str(child))
		print(str(child.buffer))
		print(str(child.before))
		sys.exit(2)

	return child


# Left and right arrows.
# @param exe   The executable.
# @param args  The arguments to pass to the executable.
# @param env   The environment.
def test_bc3(exe, args, env):

	child = pexpect.spawn(exe, args=args, env=env)

	try:
		bc_banner(child)
		child.send("12\x1b[D3\x1b[C4\x1bOD5\x1bOC6")
		child.send("\n")
		check_line(child, "132546")
		child.send("12\x023\x064")
		child.send("\n")
		check_line(child, "1324")
		child.send("12\x1b[H3\x1bOH\x01\x1b[H45\x1bOF6\x05\x1b[F7\x1bOH8")
		child.send("\n")
		check_line(child, "84531267")
		write_str(child, "quit")
		child.send("\n")
	except pexpect.TIMEOUT:
		print("timed out")
		print(str(child))
		sys.exit(2)
	except pexpect.EOF:
		print("EOF")
		print(str(child))
		print(str(child.buffer))
		print(str(child.before))
		sys.exit(2)

	return child


# Up and down arrows.
# @param exe   The executable.
# @param args  The arguments to pass to the executable.
# @param env   The environment.
def test_bc4(exe, args, env):

	child = pexpect.spawn(exe, args=args, env=env)

	try:
		bc_banner(child)
		write_str(child, "15")
		check_line(child, "15")
		write_str(child, "2^16")
		check_line(child, "65536")
		child.send("\x1b[A\x1bOA")
		child.send("\n")
		check_line(child, "15")
		child.send("\x1b[A\x1bOA\x1b[A\x1b[B")
		check_line(child, "65536")
		child.send("\x1b[A\x1bOA\x0e\x1b[A\x1b[A\x1b[A\x1b[B\x10\x1b[B\x1b[B\x1bOB\x1b[B\x1bOA")
		child.send("\n")
		check_line(child, "65536")
		write_str(child, "quit")
		child.send("\n")
	except pexpect.TIMEOUT:
		print("timed out")
		print(str(child))
		sys.exit(2)
	except pexpect.EOF:
		print("EOF")
		print(str(child))
		print(str(child.buffer))
		print(str(child.before))
		sys.exit(2)

	return child


# Clear screen.
# @param exe   The executable.
# @param args  The arguments to pass to the executable.
# @param env   The environment.
def test_bc5(exe, args, env):

	child = pexpect.spawn(exe, args=args, env=env)

	try:
		bc_banner(child)
		child.send("\x0c")
		write_str(child, "quit")
		child.send("\n")
	except pexpect.TIMEOUT:
		print("timed out")
		print(str(child))
		sys.exit(2)
	except pexpect.EOF:
		print("EOF")
		print(str(child))
		print(str(child.buffer))
		print(str(child.before))
		sys.exit(2)

	return child


# Printed material without a newline.
# @param exe   The executable.
# @param args  The arguments to pass to the executable.
# @param env   The environment.
def test_bc6(exe, args, env):

	child = pexpect.spawn(exe, args=args, env=env)

	try:
		bc_banner(child)
		child.send("print \"Enter number: \"")
		child.send("\n")
		child.expect("Enter number: ")
		child.send("4\x1b[A\x1b[A")
		child.send("\n")
		write_str(child, "quit")
		child.send("\n")
	except pexpect.TIMEOUT:
		print("timed out")
		print(str(child))
		sys.exit(2)
	except pexpect.EOF:
		print("EOF")
		print(str(child))
		print(str(child.buffer))
		print(str(child.before))
		sys.exit(2)

	return child


# Word start and word end.
# @param exe   The executable.
# @param args  The arguments to pass to the executable.
# @param env   The environment.
def test_bc7(exe, args, env):

	child = pexpect.spawn(exe, args=args, env=env)

	try:
		bc_banner(child)
		write_str(child, "12 + 34 + 56 + 78 + 90")
		check_line(child, "270")
		child.send("\x1b[A")
		child.send("\x1bh\x1bh\x1bf + 14 ")
		child.send("\n")
		check_line(child, "284")
		write_str(child, "quit")
		child.send("\n")
	except pexpect.TIMEOUT:
		print("timed out")
		print(str(child))
		sys.exit(2)
	except pexpect.EOF:
		print("EOF")
		print(str(child))
		print(str(child.buffer))
		print(str(child.before))
		sys.exit(2)

	return child


# Backspace.
# @param exe   The executable.
# @param args  The arguments to pass to the executable.
# @param env   The environment.
def test_bc8(exe, args, env):

	child = pexpect.spawn(exe, args=args, env=env)

	try:
		bc_banner(child)
		child.send("12\x1b[D3\x1b[C4\x08\x7f")
		child.send("\n")
		check_line(child, "13")
		write_str(child, "quit")
		child.send("\n")
	except pexpect.TIMEOUT:
		print("timed out")
		print(str(child))
		sys.exit(2)
	except pexpect.EOF:
		print("EOF")
		print(str(child))
		print(str(child.buffer))
		print(str(child.before))
		sys.exit(2)

	return child


# Backspace and delete words.
# @param exe   The executable.
# @param args  The arguments to pass to the executable.
# @param env   The environment.
def test_bc9(exe, args, env):

	child = pexpect.spawn(exe, args=args, env=env)

	try:
		bc_banner(child)
		write_str(child, "12 + 34 + 56 + 78 + 90")
		check_line(child, "270")
		child.send("\x1b[A")
		child.send("\x1b[0;5D\x1b[0;5D\x1b[0;5D\x1b[0;5C\x1b[0;5D\x1bd\x1b[3~\x1b[d\x1b[d\x1b[d\x1b[d\x7f\x7f\x7f")
		child.send("\n")
		check_line(child, "102")
		child.send("\x1b[A")
		child.send("\x17\x17")
		child.send("\n")
		check_line(child, "46")
		write_str(child, "quit")
		child.send("\n")
	except pexpect.TIMEOUT:
		print("timed out")
		print(str(child))
		sys.exit(2)
	except pexpect.EOF:
		print("EOF")
		print(str(child))
		print(str(child.buffer))
		print(str(child.before))
		sys.exit(2)

	return child


# Backspace and delete words 2.
# @param exe   The executable.
# @param args  The arguments to pass to the executable.
# @param env   The environment.
def test_bc10(exe, args, env):

	child = pexpect.spawn(exe, args=args, env=env)

	try:
		bc_banner(child)
		write_str(child, "12 + 34 + 56 + 78 + 90")
		check_line(child, "270")
		child.send("\x1b[A\x1b[A\x1b[A\x1b[B\x1b[B\x1b[B\x1b[A")
		child.send("\n")
		check_line(child, "270")
		child.send("\x1b[A\x1b[0;5D\x1b[0;5D\x0b")
		child.send("\n")
		check_line(child, "180")
		child.send("\x1b[A\x1521")
		check_line(child, "21")
		write_str(child, "quit")
		child.send("\n")
	except pexpect.TIMEOUT:
		print("timed out")
		print(str(child))
		sys.exit(2)
	except pexpect.EOF:
		print("EOF")
		print(str(child))
		print(str(child.buffer))
		print(str(child.before))
		sys.exit(2)

	return child


# Swap.
# @param exe   The executable.
# @param args  The arguments to pass to the executable.
# @param env   The environment.
def test_bc11(exe, args, env):

	child = pexpect.spawn(exe, args=args, env=env)

	try:
		bc_banner(child)
		write_str(child, "12 + 34 + 56 + 78")
		check_line(child, "180")
		child.send("\x1b[A\x02\x14")
		check_line(child, "189")
		write_str(child, "quit")
		child.send("\n")
	except pexpect.TIMEOUT:
		print("timed out")
		print(str(child))
		sys.exit(2)
	except pexpect.EOF:
		print("EOF")
		print(str(child))
		print(str(child.buffer))
		print(str(child.before))
		sys.exit(2)

	return child


# Basic dc test.
# @param exe   The executable.
# @param args  The arguments to pass to the executable.
# @param env   The environment.
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
	except pexpect.EOF:
		print("EOF")
		print(str(child))
		print(str(child.buffer))
		print(str(child.before))
		sys.exit(2)

	return child


# SIGINT with quit.
# @param exe   The executable.
# @param args  The arguments to pass to the executable.
# @param env   The environment.
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
	except pexpect.EOF:
		print("EOF")
		print(str(child))
		print(str(child.buffer))
		print(str(child.before))
		sys.exit(2)

	return child


# Execute string.
# @param exe   The executable.
# @param args  The arguments to pass to the executable.
# @param env   The environment.
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
	except pexpect.EOF:
		print("EOF")
		print(str(child))
		print(str(child.buffer))
		print(str(child.before))
		sys.exit(2)

	return child


# The array of bc tests.
bc_tests = [
	test_utf8_0,
	test_utf8_1,
	test_utf8_2,
	test_utf8_3,
	test_utf8_4,
	test_sigint_sigquit,
	test_eof,
	test_sigint,
	test_bc1,
	test_bc2,
	test_bc3,
	test_bc4,
	test_bc5,
	test_bc6,
	test_bc7,
	test_bc8,
	test_bc9,
	test_bc10,
	test_bc11,
]

# The array of bc error codes.
bc_expected_codes = [
	4,
	4,
	4,
	4,
	4,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
]

# The array of dc tests.
dc_tests = [
	test_utf8_0,
	test_utf8_1,
	test_utf8_2,
	test_utf8_3,
	test_sigint_sigquit,
	test_eof,
	test_sigint,
	test_dc1,
	test_dc2,
	test_dc3,
]

# The array of dc error codes.
dc_expected_codes = [
	4,
	4,
	4,
	4,
	0,
	0,
	0,
	0,
	0,
	0,
]


# Print the usage and exit with an error.
def usage():
	print("usage: {} [-t] dir [-a] test_idx [exe options...]".format(script))
	print("       The valid values for dir are: 'bc' and 'dc'.")
	print("       The max test_idx for bc is {}.".format(len(bc_tests) - 1))
	print("       The max test_idx for dc is {}.".format(len(dc_tests) - 1))
	print("       If -a is given, the number of test for dir is printed.")
	print("       No tests are run.")
	sys.exit(1)


# Must run this script alone.
if __name__ != "__main__":
	usage()

if len(sys.argv) < 2:
	usage()

idx = 1

exedir = sys.argv[idx]

idx += 1

if exedir == "-t":
	do_test = True
	exedir = sys.argv[idx]
	idx += 1
else:
	do_test = False

test_idx = sys.argv[idx]

idx += 1

if test_idx == "-a":
	if exedir == "bc":
		l = len(bc_tests)
	else:
		l = len(dc_tests)
	print("{}".format(l))
	sys.exit(0)

test_idx = int(test_idx)

# Set a default executable unless we have one.
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

# This is the environment necessary for most tests.
env = {
	"BC_BANNER": "1",
	"BC_PROMPT": "1",
	"DC_PROMPT": "1",
	"BC_TTY_MODE": "1",
	"DC_TTY_MODE": "1",
	"BC_SIGINT_RESET": "1",
	"DC_SIGINT_RESET": "1",
}

# Make sure to include the outside environment.
env.update(os.environ)

# Run the correct test.
child = test_array[test_idx](exe[0], exe[1:], env)

child.close()

# Make sure we got the expected exit code.
exp = expected_codes[test_idx]
exit = child.exitstatus

if exit != exp:
	print("child failed; expected exit code {}, got {}".format(exp, exit))
	print(str(child))
	sys.exit(1)