#! /bin/bash

# This script uses a technique to use a keypress to get
# out of an infinite loop. The technique was found here:
# https://stackoverflow.com/questions/5297638/bash-how-to-end-infinite-loop-with-any-key-pressed

finish() {
	rm -rf "$out1" "$out2" "$math" "$results" "$opfile"
}
trap finish EXIT

gen()
{
	limit="$1"
	shift

	result=$(dd if=/dev/urandom bs="$limit" count=1 2>/dev/null | od -t u4 | awk 'NR==1 {print $2}')
	echo -n "$result"
}

neg()
{
	result=$(gen 1)
	result="$((result & 1))"
	echo -n "$result"
}

zero()
{
	result=$(gen 1)
	echo -n "$result"
}

limit()
{
	max="$1"
	shift

	result=$(gen 1)
	result=$(expr "$result" % "$max")
	echo -n $(expr "$result" + 1)
}

num()
{
	n=""

	neg=$1
	shift

	real=$1
	shift

	zero=$1
	shift

	if [ "$#" -gt 0 ]; then
		limit="$1"
		shift
	else
		limit="$(limit 4)"
	fi

	if [ "$zero" -ne 0 ]; then
		z=$(zero)
	else
		z=1
	fi

	if [ "$z" -eq 0 ]; then
		n="0"
	else

		if [ "$neg" -ne 0 ]; then

			neg=$(neg)

			if [ "$neg" -eq 0 ]; then
				n="-"
			fi
		fi

		g=$(gen $limit)
		n="${n}${g}"

		if [ "$real" -ne 0 ]; then

			z=$(neg)

			if [ "$z" -ne 0 ]; then

				limit=$(limit 25)
				g=$(gen $limit)
				n="$n.$g"
			fi
		fi
	fi

	echo -n "$n"
}

append_test() {
	echo "$line" >> "$math"
	cat "$out1" >> "$results"
	echo "$op" >> "$opfile"
}

ops=( '+' '-' '*' '/' '%' '^' )
files=( "add" "subtract" "multiply" "divide" "modulus" "power" "sqrt" "exponent"
        "log" "arctangent" "sine" "cosine" "bessel" )
funcs=( "sqrt" "e" "l" "a" "s" "c" "j" )

script="$0"
testdir=$(dirname "$script")

if [ "$#" -gt 0 ]; then
	bc="$1"
	shift
else
	bc="$testdir/../bc"
fi

out1="$testdir/../.log_bc.txt"
out2="$testdir/../.log_test.txt"

# Files to output failed tests to.
math="$testdir/../.math.txt"
results="$testdir/../.results.txt"
opfile="$testdir/../.ops.txt"

rm -rf "$math" "$results" "$opfile"

# Set it so we can exit the loop on keypress.
if [ -t 0 ]; then
	stty -echo -icanon -icrnl time 0 min 0
fi

it=0
keypress=''

while [ "x$keypress" = "x" ]; do

	t="$it"
	it=$(expr "$it" + "1")

	line=""
	operator=$(gen 1)
	op=$(expr "$operator" % 13)

	if [ "$op" -lt 6 ]; then

		line="$(num 1 1 1) ${ops[$op]}"

		if [ "$op" -eq 3 -o "$op" -eq 4 ]; then

			number=$(num 1 1 0)

			scale=$(num 0 0 1 1)
			scale=$(echo "s = $scale % 25; s /= 1; s" | bc)

			line="scale = $scale; $line"

		elif [ "$op" -eq 5 ]; then
			number=$(num 1 0 1 1)
		else
			number=$(num 1 1 1)
		fi

		line="$line $number"

	else

		if [ "$op" -eq 6 ]; then

			number=$(num 0 1 1)

			# GNU bc gets "sqrt(1)" wrong, so skip it.
			if [ "$number" == "1" ]; then
				keypress=$(cat -v)
				continue
			fi

		elif [ "$op" -eq 7 -o "$op" -eq 12 ]; then

			number=$(num 1 1 1 1)

			if [ "$op" -eq 12 ]; then
				number=$(echo "n = $number % 100; scale = 8; n /= 1; n" | bc)
			fi

		else
			number=$(num 1 1 1)
		fi

		func=$(expr "$op" - 6)
		line="${funcs[$func]}($number"

		if [ "$op" -ne 12 ]; then
			line="$line)"
		else
			n=$(num 1 1 1)
			n=$(echo "n = $n % 100; scale = 8; n /= 1; n" | bc)
			line="$line, $n)"
		fi
	fi

	echo "Test $t: $line"

	echo "$line; halt" | bc -lq > "$out1"

	content=$(cat "$out1")

	if [ "$content" == "" ]; then
		echo "    other bc returned an error ($error); continuing..."
		keypress=$(cat -v)
		continue
	elif [ "$content" == "-0" ]; then
		echo "0" > "$out1"
	fi

	echo "$line; halt" | "$bc" "$@" -lq > "$out2"

	error="$?"

	if [ "$error" -ne 0 ]; then
		echo "    bc returned an error ($error); adding \"$line\" to checklist..."
		append_test
		keypress=$(cat -v)
		continue
	fi

	diff "$out1" "$out2" > /dev/null
	error="$?"

	if [ "$error" -ne 0 ]; then

		# This works around a bug in GNU bc that gets some
		# transcendental functions slightly wrong.
		if [ "$op" -ge 7 ]; then

			# Have GNU bc calculate to one more decimal place and truncate by 1.
			content=$(echo "scale += 10; $line; halt" | bc -lq)
			content2=${content%??????????}
			echo "$content2" > "$out1"

			# Compare the truncated.
			diff "$out1" "$out2" > /dev/null
			error="$?"

			# GNU bc got it wrong.
			if [ "$error" -eq 0 ]; then
				echo "    failed because of bug in other bc; continuing..."
				keypress=$(cat -v)
				continue
			fi
		fi

		echo "    failed; adding \"$line\" to checklist..."
		append_test
	fi

	keypress=$(cat -v)

done

# Reset the input.
if [ -t 0 ]; then
	stty sane
fi

if [ ! -f "$math" ]; then
	echo -e "\nNo items in checklist."
	echo "Exiting..."
	exit 0
fi

echo -e "\nGoing through the checklist...\n"

paste "$math" "$results" "$opfile" | while read line result curop; do

	echo -e "\n$line"

	echo "$line; halt" | bc -lq > "$out1"
	echo "$line; halt" | "$bc" "$@" -lq > "$out2"

	diff "$out1" "$out2"

	echo -en "\nAdd test to test suite? [y/N] "
	read answer </dev/tty

	if [ "$answer" != "${answer#[Yy]}" ] ;then
		echo Yes
		echo "$line" >> "$testdir/${files[$curop]}.txt"
		cat "$result" >> "$testdir/${files[$curop]}_results.txt"
	else
		echo No
	fi

done
