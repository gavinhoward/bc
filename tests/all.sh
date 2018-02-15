#! /bin/sh

if [ "$#" -lt 1 ]; then
	bc="../bc"
	testdir="."
else

	bc="$1"

	if [ "$#" -lt 2]; then
		testdir="."
	else
		testdir="$2"
	fi
fi

set -e

bcdir=$(dirname "${bc}")

out1="$bcdir/log_bc.txt"
out2="$bcdir/log_test.txt"

while read t; do

	"$testdir/test.sh" "$t" "$bc" "$out1" "$out2"

done < "$testdir/all.txt"

"$testdir/scripts.sh" "$bc" "$out1" "$out2"

set +e

# TODO: Read tests
# TODO: Lex errors
# TODO: Parse errors
# TODO: VM errors
# TODO: Math errors
# TODO: POSIX warnings
# TODO: POSIX errors
