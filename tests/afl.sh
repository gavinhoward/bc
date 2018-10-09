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

checkcrash() {

	local exebase="$1"
	shift

	local out="$1"
	shift

	local error="$1"
	shift

	local f="$1"
	shift

	local type="$1"
	shift

	local test="$1"
	shift

	if [ "$error" -gt 127 ]; then

		echo "\n$exebase crashed on $type:\n"
		echo "    $f"

		echo "\nCopying to \"$out\""
		cp "$f" "$out"

		echo "\nexiting..."
		exit "$error"

	fi
}

script="$0"

testdir=$(dirname "$script")

if [ "$#" -lt 1 ]; then
	echo "usage: $script dir [exe results_dir]"
	exit 1
else
	exedir="$1"
	shift
fi

if [ "$#" -lt 1 ]; then
	exe="$testdir/../$exedir"
else
	exe="$1"
	shift
fi

exebase=$(basename "$exe")

if [ "$exebase" = "bc" ]; then
	halt="halt"
	options="-lq"
else
	halt="q"
	options="-x"
fi

if [ "$#" -lt 1 ]; then
	resultsdir="$testdir/../../results"
else
	resultsdir="$1"
	shift
fi

out="$testdir/../.test.txt"

for d in $resultsdir/*; do

	echo "$d"

	for f in $d/crashes/*; do

		echo "    $f"

		base=$(basename "$f")

		[ -e "$f" ] || continue
		[ "$base" != "README.txt" ] || continue

		while read line; do

			echo "$line" | "$exe" "$@" "$options" > /dev/null 2>&1
			error="$?"

			checkcrash "$exebase" "$out" "$error" "$f" "test" "$line"

		done < "$f"

		echo "    Running file through cat..."

		cat "$f" | "$exe" "$@" "$options" > /dev/null 2>&1
		error="$?"

		checkcrash "$exebase" "$out" "$error" "$f" "running $f through cat" "$f"

		echo "    Running file through carrot input..."

		"$exe" "$@" "$options" < "$f" > /dev/null 2>&1
		error="$?"

		checkcrash "$exebase" "$out" "$error" "$f" "running $f through carrot input" "$f"

		echo "    Running whole file..."

		echo "$halt" | "$exe" "$@" "$options" "$f" > /dev/null 2>&1
		error="$?"

		checkcrash "$exebase" "$out" "$error" "$f" "file" "$f"

	done

done
