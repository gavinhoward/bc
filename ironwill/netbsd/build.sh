#!/bin/sh



MIRROR="ftp://ftp.netbsd.org/pub/NetBSD/NetBSD-7.1/source/sets/"

#wget ${MIRROR}xsrc.tgz
#wget ${MIRROR}gnusrc.tgz
#wget ${MIRROR}sharesrc.tgz
#wget ${MIRROR}src.tgz
#wget ${MIRROR}syssrc.tgz

#gunzip *.tgz

for i in *.tar ; do tar -xf $i ; done

cd usr/src 

./build.sh -m i386 -U -O ../obj/ tools >logfile 2>&1
