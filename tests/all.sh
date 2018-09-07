#! /bin/sh

set -e

script="$0"

testdir=$(dirname "$script")

if [ "$#" -lt 1 ]; then
	bc="$testdir/../bc"
else
	bc="$1"
fi

while read t; do

	sh "$testdir/test.sh" "$t" "$bc" "$@"

done < "$testdir/all.txt"

sh "$testdir/scripts.sh" "$bc" "$@"

# TODO: Read tests
# TODO: Lex errors
# TODO: Parse errors
# TODO: VM errors
# TODO: Math errors
# TODO: POSIX warnings
# TODO: POSIX errors
