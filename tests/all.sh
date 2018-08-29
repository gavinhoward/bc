#! /bin/sh

set -e

script="$0"

testdir=$(dirname "$script")

if [ "$#" -lt 1 ]; then
	bc="$testdir/../bc"
else
	bc="$1"
fi

bcdir=$(dirname "${bc}")

out="$bcdir/log_test.txt"

while read t; do

	echo "$t"
	sh "$testdir/test.sh" "$t" "$bc" "$out"

done < "$testdir/all.txt"

sh "$testdir/scripts.sh" "$bc" "$bcdir/log_bc.txt" "$out"

# TODO: Read tests
# TODO: Lex errors
# TODO: Parse errors
# TODO: VM errors
# TODO: Math errors
# TODO: POSIX warnings
# TODO: POSIX errors
