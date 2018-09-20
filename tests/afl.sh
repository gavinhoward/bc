#! /bin/sh

script="$0"

testdir=$(dirname "$script")

if [ "$#" -lt 1 ]; then
	bc="$testdir/../bc"
else
	bc="$1"
	shift
fi

if [ "$#" -lt 1 ]; then
	resultsdir="$testdir/../../results"
else
	resultsdir="$1"
	shift
fi

for d in $resultsdir/*; do

	echo "$d"

	for f in $d/crashes/*; do

		echo "    $f"

		base=$(basename "$f")

		[ -e "$f" ] || continue
		[ "$base" != "README.txt" ] || continue

		while read line; do

			echo "$line" | "$bc" "$@" -l > /dev/null 2>&1
			error="$?"

			if [ "$error" -gt 127 ]; then

				echo "\nbc crashed on test:\n"
				echo "    $line"

				echo "\nCopying to \"$testdir/../../test.txt\""
				cp "$f" "$testdir/../../test.txt"

				echo "\nexiting..."
				exit "$error"

			fi

		done < "$f"

		echo "    Running whole file..."

		echo "halt" | "$bc" "$@" -l "$f" > /dev/null 2>&1
		error="$?"

		if [ "$error" -gt 127 ]; then

			echo "\nbc crashed on file:\n"
			echo "    $f"

			echo "\nCopying to \"$testdir/../../test.txt\""
			cp "$f" "$testdir/../../test.txt"

			echo "\nexiting..."
			exit "$error"

		fi

	done

done
