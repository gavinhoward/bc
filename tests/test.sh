#! /bin/sh
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

set -e

script="$0"

testdir=$(dirname "$script")

if [ "$#" -lt 2 ]; then
	printf 'usage: %s dir test [generate_tests] [exe [args...]]\n' "$0"
	printf 'valid dirs are:\n'
	printf '\n'
	cat "$testdir/all.txt"
	printf '\n'
	exit 1
fi

d="$1"
shift

t="$1"
name="$testdir/$d/$t.txt"
results="$testdir/$d/${t}_results.txt"
shift

if [ "$#" -gt 0 ]; then
	generate_tests="$1"
	shift
else
	generate_tests=1
fi

if [ "$#" -gt 0 ]; then
	exe="$1"
	shift
else
	exe="$testdir/../bin/$d"
fi

out="$testdir/../.log_test.txt"

if [ "$d" = "bc" ]; then
	options="-lq"
	var="BC_LINE_LENGTH"
	halt="halt"
else
	options=""
	var="DC_LINE_LENGTH"
	halt="q"
fi

if [ ! -f "$name" ]; then

	if [ "$generate_tests" -eq 0 ]; then
		printf 'Skipping %s %s test\n' "$d" "$t"
		exit 0
	fi

	printf 'Generating %s %s...\n' "$d" "$t"
	"$testdir/$d/scripts/$t.$d" > "$name"
fi

if [ ! -f "$results" ]; then
	printf 'Generating %s %s results...\n' "$d" "$t"
	printf '%s\n' "$halt" | "$d" $options "$name" > "$results"
fi

if [ "$d" = "dc" ]; then
	options="-x"
fi

export $var=string

printf 'Running %s %s...\n' "$d" "$t"

printf '%s\n' "$halt" | "$exe" "$@" $options "$name" > "$out"

diff "$results" "$out"

rm -rf "$out"
