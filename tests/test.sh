#! /bin/sh

script="$0"

testdir=$(dirname "$script")

if [ "$#" -lt 1 ]; then
	echo "usage: $0 test [bc test_output]"
	echo "valid tests are:"
	echo ""
	cat "$testdir/all.txt"
	echo ""
	exit 1
fi

set -e

t="$1"
name="$testdir/$t.txt"
results="$testdir/${t}_results.txt"
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
	out1="$testdir/../log_test.txt"
fi

if [ "$t" = "parse" -o "$t" = "print" -o "$t" = "arctangent" ]; then

	f="$testdir/$t.txt"

	if [ ! -f "$f" ]; then
		echo "Generating $t..."
		"$testdir/scripts/$t.bc" > "$f"
	fi

	echo "Running $t..."

fi

echo "halt" | "$bc" -lq "$name" > "$out1"

diff "$results" "$out1"

