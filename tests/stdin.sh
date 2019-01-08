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

out="$testdir/../.log_test.txt"

if [ "$d" = "bc" ]; then
	options="-lq"
else
	options="-x"
fi

rm -f "$out"

printf 'Running %s stdin tests...\n' "$d"

while read t; do
	printf '%s' "$t" | "$exe" "$@" "$options" >> "$out"
done < "$testdir/$d/stdin.txt"

diff "$testdir/$d/stdin_results.txt" "$out"

rm -rf "$out1"
