#!/bin/sh

#make all 

# test one
printf "%s\n"
printf "%s\n" "populating with random data .. "
printf "%s\n"
dd if=/dev/urandom of=testfile1 bs=1M count=10 2>/dev/null
printf "%s\n" 
./cat testfile1 > testfile2 
diff testfile1 testfile2 && printf "%s\n" "$0 first test successful"
rm testfile1 testfile2 


# test 2
printf "%s\n"
printf "%s\n" "populating with random data .. "
printf "%s\n"
dd if=/dev/urandom of=testfile1 bs=1M count=100 2>/dev/null
printf "%s\n" 
./cat testfile1 > testfile2 
diff testfile1 testfile2 && printf "%s\n" "$0 second test successful"
rm testfile1 testfile2 


# test 3
printf "%s\n"
printf "%s\n" "populating with random data .. "
printf "%s\n"
dd if=/dev/urandom of=testfile1 bs=1M count=1000 2>/dev/null
printf "%s\n" 
./cat testfile1 > testfile2 
diff testfile1 testfile2 && printf "%s\n" "$0 third test successful"
rm testfile1 testfile2





