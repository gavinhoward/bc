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
	echo "usage: $script toybox_repo"
	exit 1
}

header() {

	local msg="$1"
	shift

	echo ""
	echo "******"
	echo "$msg..."
	echo "******"
	echo ""
}

build() {

	local CFLAGS="$1"
	shift

	local CC="$1"
	shift

	local configure_flags="$1"
	shift

	local exe="$1"
	shift

	header "Running \"./configure.sh $configure_flags\" with CC=\"$CC\" and CFLAGS=\"$CFLAGS\""

	CFLAGS="$CFLAGS" CC="$CC" ./configure.sh $configure_flags > /dev/null
	"$exe" "$@" 2> "$scriptdir/.test.txt"

	if [ -s "$scriptdir/.test.txt" ]; then
		echo "$CC generated warning(s):"
		echo ""
		cat "$scriptdir/.test.txt"
		exit 1
	fi
}

runtest() {

	header "Running tests"

	make test_all
}

runconfigtests() {

	local CFLAGS="$1"
	shift

	local CC="$1"
	shift

	local configure_flags="$1"
	shift

	local run_tests="$1"
	shift

	local exe="$1"
	shift

	if [ "$run_tests" -ne 0 ]; then
		header "Running tests with configure flags \"$configure_flags\", CC=\"$CC\", and CFLAGS=\"$CFLAGS\""
	else
		header "Building with configure flags \"$configure_flags\", CC=\"$CC\", and CFLAGS=\"$CFLAGS\""
	fi

	build "$CFLAGS" "$CC" "$configure_flags" "$exe" "$@"
	if [ "$run_tests" -ne 0 ]; then
		runtest
	fi

	make clean

	build "$CFLAGS" "$CC" "$configure_flags -b" "$exe" "$@"
	if [ "$run_tests" -ne 0 ]; then
		runtest
	fi

	make clean

	build "$CFLAGS" "$CC" "$configure_flags -d" "$exe" "$@"
	if [ "$run_tests" -ne 0 ]; then
		runtest
	fi

	make clean
}

runtestseries() {

	local CFLAGS="$1"
	shift

	local CC="$1"
	shift

	local run_tests="$1"
	shift

	local exe="$1"
	shift

	runconfigtests "$CFLAGS" "$CC" "" "$run_tests" "$exe" "$@"
	runconfigtests "$CFLAGS" "$CC" "-E" "$run_tests" "$exe" "$@"
	runconfigtests "$CFLAGS" "$CC" "-H" "$run_tests" "$exe" "$@"
	runconfigtests "$CFLAGS" "$CC" "-R" "$run_tests" "$exe" "$@"
	runconfigtests "$CFLAGS" "$CC" "-S" "$run_tests" "$exe" "$@"
	runconfigtests "$CFLAGS" "$CC" "-EH" "$run_tests" "$exe" "$@"
	runconfigtests "$CFLAGS" "$CC" "-ER" "$run_tests" "$exe" "$@"
	runconfigtests "$CFLAGS" "$CC" "-ES" "$run_tests" "$exe" "$@"
	runconfigtests "$CFLAGS" "$CC" "-HR" "$run_tests" "$exe" "$@"
	runconfigtests "$CFLAGS" "$CC" "-HS" "$run_tests" "$exe" "$@"
	runconfigtests "$CFLAGS" "$CC" "-RS" "$run_tests" "$exe" "$@"
	runconfigtests "$CFLAGS" "$CC" "-EHR" "$run_tests" "$exe" "$@"
	runconfigtests "$CFLAGS" "$CC" "-EHS" "$run_tests" "$exe" "$@"
	runconfigtests "$CFLAGS" "$CC" "-ERS" "$run_tests" "$exe" "$@"
	runconfigtests "$CFLAGS" "$CC" "-HRS" "$run_tests" "$exe" "$@"
	runconfigtests "$CFLAGS" "$CC" "-EHRS" "$run_tests" "$exe" "$@"
}

runtests() {

	local CFLAGS="$1"
	shift

	local CC="$1"
	shift

	runtestseries "$CFLAGS" "$CC" 1 make
}

scan_build() {

	header "Running scan-build"

	runtestseries "$debug" "$CC" 0 scan-build make
	runtestseries "$release" "$CC" 0 scan-build make
	runtestseries "$reldebug" "$CC" 0 scan-build make
	runtestseries "$minsize" "$CC" 0 scan-build make
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

	local project="$1"
	shift

	local bc="$1"
	shift

	local repo="$1"
	shift

	header "Building and testing $project"

	dist/release.py "$project" "$repo"

	d=$(pwd)

	cd "$repo"
	make clean
	make

	cd "$d"
}

toybox() {

	toybox_bc="$toybox_repo/generated/unstripped/toybox"

	build_dist toybox "$toybox_bc" "$toybox_repo"

	runtest tests/all.sh bc "$toybox_bc" bc
	runtest tests/bc/timeconst.sh tests/bc/scripts/timeconst.bc "$toybox_bc" bc
}

debug() {

	local CC="$1"
	shift

	runtests "$debug" "$CC"

	if [ "$CC" = "clang" ]; then
		runtests "$debug -fsanitize=address" "$CC"
		runtests "$debug -fsanitize=undefined" "$CC"
		runtests "$debug -fsanitize=memory" "$CC"
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

cflags="-Wall -Wextra -pedantic -std=c99"

debug="$cflags -g -O0 -fno-omit-frame-pointer"
release="$cflags -DNDEBUG -O3"
reldebug="$cflags -O3 -g -fno-omit-frame-pointer"
minsize="$cflags -DNDEBUG -Os"

set -e

script="$0"
scriptdir=$(dirname "$script")

if [ $# -lt 1 ]; then
	usage
fi

toybox_repo="$1"
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

build "$release" "clang" "" make

karatsuba
vg

build "$release" "afl-gcc" "" make

echo ""
echo "Run $scriptdir/tests/randmath.py and the fuzzer."
echo "Then run the GitHub release script as follows:"
echo ""
echo "    <github_release> $version <msg> dist/ .clang-format release.sh RELEASE.md \\"
echo "    tests/afl.py tests/bc/scripts/timeconst.bc"
