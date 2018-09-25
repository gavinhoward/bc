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
	echo "usage: $0 test [bc [bc_args...]]"
	echo "valid tests are:"
	echo ""
	cat "$testdir/all.txt"
	echo ""
	exit 1
fi

t="$1"
name="$testdir/$t.txt"
results="$testdir/${t}_results.txt"
shift

if [ "$#" -gt 0 ]; then
	bc="$1"
	shift
else
	bc="$testdir/../bc"
fi

out="$testdir/../.log_test.txt"

if [ ! -f "$name" ]; then
	echo "Generating $t..."
	"$testdir/scripts/$t.bc" > "$name"
fi

if [ ! -f "$results" ]; then
	echo "Generating $t results..."
	echo "halt" | bc -lq "$name" > "$results"
fi

echo "Running $t..."

echo "halt" | "$bc" "$@" -lq "$name" > "$out"

diff "$results" "$out"

rm -rf "$out"

