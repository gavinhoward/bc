#! /bin/sh

if [ "$#" -lt 4 ]; then
	echo "usage: test.sh <test> <bc> <test_output1> <test_output2>"
	exit 1
fi

script="$0"

testdir=$(dirname "$script")

set -e

name="$testdir/$1"
shift

bc="$1"
shift

out1="$1"
shift

out2="$1"
shift

rm -rf "$out1"
rm -rf "$out2"

cat "$name.txt" | bc -lq >> "$out1"

cat "$name.txt" | "$bc" -lq >> "$out2"

diff "$out1" "$out2"
