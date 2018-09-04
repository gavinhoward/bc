#! /bin/sh

set -e

script="$0"

testdir=$(dirname "${script}")
scriptdir="$testdir/scripts"

if [ "$#" -lt 3 ]; then
	echo "usage: $script <bc> <test_output1> <test_output2>"
	exit 1
fi

bc="$1"
shift

out1="$1"
shift

out2="$1"
shift

for s in $scriptdir/*.bc; do

	f=$(basename -- "$s")
	name="${f%.*}"

	echo "Running script: $f"

	rm -rf "$out1"
	rm -rf "$out2"

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

	echo "halt" | "$bc" -lq "$s" > "$out2"

	diff "$res" "$out2"

done

# TODO: Read tests
# TODO: Lex errors
# TODO: Parse errors
# TODO: VM errors
# TODO: Math errors
# TODO: POSIX warnings
# TODO: POSIX errors
