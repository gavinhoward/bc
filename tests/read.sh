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

. "$testdir/functions.sh"

if [ "$#" -lt 1 ]; then
	printf 'usage: %s dir [exe [args...]]\n' "$0"
	printf 'valid dirs are:\n'
	printf '\n'
	cat "$testdir/all.txt"
	printf '\n'
	exit 1
fi

d="$1"
shift

if [ "$#" -gt 0 ]; then
	exe="$1"
	shift
else
	exe="$testdir/../bin/$d"
fi

name="$testdir/$d/read.txt"
results="$testdir/$d/read_results.txt"
errors="$testdir/$d/read_errors.txt"

out="$testdir/../.log_${d}_test.txt"

exebase=$(basename "$exe")

if [ "$d" = "bc" ]; then
	options="-lq"
	halt="halt"
else
	options="-x"
	halt="q"
fi

if [ "$d" = "bc" ]; then
	read_call="read()"
	read_expr="${read_call}\n5+5;"
else
	read_call="?"
	read_expr="${read_call}"
fi

printf 'Running %s read...\n' "$d"

while read line; do

	printf '%s\n%s\n' "$read_call" "$line" | "$exe" "$@" "$options" > "$out"
	diff "$results" "$out"

done < "$name"

set +e

printf 'Running %s read errors...\n' "$d"

while read line; do

	printf '%s\n%s\n' "$read_call" "$line" | "$exe" "$@" "$options" 2> "$out" > /dev/null
	err="$?"

	checktest "$err" "$line" "$out" "$exebase"

done < "$errors"

printf 'Running %s empty read...\n' "$d"

read_test=$(printf '%s\n' "$read_call")

printf '%s\n' "$read_test" | "$exe" "$@" "$opts" 2> "$out" > /dev/null
err="$?"

checktest "$err" "$read_test" "$out" "$exebase"

printf 'Running %s read EOF...\n' "$d"

read_test=$(printf '%s' "$read_call")

printf '%s' "$read_test" | "$exe" "$@" "$opts" 2> "$out" > /dev/null
err="$?"

checktest "$err" "$read_test" "$out" "$exebase"
