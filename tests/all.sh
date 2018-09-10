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
sh "$testdir/errors.sh" "$bc" "$@"
