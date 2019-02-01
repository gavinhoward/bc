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

# WARNING: Test files cannot have empty lines!

die() {

	local d="$1"
	shift

	local msg="$1"
	shift

	local name="$1"
	shift

	local err="$1"
	shift

	printf '\n'
	printf '%s %s on test:\n' "$d" "$msg"
	printf '\n'
	printf '    %s\n' "$name"
	printf '\n'
	printf 'exiting...\n'
	exit "$err"
}

checkcrash() {

	local error="$1"
	shift

	local name="$1"
	shift

	if [ "$error" -gt 127 ]; then
		die "$d" "crashed" "$name" "$error"
	fi
}

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

	checkcrash "$error" "$name"

	if [ "$error" -eq 0 ]; then
		die "$d" "returned no error" "$name" 127
	fi

	if [ ! -s "$out" ]; then
		die "$d" "produced no error message" "$name" "$error"
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
	printf 'usage: %s dir [exec args...]\n' "$script"
	exit 1
else
	d="$1"
	shift
fi

if [ "$#" -lt 1 ]; then
	exe="$testdir/../bin/$d"
else
	exe="$1"
	shift
fi

out="$testdir/../.log_${d}_test.txt"

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

		line="last"
		printf '%s\n' "$line" | "$exe" "$@" "-lw"  2> "$out" > /dev/null
		err="$?"

		if [ "$err" -ne 0 ]; then
			die "$d" "returned an error ($err)" "POSIX warning" 1
		fi

		checktest "1" "$line" "$out" "$exebase"

		options="-ls"
	else
		options="$opts"
	fi

	base=$(basename "$testfile")
	base="${base%.*}"
	printf 'Running %s...\n' "$base"

	while read -r line; do

		rm -f "$out"

		printf '%s\n' "$line" | "$exe" "$@" "$options" 2> "$out" > /dev/null
		err="$?"

		checktest "$err" "$line" "$out" "$exebase"

	done < "$testfile"

done

for testfile in $testdir/$d/errors/*.txt; do

	printf 'Running error file %s...\n' "$testfile"

	printf '%s\n' "$halt" | "$exe" "$@" $opts "$testfile" 2> "$out" > /dev/null
	err="$?"

	checktest "$err" "$testfile" "$out" "$exebase"

	printf 'Running error file %s through cat...\n' "$testfile"

	cat "$testfile" | "$exe" "$@" $opts 2> "$out" > /dev/null
	err="$?"

	checkcrash "$err" "$testfile"

done
