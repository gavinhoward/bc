#! /bin/sh

out1=../log_bc.txt
out2=../log_test.txt

rm -rf "$out1"
rm -rf "$out2"

./parse_print_decimal.sh ../bc "$out1" "$out2" 1 11 0 00000.000000 120492435 1235.682358692356

