dd if=/dev/urandom of=TESTSMALL count=10000
dd if=/dev/urandom of=TESTMID count=100000
#dd if=/dev/urandom of=TESTLARGE count=1000000



./dd if=grep.c conv=unblock cbs=22 > test1
dd if=grep.c conv=unblock cbs=22 > test2
diff test1 test2


./dd if=TESTSMALL conv=unblock ibs=51 cbs=8 obs=3441 > test1
dd if=TESTSMALL conv=unblock ibs=51 obs=3441  cbs=8 > test2
diff test1 test2 

./dd if=TESTSMALL conv=unblock bs=22323 cbs=40 > test1
dd if=TESTSMALL conv=unblock bs=22323 cbs=40 > test2
diff test1 test2

./dd if=TESTSMALL conv=unblock bs=22323234 cbs=80 > test1
dd if=TESTSMALL conv=unblock bs=22323234 cbs=80 > test2
diff test1 test2

./dd if=TESTSMALL conv=unblock cbs=22 > test1
dd if=TESTSMALL conv=unblock cbs=22 > test2
diff test1 test2


./dd if=TESTMID conv=unblock cbs=22 > test1
dd if=TESTMID conv=unblock cbs=22> test2
diff test1 test2

exit

./dd if=TESTMID conv=unblock cbs=40 > test1
dd if=TESTMID conv=unblock cbs=40 > test2
diff test1 test2

./dd if=TESTMID conv=unblock cbs=400 > test1
dd if=TESTMID conv=unblock cbs=400 > test2
diff test1 test2

./dd if=TESTMID conv=unblock cbs=401 > test1
dd if=TESTMID conv=unblock cbs=401 > test2
diff test1 test2

./dd if=TESTMID conv=unblock cbs=6254 > test1
dd if=TESTMID conv=unblock cbs=6254  > test2
diff test1 test2

./dd if=TESTMID conv=unblock cbs=6255 > test1
dd if=TESTMID conv=unblock cbs=6255  > test2
diff test1 test2

./dd if=TESTMID conv=unblock cbs=6256 > test1

dd if=TESTMID conv=unblock cbs=6256  > test2
diff test1 test2


#./dd if=TESTLARGE conv=unblock cbs=2 > test1
#dd if=TESTLARGE conv=unblock cbs=2> test2
#diff test1 test2


#./dd if=TESTLARGE conv=unblock cbs=40 > test1
#dd if=TESTLARGE conv=unblock cbs=40 > test2
#diff test1 test2

#./dd if=TESTLARGE conv=unblock cbs=401 > test1
#dd if=TESTLARGE conv=unblock cbs=401 > test2
#diff test1 test2



