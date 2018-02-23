#! /bin/bash

gen()
{
	result=`dd if=/dev/urandom bs=4 count=1 2>/dev/null | od -t u4 | awk 'NR==1 {print $2}'`
	echo -n "$result"
}

neg()
{
	result=$(gen)
	result="$((result & 1))"
	echo -n "$result"
}

zero()
{
	result=$(gen)
	result="$((result & 15))"
	echo -n $(expr "$result" = 15)
}

num()
{
	num=""

	neg=$1

	real=$2

	zero=$3

	if [ "$neg" -ne 0 ]; then

		neg=$(neg)

		if [ "$neg" -ne 0 ]; then
			num="-"
		fi
	fi

	if [ "$zero" -eq 0 ]; then

		z=$(zero)

		if [ "$z" -ne 0 ]; then
			num="${num}0"
		fi
	else
		num="$num`gen`"
	fi

	if [ "$real" -ne 0 ]; then

		if [ "$zero" -eq 0 ]; then

			z=$(zero)

			if [ "$z" -ne 0 ]; then
				num="$num."
				num="$num`gen`"
			fi
		fi
	fi

	echo -n "$num"
}

ops=( '+' '-' '*' '/' '%' '^' )
files=( "add" "subtract" "multiply" "divide" "modulus" "power" "sqrt" )

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

t=0

while [ "$t" -lt "$ntests" ]; do

	rm -rf "$out1"
	rm -rf "$out2"

	line=""

	operator=$(gen)

	op=$(expr "$operator" % 7)

	if [ "$op" -ne 7 ]; then

		line=$(num 1 1 1)

		line="$line ${ops[$op]}"

		if [ "$op" -eq 3 ]; then
			line="$line `num 1 1 0`"
		elif [ "$op" -eq 5 ]; then
			line="$line `num 1 0 1`"
		else
			line="$line `num 1 1 1`"
		fi


	else
		line="sqrt(`num 0 1 1`)"
	fi

	echo "$line"

	echo "$line" | bc -lq > "$out1"
	echo "$line" | "$bc" -lq > "$out2"

	error="$?"

	if [ "$error" -ne 0 ]; then
		exit "$error"
	fi

	diff "$out1" "$out2"

	error="$?"

	if [ "$error" -ne 0 ]; then
		echo "$line" >> "${files[$op]}.txt"
		exit "$error"
	fi

	t=$(expr "$t" + "1")

done

