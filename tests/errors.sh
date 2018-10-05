#! /bin/bash
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

# WARNING: Test files cannot have empty lines!

checktest()
{
	local error="$1"
	shift

	local name="$1"
	shift

	local out="$1"
	shift

	local exebase="$1"
	shift

	if [ "$error" -eq 0 ]; then
		echo "\n$d returned no error on test:\n"
		echo "    $name"
		echo "\nexiting..."
		exit 127
	fi

	if [ "$error" -gt 127 ]; then
		echo "\n$d crashed on test:\n"
		echo "    $name"
		echo "\nexiting..."
		exit "$error"
	fi

	if [ ! -s "$out" ]; then
		echo "\n$d produced no error message on test:\n"
		echo "    $name"
		echo "\nexiting..."
		exit "$error"
	fi

	# Display the error messages if not directly running exe.
	# This allows the script to print valgrind output.
	if [ "$exebase" != "bc" -a "$exebase" != "dc" ]; then
		cat "$out"
	fi
}

script="$0"
testdir=$(dirname "$script")

if [ "$#" -eq 0 ]; then
	echo "usage: $script dir [exec args...]"
	exit 1
else
	d="$1"
	shift
fi

if [ "$#" -lt 1 ]; then
	exe="$testdir/../$d"
else
	exe="$1"
	shift
fi

out="$testdir/../.log_test.txt"

exebase=$(basename "$exe")

posix="posix_errors"

if [ "$d" = "bc" ]; then
	opts="-l"
	halt="halt"
else
	opts="-x"
	halt="q"
fi

for testfile in $testdir/$d/*errors.txt; do

	if [ -z "${testfile##*$posix*}" ]; then
		options="-ls"
	else
		options="$opts"
	fi

	base=$(basename "$testfile")
	base="${base%.*}"
	echo "Running $base..."

	while read -r line; do

		rm -f "$out"

		echo "$line" | "$exe" "$@" "$options" 2> "$out" > /dev/null

		err="$?"

		checktest "$err" "$line" "$out" "$exebase"

	done < "$testfile"

done

for testfile in $testdir/$d/errors/*.txt; do

	echo "Running error file \"$testfile\"..."

	echo "$halt" | "$exe" "$@" $opts "$testfile" 2> "$out" > /dev/null

	checktest "$?" "$testfile" "$out" "$exebase"

done
