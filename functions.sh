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

readlink() {

	local exe="$1"
	shift

	L=$(ls -dl "$exe")

	printf ${L#*-> }
}

removeext() {

	local name="$1"
	shift

	printf '%s' "$name" | cut -f 1 -d '.'
}

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

	if [ "$error" -eq 100 ]; then

		output=$(cat "$out")
		fatal_error="Fatal error"

		if [ "${output##*$fatal_error*}" ]; then
			printf "%s\n" "$output"
			die "$d" "had memory errors on a non-fatal error" "$name" "$error"
		fi
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
