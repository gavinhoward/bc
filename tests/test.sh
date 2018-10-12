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
	echo "usage: $0 dir test [exe [args...]]"
	echo "valid dirs are:"
	echo ""
	cat "$testdir/all.txt"
	echo ""
	exit 1
fi

d="$1"
shift

t="$1"
name="$testdir/$d/$t.txt"
results="$testdir/$d/${t}_results.txt"
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
	halt="halt"
else
	options="-x"
	halt="q"
fi

if [ ! -f "$name" ]; then
	echo "Generating $d $t..."
	"$testdir/$d/scripts/$t.$d" > "$name"
fi

if [ ! -f "$results" ]; then
	echo "Generating $d $t results..."
	echo "halt" | $d "$name" > "$results"
fi

echo "Running $d $t..."

echo "$halt" | "$exe" "$@" $options "$name" > "$out"

diff "$results" "$out"

rm -rf "$out"

