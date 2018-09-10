#! /bin/sh

# WARNING: Test files cannot have empty lines!

script="$0"

testdir=$(dirname "$script")
testfile="$testdir/errors.txt"

if [ "$#" -lt 1 ]; then
	bc="$testdir/../bc"
else
	bc="$1"
fi

echo "Running errors..."

while read -r line; do

	echo "    Test: $line"

	echo "$line" | "$bc" "$@" -l 2>/dev/null
	error="$?"

	if [ "$error" -eq 0 ]; then
		echo "\nbc returned no error"
		echo "exiting..."
		exit 127
	fi

done < "$testfile"
