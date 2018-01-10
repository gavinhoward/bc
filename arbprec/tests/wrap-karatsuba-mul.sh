./tests/karatsuba-mul $1 $2 $3 $4 > log1

echo "scale=1000; $1 * $2" | bc -l > log2

diff log1 log2

if [ "$?" != "0" ]; then
    echo "$1 * $2 failed"
    exit 1
fi

