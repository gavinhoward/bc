#! /bin/sh

# WARNING: Test files cannot have empty lines!

script="$0"

testdir=$(dirname "$script")
testfile="$testdir/errors.txt"

if [ "$#" -lt 1 ]; then
	bc="$testdir/../bc"
else
	bc="$1"
	shift
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
	echo "Running $base..."

	while read -r line; do

		echo "$line" | "$bc" "$@" "$options" > /dev/null
		error="$?"

		if [ "$error" -eq 0 ]; then
			echo "\nbc returned no error on test:\n"
			echo "    $line"
			echo "\nexiting..."
			exit 127
		fi

	done < "$testfile"

done
