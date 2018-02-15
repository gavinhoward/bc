#! /bin/sh

s="$0"

testdir=$(dirname "$s")

if [ "$#" -lt 1 ]; then
	echo "usage: script.sh <script> [bc]"
	exit 1
fi

script="$1"
shift

if [ "$#" -lt 1 ]; then
	bc="$testdir/../bc"
else
	bc="$1"
fi

set -e

bcdir=$(dirname "${bc}")

out1="$bcdir/log_bc.txt"
out2="$bcdir/log_test.txt"

"$testdir/scripts/script.sh" "$bc" "$out1" "$out2" "$script"
