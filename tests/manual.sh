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

if [ "$#" -lt 1 ]; then
	echo "usage: manual.sh <bc> [exprs...]"
	exit 1
fi

bc="$1"
shift

bcdir=$(dirname "${bc}")

out1="$bcdir/.log_bc.txt"
out2="$bcdir/.log_test.txt"

for string in "$@"; do

	echo "$string" | bc -q >> "$out1"
	echo "$string" | "$bc" -q >> "$out2"

done

diff "$out1" "$out2"

rm -rf "$out1" "$out2"
