#! /bin/sh

# WARNING: Test files cannot have empty lines!

script="$0"

testdir=$(dirname "$script")
errdir="$testdir/errors"

if [ "$#" -lt 1 ]; then
	bc="$testdir/../bc"
else
	bc="$1"
fi

for i in $(seq 4 34); do

	testfile="$errdir/$i.txt"

	echo "Running tests for error code $i"

	while read -r line; do

		echo "    Test: $line"

		echo "$line" | "$bc" "$@" -l 2>/dev/null
		error="$?"

		if [ "$error" -ne "$i" ]; then
			echo "\nbc returned wrong error code: $error"
			echo "    should have been $i"
			echo "exiting..."
			exit 127
		fi

	done < "$testfile"

	echo ""

done
