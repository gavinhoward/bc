#! /bin/sh
#
# Copyright (c) 2018-2019 Gavin D. Howard and contributors.
#
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# * Redistributions of source code must retain the above copyright notice, this
#   list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#

readlink() {

	local exe="$1"
	shift

	L=$(ls -dl "$exe")

	printf "${L#*-> }"
}

removeext() {

	local name="$1"
	shift

	printf "${name%.*}"
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
