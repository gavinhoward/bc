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

	local f="$1"
	shift

	local arrow="-> "
	local d=$(dirname "$f")

	local lsout=""
	local link=""

	lsout=$(ls -dl "$f")
	link=$(printf '%s' "${lsout#*$arrow}")

	while [ -z "${lsout##*$arrow*}" ]; do
		f="$d/$link"
		d=$(dirname "$f")
		lsout=$(ls -dl "$f")
		link=$(printf '%s' "${lsout#*$arrow}")
	done

	printf '%s' "${f##*$d/}"
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

substring_replace() {

	local str="$1"
	shift

	local needle="$1"
	shift

	local replacement="$1"
	shift

	result=$(printf '%s' "$str" | sed -e "s!$needle!$replacement!g")

	printf '%s\n' "$result"
}

gen_nlspath() {

	local nlspath="$1"
	shift

	local locale="$1"
	shift

	local execname="$1"
	shift

	local char="@"
	local modifier="${locale#*$char}"
	local tmplocale="${locale%%$char*}"

	char="."
	local charset="${tmplocale#*$char}"
	tmplocale="${tmplocale%%$char*}"

	if [ "$charset" = "$tmplocale" ]; then
		charset=""
	fi

	char="_"
	local territory="${tmplocale#*$char}"
	local language="${tmplocale%%$char*}"

	if [ "$territory" = "$tmplocale" ]; then
		territory=""
	fi

	if [ "$language" = "$tmplocale" ]; then
		language=""
	fi

	local needles="%%:%L:%N:%l:%t:%c"

	needles=$(printf '%s' "$needles" | tr ':' '\n')

	for i in $needles; do
		nlspath=$(substring_replace "$nlspath" "$i" "|$i|")
	done

	nlspath=$(substring_replace "$nlspath" "%%" "%")
	nlspath=$(substring_replace "$nlspath" "%L" "$locale")
	nlspath=$(substring_replace "$nlspath" "%N" "$execname")
	nlspath=$(substring_replace "$nlspath" "%l" "$language")
	nlspath=$(substring_replace "$nlspath" "%t" "$territory")
	nlspath=$(substring_replace "$nlspath" "%c" "$charset")

	nlspath=$(printf '%s' "$nlspath" | tr -d '|')

	printf '%s' "$nlspath"
}
