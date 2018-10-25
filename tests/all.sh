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

set -e

script="$0"

testdir=$(dirname "$script")

if [ "$#" -ge 1 ]; then
	d="$1"
	shift
else
	echo "usage: dir [exec args...]"
	exit 1
fi

if [ "$#" -lt 1 ]; then
	exe="$testdir/../bin/$d"
else
	exe="$1"
	shift
fi

if [ "$d" = "bc" ]; then

	halt="quit"

	echo -e "\nRunning $d limits tests...\n"
	echo "limits" | "$exe"

else
	halt="q"
fi

echo -e "\nRunning $d tests...\n"

while read t; do
	sh "$testdir/test.sh" "$d" "$t" "$exe" "$@"
done < "$testdir/$d/all.txt"

sh "$testdir/scripts.sh" "$d" "$exe" "$@"
sh "$testdir/errors.sh" "$d" "$exe" "$@"

echo -e "\nRunning quit test...\n"

if [ "$d" = "bc" ]; then
	halt="quit"
else
	halt="q"
fi

echo "$halt" | "$exe"

echo -e "\nRunning arg tests...\n"

f="$testdir/$d/add.txt"
exprs=$(cat "$f")
results=$(cat "$testdir/$d/add_results.txt")

out1="$testdir/../.log_bc.txt"
out2="$testdir/../.log_test.txt"

echo -e "$results\n$results\n$results\n$results" > "$out1"

"$exe" -e "$exprs" -f "$f" --expression "$exprs" --file "$f" > "$out2"

diff "$out1" "$out2"
err="$?"

if [ "$d" = "bc" ]; then
	echo "$halt" | "$exe" -i
fi
echo "$halt" | "$exe" -h > /dev/null
echo "$halt" | "$exe" -v > /dev/null
echo "$halt" | "$exe" -V > /dev/null

set +e

"$exe" -u -e "$exprs"
err="$?"

if [ "$err" -eq 0 ]; then
	echo "$d did not return an error ($err) on invalid argument test"
	echo "exiting..."
	exit 1
fi

echo -e "\nRunning directory test...\n"

"$exe" "$testdir"
err="$?"

if [ "$err" -eq 0 ]; then
	echo "$d did not return an error ($err) on directory test"
	echo "exiting..."
	exit 1
fi

echo -e "\nRunning binary file test...\n"

bin=$(which cat)

"$exe" "$bin"
err="$?"

if [ "$err" -eq 0 ]; then
	echo "$d did not return an error ($err) on binary file test"
	echo "exiting..."
	exit 1
fi

echo -e "\nRunning binary stdin test...\n"

cat "$bin" | "$exe"
err="$?"

if [ "$err" -eq 0 ]; then
	echo "$d did not return an error ($err) on binary stdin test"
	echo "exiting..."
	exit 1
fi
