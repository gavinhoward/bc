#!/bin/sh


TESTDIR="a/b/c/123/d/f/123/f/4/s/4/gdfg/sd/sdfsdf/fvbbnnndfgbdgfg/asd"

./mkdir -p  "$TESTDIR"

[ -d "$TESTDIR" ] && printf "%s\n" "$0 test one successful"

rm -rf a



TESTDIR="a/b/c/123/d/f/123/f/4/s/4/gdfg/sd/sdfsdf/fvbbnnndfgbdgfg/asd/asdasd/asdasdasda/123d234234/asdf3r2r23r423r23er2323rr23r23r/"

./mkdir -p  "$TESTDIR"

[ -d "$TESTDIR" ] && printf "%s\n" "$0 test two successful"

rm -rf a



