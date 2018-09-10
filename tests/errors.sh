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

errors="$testdir/errors.txt"
posix_errors="$testdir/posix_errors.txt"

for testfile in "$errors" "$posix_errors"; do

	if [ "$testfile" = "$posix_errors" ]; then
		options="-ls"
	else
		options="-l"
	fi

	base=$(basename "$testfile")
	base="${base%.*}"
	echo "\nRunning $base..."

	while read -r line; do

		echo "    Test: $line"

		echo "$line" | "$bc" "$@" "$options" 2>/dev/null
		error="$?"

		if [ "$error" -eq 0 ]; then
			echo "\nbc returned no error"
			echo "exiting..."
			exit 127
		fi

	done < "$testfile"

done
