#! /bin/bash
#
# Copyright 2018 Gavin D. Howard
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
# REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
# AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
# INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
# LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
# OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
# PERFORMANCE OF THIS SOFTWARE.
#

usage() {
	echo "usage: $script toybox_repo busybox_repo"
	exit 1
}

header() {

	msg="$1"

	echo ""
	echo "******"
	echo "$msg..."
	echo "******"
	echo ""
}

build() {

	CFLAGS="$1"
	shift

	CC="$1"
	shift

	target="$1"
	shift

	exe="$1"
	shift

	header "Building $target with CC=\"$CC\" and CFLAGS=\"$CFLAGS\""

	CFLAGS="$CFLAGS" CC="$CC" "$exe" "$@" "$target" 2> "$scriptdir/.test.txt"

	if [ -s "$scriptdir/.test.txt" ]; then
		echo "$CC generated warning(s):"
		echo ""
		cat "$scriptdir/.test.txt"
		exit 1
	fi
}

makebuild() {

	CFLAGS="$1"
	shift

	CC="$1"
	shift

	target="$1"
	shift

	build "$CFLAGS" "$CC" "$target" make
}

runtest() {

	header "Running test \"$*\""

	"$@"
}

runtests() {

	CFLAGS="$1"
	shift

	CC="$1"
	shift

	header "Running tests with CFLAGS=\"$CFLAGS\""

	makebuild "$CFLAGS" "$CC" all
	runtest make test_all

	make clean

	makebuild "$CFLAGS" "$CC" bc
	runtest make test_bc_all

	make clean

	makebuild "$CFLAGS" "$CC" dc
	runtest make test_dc

	make clean
}

scan_build() {

	header "Running scan-build"

	build "$debug" "$CC" all scan-build make
	build "$debug" "$CC" bc scan-build make
	build "$debug" "$CC" dc scan-build make

	build "$release" "$CC" all scan-build make
	build "$release" "$CC" bc scan-build make
	build "$release" "$CC" dc scan-build make

	build "$reldebug" "$CC" all scan-build make
	build "$reldebug" "$CC" bc scan-build make
	build "$reldebug" "$CC" dc scan-build make

	build "$minsize" "$CC" all scan-build make
	build "$minsize" "$CC" bc scan-build make
	build "$minsize" "$CC" dc scan-build make
}

karatsuba() {

	header "Running Karatsuba tests"
	make karatsuba_test
}

vg() {

	header "Running valgrind"

	makebuild "$release" "$CC" all
	runtest make valgrind_all

	make clean

	makebuild "$release" "$CC" bc
	runtest make valgrind_bc_all

	make clean

	makebuild "$release" "$CC" dc
	runtest make valgrind_dc

	make clean
}

build_dist() {

	project="$1"
	shift

	bc="$1"
	shift

	repo="$1"
	shift

	header "Building and testing $project"

	dist/release.py "$project" "$repo"

	d=$(pwd)

	cd "$repo"
	make

	cd "$d"
}

toybox() {

	toybox_bc="$toybox_repo/generated/unstripped/toybox"

	build_dist toybox "$toybox_bc" "$toybox_repo"

	runtest tests/all.sh bc "$toybox_bc" bc
	runtest tests/bc/timeconst.sh tests/bc/scripts/timeconst.bc "$toybox_bc" bc
}

busybox() {

	busybox_bc="$busybox_repo/busybox"

	build_dist busybox "$busybox_bc" "$busybox_repo"

	runtest tests/all.sh bc "$busybox_bc" bc
	runtest tests/all.sh dc "$busybox_bc" dc
	runtest tests/bc/timeconst.sh tests/bc/scripts/timeconst.bc "$busybox_bc" bc
}

debug() {

	CC="$1"
	shift

	runtests "$debug" "$CC"

	if [ "$CC" = "clang" ]; then
		runtests "$debug -fsanitize=address" "$CC" "$@"
		runtests "$debug -fsanitize=undefined" "$CC" "$@"
	fi
}

release() {
	runtests "$release" "$@"
}

reldebug() {
	runtests "$reldebug" "$@"
}

minsize() {
	runtests "$minsize" "$@"
}

debug="-g -O0 -fno-omit-frame-pointer"
release="-DNDEBUG -O3"
reldebug="-O3 -g -fno-omit-frame-pointer"
minsize="-DNDEBUG -Os"

set -e

script="$0"
scriptdir=$(dirname "$script")

if [ $# -lt 2 ]; then
	usage
fi

toybox_repo="$1"
shift
busybox_repo="$1"
shift

cd "$scriptdir"

make clean_tests

version=$(make version)

debug "clang"
release "clang"
reldebug "clang"
minsize "clang"

debug "gcc"
release "gcc"
reldebug "gcc"
minsize "gcc"

scan_build

toybox
busybox

build "$release" "$CC" all make

karatsuba
vg

make CC="afl-gcc" CFLAGS="$release"

echo ""
echo "Run $scriptdir/tests/randmath.py and the fuzzer."
echo "Then run the GitHub release script as follows:"
echo ""
echo "    <github_release> $version <msg> dist/ release.sh RELEASE.md tests/afl.py"
