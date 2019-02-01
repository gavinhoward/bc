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

# Tests the timeconst.bc script from the Linux kernel build.
# You can find the script at kernel/time/timeconst.bc in any Linux repo.
# One such repo is: https://github.com/torvalds/linux

# Note: when testing this bc in toybox, make sure the link is in the same
# directory as the toybox binary, or this script will not work.

script="$0"
testdir=$(dirname "$script")

if [ "$#" -gt 0 ]; then
	timeconst="$1"
	shift
else
	timeconst="$testdir/scripts/timeconst.bc"
fi

if [ "$#" -gt 0 ]; then
	bc="$1"
	shift
else
	bc="$testdir/../../bin/bc"
fi

out1="$testdir/../.log_bc_timeconst.txt"
out2="$testdir/../.log_bc_timeconst_test.txt"

base=$(basename "$timeconst")

if [ ! -f "$timeconst" ]; then
	printf 'Warning: %s does not exist\n' "$timeconst"
	printf 'Skipping...\n'
	exit 0
fi

printf 'Running %s...\n' "$base"

for i in $(seq 0 1000); do

	printf '%s\n' "$i" | bc -q "$timeconst" > "$out1"
	printf '%s\n' "$i" | "$bc" "$@" -q "$timeconst" > "$out2"

	diff "$out1" "$out2"

	error="$?"

	if [ "$error" -ne 0 ]; then
		printf 'Failed on input: %s\n' "$i"
		exit "$error"
	fi

done

rm -rf "$out1" "$out2"
