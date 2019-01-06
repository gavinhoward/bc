#! /bin/sh
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
	printf 'usage: %s [toybox_repo] [run_tests] [run_scan_build]\n' "$script"
	exit 1
}

header() {

	local msg="$1"
	shift

	printf '\n'
	printf '******\n'
	printf '%s...\n' "$msg"
	printf '******\n'
	printf '\n'
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
		printf '%s generated warning(s):\n' "$CC"
		printf '\n'
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

	local run_tests="$1"
	shift

	runtestseries "$CFLAGS" "$CC" "$run_tests" make
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

	local run_tests="$1"
	shift

	runtests "$debug" "$CC" "$run_tests"

	if [ "$CC" = "clang" ]; then
		runtests "$debug -fsanitize=address" "$CC" "$run_tests"
		runtests "$debug -fsanitize=undefined" "$CC" "$run_tests"
		runtests "$debug -fsanitize=memory" "$CC" "$run_tests"
	fi
}

release() {

	local CC="$1"
	shift

	local run_tests="$1"
	shift

	runtests "$release" "$CC" "$run_tests"
}

reldebug() {

	local CC="$1"
	shift

	local run_tests="$1"
	shift

	runtests "$reldebug" "$CC" "$run_tests"
}

minsize() {

	local CC="$1"
	shift

	local run_tests="$1"
	shift

	runtests "$minsize" "$CC" "$run_tests"
}

cflags="-Wall -Wextra -pedantic -std=c99"

debug="$cflags -g -O0 -fno-omit-frame-pointer"
release="$cflags -DNDEBUG -O3"
reldebug="$cflags -O3 -g -fno-omit-frame-pointer"
minsize="$cflags -DNDEBUG -Os"

set -e

script="$0"
scriptdir=$(dirname "$script")

if [ "$#" -gt 0 ]; then
	toybox_repo="$1"
	shift
else
	toybox_repo=""
fi

if [ "$#" -gt 0 ]; then
	run_tests="$1"
	shift
else
	run_tests=1
fi

if [ "$#" -gt 0 ]; then
	run_scan_build="$1"
	shift
else
	run_scan_build=1
fi

cd "$scriptdir"

make clean_tests

version=$(make version)

debug "clang" "$run_tests"
release "clang" "$run_tests"
reldebug "clang" "$run_tests"
minsize "clang" "$run_tests"

debug "gcc" "$run_tests"
release "gcc" "$run_tests"
reldebug "gcc" "$run_tests"
minsize "gcc" "$run_tests"

if [ "$run_scan_build" -ne 0 ]; then
	scan_build
fi

if [ "$toybox_repo" != "" ]; then
	toybox
fi

build "$release" "clang" "" make

if [ "$run_tests" -ne 0 ]; then
	karatsuba
	vg
fi

build "$release" "afl-gcc" "" make

printf '\n'
printf 'Run %s/tests/randmath.py and the fuzzer.\n' "$scriptdir"
printf 'Then run the GitHub release script as follows:\n'
printf '\n'
printf '    <github_release> %s <msg> dist/ .clang-format release.sh RELEASE.md \\\n' "$version"
printf '    tests/afl.py tests/bc/scripts/timeconst.bc\n'
