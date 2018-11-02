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
# Release script for bc (to put into toybox and busybox).

# ***WARNING***: This script is only intended to be used by the project
# maintainer to cut releases.

import sys
import os
import subprocess
import regex as re

def read_file(file):

	array = []

	with open(file) as f:
		array = f.read().splitlines()

	for i in range(0, len(array)):
		r = re.compile('\\n', re.M)
		array[i] = r.sub('\n', array[i])

	for i in range(0, len(array)):
		r = re.compile('\\t', re.M)
		array[i] = r.sub('\t', array[i])

	return array

def print_projects(scriptdir):
	with os.scandir(scriptdir) as it:
		for entry in it:
			if not entry.name.startswith('.') and entry.is_dir():
				print("    {}".format(entry.name))

def usage():
	print("usage: {} project repo\n".format(script))
	print("    valid projects are:\n")
	print_projects(scriptdir)
	sys.exit(1)

if __name__ != "__main__":
	usage()

cwd = os.path.realpath(os.getcwd())

script = os.path.realpath(sys.argv[0])
scriptdir = os.path.dirname(script)

if len(sys.argv) < 3:
	usage()

project = sys.argv[1].rstrip(os.sep)
repo = sys.argv[2].rstrip(os.sep)

projectdir = scriptdir + "/" + project

if not os.path.isdir(projectdir):
	print("Invalid project; valid projects are:\n")
	print_projects(scriptdir)
	sys.exit(1)

with open(projectdir + "/path.txt") as f:
	repo_bc = repo + "/" + f.readline()
	repo_bc = repo_bc[:-1]

os.chdir(scriptdir)
os.chdir("..")

res = subprocess.run(["make", "libcname"], stdout=subprocess.PIPE)

if res.returncode != 0:
	sys.exit(res.returncode)

libcname = res.stdout.decode("utf-8")[:-1]

res = subprocess.run(["make", libcname])

if res.returncode != 0:
	sys.exit(res.returncode)

content = ""

with open(projectdir + "/files.txt") as f:
	files = f.read().splitlines()

for name in files:

	if name == libcname:
		header_end = 2
		if project == "busybox":
			content += "\n// clang-format off\n"
	else:
		header_end = 22

	with open(name) as f:
		lines = f.readlines()

	# Skip the header lines.
	for i in range(header_end, len(lines)):
		content += lines[i]

	if name == libcname and project == "busybox":
		content += "\n// clang-format on\n"

if project == "toybox":
	r = re.compile('\t', re.M)
	content = r.sub('  ', content)

removals = read_file(projectdir + "/remove_dotall.txt")

for reg in removals:
	r = re.compile(reg, re.M | re.DOTALL)
	content = r.sub('', content)

removals = read_file(projectdir + "/remove.txt")

for reg in removals:
	r = re.compile(reg, re.M)
	content = r.sub('', content)

res = subprocess.run(["make", "version"], stdout=subprocess.PIPE)

if res.returncode != 0:
	sys.exit(res.returncode)

version = res.stdout.decode("utf-8")[:-1]

r = re.compile('BC_VERSION', re.M)
content = r.sub('"' + version + '"', content)

needles = read_file(projectdir + "/needles.txt")
replacements = read_file(projectdir + "/replacements.txt")

if len(needles) != len(replacements):
	print("Number of replacements do not match")
	sys.exit(1)

for i in range(0, len(needles)):
	r = re.compile(needles[i], re.M | re.DOTALL)
	content = r.sub(replacements[i], content)

# Make sure to cleanup newlines
r = re.compile('\\n', re.M)
content = r.sub('\n', content)

if project == "busybox":

	cmd = ["clang-format", "-style=file"]
	res = subprocess.run(cmd, input=content.encode(), stdout=subprocess.PIPE)

	if res.returncode != 0:
		print("Error running clang-format ({})\nExiting...".format(res.returncode))

	content = res.stdout.decode()

	content = re.sub('\n[\t]*// clang-format off', '', content)
	content = re.sub('\n[\t]*// clang-format on', '', content)

	content = re.sub('\n\{\n\n', '\n{\n', content)
	content = re.sub('[\n]+[\t]*;', ';', content)
	r = re.compile('^([\t]*)case (.*?) \{', re.M)
	content = r.sub('\\1case \\2\n\\1{', content)
	r = re.compile('^([\t]*)default(.*?) \{', re.M)
	content = r.sub('\\1default\\2\n\\1{', content)
	r = re.compile('^([\t]+)([ ]+)(.*?) \{', re.M)
	content = r.sub('\\1\\2\\3\n\\1{', content)
	content = re.sub('#else  //', '#else //', content)

with open(projectdir + "/header.c") as f:
	content = f.read() + content

content = re.sub('\n\n\n+', '\n\n', content)

os.chdir(cwd)

with open(repo_bc, 'w') as f:
	f.write(content)
