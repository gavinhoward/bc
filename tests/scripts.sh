#! /bin/sh

set -e

script="$0"

testdir=$(dirname "${script}")
scriptdir="$testdir/scripts"

if [ "$#" -gt 0 ]; then
	bc="$1"
	shift
else
	bc="$testdir/../bc"
fi

out1="$testdir/../.log_bc.txt"
out2="$testdir/../.log_test.txt"

for s in $scriptdir/*.bc; do

	f=$(basename -- "$s")
	name="${f%.*}"

	if [ "$f" = "timeconst.bc" ]; then
		continue
	fi

	echo "Running script: $f"

	orig="$testdir/$name.txt"
	results="$scriptdir/$name.txt"

	if [ -f "$orig" ]; then
		res="$orig"
	elif [ -f "$results" ]; then
		res="$results"
	else
		echo "halt" | bc -lq "$s" > "$out1"
		res="$out1"
	fi

	echo "halt" | "$bc" "$@" -lq "$s" > "$out2"

	diff "$res" "$out2"

done

rm -rf "$out1" "$out2"
