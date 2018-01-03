dd if=/dev/urandom of=TESTSMALL count=10000
dd if=/dev/urandom of=TESTMID count=100000
#dd if=/dev/urandom of=TESTLARGE count=1000000



./dd if=grep.c conv=ucase cbs=22 > test1
dd if=grep.c conv=ucase cbs=22 > test2
diff test1 test2


./dd if=TESTSMALL conv=ucase ibs=51 cbs=8 obs=3441 > test1
dd if=TESTSMALL conv=ucase ibs=51 obs=3441  cbs=8 > test2
diff test1 test2

./dd if=TESTSMALL conv=ucase bs=22323 cbs=40 > test1
dd if=TESTSMALL conv=ucase bs=22323 cbs=40 > test2
diff test1 test2

./dd if=TESTSMALL conv=ucase bs=22323234 cbs=80 > test1
dd if=TESTSMALL conv=ucase bs=22323234 cbs=80 > test2
diff test1 test2

./dd if=TESTSMALL conv=ucase cbs=22 > test1
dd if=TESTSMALL conv=ucase cbs=22 > test2
diff test1 test2


./dd if=TESTMID conv=ucase cbs=22 > test1
dd if=TESTMID conv=ucase cbs=22> test2
diff test1 test2

