./tests/add $1 $2 $3 $4 > log1

echo "$1 + $2" | bc -l > log2

diff log1 log2
