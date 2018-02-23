#! /bin/sh

script="$0"

testdir=$(dirname "$script")

if [ "$#" -lt 1 ]; then
	echo "usage: $0 test [bc test_output1 test_output2]"
	echo "valid tests are:"
	echo ""
	cat "$testdir/all.txt"
	echo ""
	exit 1
fi

set -e

name="$testdir/$1.txt"
shift

if [ "$#" -gt 0 ]; then
	bc="$1"
	shift
else
	bc="$testdir/../bc"
fi

if [ "$#" -gt 0 ]; then
	out1="$1"
	shift
else
	out1="$testdir/../log_bc.txt"
fi

if [ "$#" -gt 0 ]; then
	out2="$1"
	shift
else
	out2="$testdir/../log_test.txt"
fi

rm -rf "$out1"
rm -rf "$out2"

cat "$name" | bc -lq >> "$out1"

cat "$name" | "$bc" -lq >> "$out2"

diff "$out1" "$out2"
