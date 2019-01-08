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

if [ "$#" -ge 1 ]; then
	d="$1"
	shift
else
	printf 'usage: %s dir [run_extended_tests] [run_reference_tests] [generate_tests] [exec args...]\n' "$script"
	exit 1
fi

if [ "$#" -lt 1 ]; then
	extra=1
else
	extra="$1"
	shift
fi

if [ "$#" -lt 1 ]; then
	refs=1
else
	refs="$1"
	shift
fi

if [ "$#" -lt 1 ]; then
	generate_tests=1
else
	generate_tests="$1"
	shift
fi

if [ "$#" -lt 1 ]; then
	exe="$testdir/../bin/$d"
else
	exe="$1"
	shift
fi

stars="***********************************************************************"
printf '%s\n' "$stars"

if [ "$d" = "bc" ]; then
	halt="quit"
else
	halt="q"
fi

printf '\nRunning %s tests...\n\n' "$d"

while read t; do

	if [ "$extra" -eq 0  ]; then
		if [ "$t" = "trunc" -o "$t" = "places" -o "$t" = "shift" -o "$t" = "lib2" ]; then
			printf 'Skipping %s %s\n' "$d" "$t"
			continue
		fi
	fi

	sh "$testdir/test.sh" "$d" "$t" "$generate_tests" "$exe" "$@"

done < "$testdir/$d/all.txt"

sh "$testdir/stdin.sh" "$d" "$exe" "$@"

sh "$testdir/scripts.sh" "$d" "$refs" "$generate_tests" "$exe" "$@"
sh "$testdir/errors.sh" "$d" "$exe" "$@"

printf '\nRunning quit test...\n'

if [ "$d" = "bc" ]; then
	halt="quit"
	opt="x"
	lopt="extended-register"
else
	halt="q"
	opt="l"
	lopt="mathlib"
fi

printf '%s\n' "$halt" | "$exe" "$@" > /dev/null 2>&1

base=$(basename "$exe")

if [ "$base" != "bc" -a "$base" != "dc" ]; then
	exit 0
fi

printf 'Running arg tests...\n'

f="$testdir/$d/add.txt"
exprs=$(cat "$f")
results=$(cat "$testdir/$d/add_results.txt")

out1="$testdir/../.log_bc.txt"
out2="$testdir/../.log_test.txt"

printf '%s\n%s\n%s\n%s\n' "$results" "$results" "$results" "$results" > "$out1"

"$exe" "$@" -e "$exprs" -f "$f" --expression "$exprs" --file "$f" > "$out2"

diff "$out1" "$out2"
err="$?"

if [ "$d" = "bc" ]; then
	printf '%s\n' "$halt" | "$exe" "$@" -i > /dev/null 2>&1
fi

printf '%s\n' "$halt" | "$exe" "$@" -h > /dev/null
printf '%s\n' "$halt" | "$exe" "$@" -v > /dev/null
printf '%s\n' "$halt" | "$exe" "$@" -V > /dev/null

set +e

"$exe" "$@" "-$opt" -e "$exprs" > /dev/null 2>&1
err="$?"

if [ "$err" -eq 0 ]; then
	printf '%s did not return an error (%d) on invalid option argument test\n' "$d" "$err"
	printf 'exiting...\n'
	exit 1
fi

"$exe" "$@" "--$lopt" -e "$exprs" > /dev/null 2>&1
err="$?"

if [ "$err" -eq 0 ]; then
	printf '%s did not return an error (%d) on invalid long option argument test\n' "$d" "$err"
	printf 'exiting...\n'
	exit 1
fi

printf 'Running directory test...\n'

"$exe" "$@" "$testdir" > /dev/null 2>&1
err="$?"

if [ "$err" -eq 0 ]; then
	printf '%s did not return an error (%d) on directory test\n' "$d" "$err"
	printf 'exiting...\n'
	exit 1
fi

printf 'Running binary file test...\n'

bin="/bin/sh"

"$exe" "$@" "$bin" > /dev/null 2>&1
err="$?"

if [ "$err" -eq 0 ]; then
	printf '%s did not return an error (%d) on binary file test\n' "$d" "$err"
	printf 'exiting...\n'
	exit 1
fi

printf 'Running binary stdin test...\n'

cat "$bin" | "$exe" "$@" > /dev/null 2>&1
err="$?"

if [ "$err" -eq 0 ]; then
	printf '%s did not return an error (%d) on binary stdin test\n' "$d" "$err"
	printf 'exiting...\n'
	exit 1
fi

if [ "$d" = "bc" ]; then

	printf 'Running limits tests...\n'
	printf 'limits\n' | "$exe" "$@" > "$out2" /dev/null 2>&1

	if [ ! -s "$out2" ]; then
		printf '%s did not produce output on the limits test\n' "$d"
		printf 'exiting...\n'
		exit 1
	fi

fi

printf '\n%s\n' "$stars"
