#! /bin/sh

# WARNING: Test files cannot have empty lines!

checktest()
{
	local error="$1"
	shift

	local name="$1"
	shift

	local out="$1"
	shift

	local bcbase="$1"
	shift

	if [ "$error" -eq 0 ]; then
		echo "\nbc returned no error on test:\n"
		echo "    $name"
		echo "\nexiting..."
		exit 127
	fi

	if [ "$error" -gt 127 ]; then
		echo "\nbc crashed on test:\n"
		echo "    $name"
		echo "\nexiting..."
		exit "$error"
	fi

	if [ ! -s "$out" ]; then
		echo "\nbc produced no error message on test:\n"
		echo "    $name"
		echo "\nexiting..."
		exit "$error"
	fi

	# Display the error messages if not directly running bc.
	# This allows the script to print valgrind output.
	if [ "$bcbase" != "bc" ]; then
		cat "$out"
	fi
}

script="$0"

testdir=$(dirname "$script")
testfile="$testdir/errors.txt"

if [ "$#" -lt 1 ]; then
	bc="$testdir/../bc"
else
	bc="$1"
	shift
fi

out="$testdir/../.log_test.txt"

errors="$testdir/errors.txt"
posix_errors="$testdir/posix_errors.txt"

bcbase=$(basename "$bc")

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

		rm -f "$out"

		echo "$line" | "$bc" "$@" "$options" 2> "$out" > /dev/null

		checktest "$?" "$line" "$out" "$bcbase"

	done < "$testfile"

done

for testfile in $testdir/errors/*.txt; do

	echo "Running file \"$testfile\" through cat..."

	cat "$testfile" | "$bc" "$@" -l 2> "$out" > /dev/null

	checktest "$?" "$testfile" "$out" "$bcbase"

	echo "Running file \"$testfile\" as a file..."

	echo "halt" | "$bc" "$@" -l "$testfile" 2> "$out" > /dev/null

	checktest "$?" "$testfile" "$out" "$bcbase"

done
