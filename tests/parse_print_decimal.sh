#! /bin/sh

if [ "$#" -lt 5 ]; then
	echo "usage: test bc out1 out2 base nums..."
	exit 1
fi

set -e

bc="$1"
shift

out1="$1"
shift

out2="$1"
shift

base="$1"
shift

for var in "$@"; do

	string="$var"

	echo "$string" | bc -q >> "$out1"
	echo "$string" | "$bc" -q >> "$out2"

done

diff "$out1" "$out2"
