#! /bin/bash

script="$0"

testdir=$(dirname "${script}")

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

for s in $testdir/scripts/*.bc; do

	echo "Running script: $s"

	rm -rf "$out1"
	rm -rf "$out2"

	echo "halt" | bc -lq "$s" > "$out1"
	echo "halt" | "$bc" -lq "$s" > "$out2"

	diff "$out1" "$out2"

	ret="$?"

	if [ "$ret" -ne 0 ]; then

		command -v meld >/dev/null 2>&1

		ret2="$?"

		if [ "$ret2" -eq 0 ]; then
			meld "$out1" "$out2"
		fi

		read -p "Continue? " reply
		if [[ $reply =~ ^[Yy]$ ]]; then
			continue
		else
			exit "$ret"
		fi

	fi

done

# TODO: Read tests
# TODO: Lex errors
# TODO: Parse errors
# TODO: VM errors
# TODO: Math errors
# TODO: POSIX warnings
# TODO: POSIX errors
