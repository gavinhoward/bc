#! /bin/sh

# Tests the timeconst.bc script from the Linux kernel build.
# You can find the script at kernel/time/timeconst.bc in any Linux repo.
# One such repo is: https://github.com/torvalds/linux

# Note: when testing this bc in toybox, make sure the link is in the same
# directory as the toybox binary, or this script will not work.

set -e

script="$0"
testdir=$(dirname "$script")

if [ "$#" -gt 0 ]; then
	timeconst="$1"
	shift
else
	timeconst="$testdir/../../timeconst.bc"
fi

if [ "$#" -gt 0 ]; then
	bc="$1"
	shift
else
	bc="$testdir/../bc"
fi

out1="$testdir/../.log_bc.txt"
out2="$testdir/../.log_test.txt"

for i in $(seq 0 10000); do

	echo "$i"

	(echo $i | bc -q "$timeconst") > "$out1"
	(echo $i | "$bc" "$@" -q "$timeconst") > "$out2"

	diff "$out1" "$out2"

done

rm -rf "$out1" "$out2"
