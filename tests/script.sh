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

s="$0"

testdir=$(dirname "$s")

if [ "$#" -lt 1 ]; then
	echo "usage: script.sh <script> [bc]"
	exit 1
fi

script="$1"
shift

if [ "$#" -lt 1 ]; then
	bc="$testdir/../bc"
else
	bc="$1"
fi

set -e

bcdir=$(dirname "${bc}")

out1="$bcdir/.log_bc.txt"
out2="$bcdir/.log_test.txt"

echo "quit" | bc -lq "$script" > "$out1"
echo "quit" | "$bc" "$@" -lq "$script" > "$out2"

diff "$out1" "$out2"

rm -rf "$out1" "$out2"
