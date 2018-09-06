#! /bin/bash

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

ops=( '+' '-' '*' '/' '%' '^' )
files=( "add" "subtract" "multiply" "divide" "modulus" "power" "sqrt" "e_power"
        "log" "arctangent" "sine" "cosine" "bessel" )
funcs=( "sqrt" "e" "l" "a" "s" "c" "j" )

script="$0"

testdir=$(dirname "$script")

if [ "$#" -lt 1 ]; then
	echo "usage: $0 num_tests [bc test_output1 test_output2]"
	exit 1
fi

ntests="$1"
shift

if [ "$#" -gt 0 ]; then
	bc="$1"
	shift
else
	bc="$testdir/../bc"
fi

if [ "$#" -gt 0 ]; then
	out1="$1"
	shift
else
	out1="$testdir/../log_bc.txt"
fi

if [ "$#" -gt 0 ]; then
	out2="$1"
	shift
else
	out2="$testdir/../log_test.txt"
fi

for t in $(seq "$ntests"); do

	rm -rf "$out1"
	rm -rf "$out2"

	line=""

	operator=$(gen 1)

	op=$(expr "$operator" % 13)

	if [ "$op" -lt 6 ]; then

		line="$(num 1 1 1) ${ops[$op]}"

		if [ "$op" -eq 3 ]; then
			number=$(num 1 1 0)
		elif [ "$op" -eq 5 ]; then
			number=$(num 1 0 1 1)
		else
			number=$(num 1 1 1)
		fi

		line="$line $number"

	else

		if [ "$op" -eq 6 ]; then
			number=$(num 0 1 1)
		elif [ "$op" -eq 12 ]; then
			number=$(num 1 1 1 1)
		else
			number=$(num 1 1 1)
		fi

		func=$(expr "$op" - 6)
		line="${funcs[$func]}($number"

		if [ "$op" -ne 12 ]; then
			line="$line)"
		else
			n=$(num 1 1 1)
			line="$line, $n)"
		fi
	fi

	echo "Test $t: $line"

	echo "$line; halt" | bc -lq > "$out1"
	echo "$line; halt" | "$bc" -lq > "$out2"

	error="$?"

	if [ "$error" -ne 0 ]; then
		echo "    bc returned an error ($error); exiting..."
		exit "$error"
	fi

	diff "$out1" "$out2"

	error="$?"

	if [ "$error" -ne 0 ]; then
		echo "    failed; adding \"$line\" to test suite..."
		echo "$line" >> "$testdir/${files[$op]}.txt"
		cat "$out1" >> "$testdir/${files[$op]}_results.txt"
	fi

done

