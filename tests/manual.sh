#! /bin/sh

if [ "$#" -lt 1 ]; then
	echo "usage: manual.sh <bc> [exprs...]"
	exit 1
fi

set -e

bc="$1"
shift

bcdir=$(dirname "${bc}")

out1="$bcdir/log_bc.txt"
out2="$bcdir/log_test.txt"

rm -rf "$out1"
rm -rf "$out2"

for string in "$@"; do

	echo "$string" | bc -q >> "$out1"
	echo "$string" | "$bc" -q >> "$out2"

done

diff "$out1" "$out2"
