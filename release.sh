#! /bin/sh
#
# Copyright (c) 2018-2019 Gavin D. Howard and contributors.
#
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# * Redistributions of source code must retain the above copyright notice, this
#   list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#

usage() {
	printf 'usage: %s [run_tests] [generate_tests] [test_with_gcc] [test_with_pcc] \\\n' "$script"
	printf '          [run_sanitizers] [run_valgrind]\n'
	exit 1
}

header() {

	local msg="$1"
	shift

	printf '\n'
	printf '*******************\n'
	printf '%s...\n' "$msg"
	printf '*******************\n'
	printf '\n'
}

do_make() {
	make -j4 "$@"
}

configure() {

	local CFLAGS="$1"
	shift

	local CC="$1"
	shift

	local configure_flags="$1"
	shift

	if [ "$gen_tests" -eq 0 ]; then
		configure_flags="-G $configure_flags"
	fi

	if [ "$CC" = "clang" ]; then
		CFLAGS="$clang_flags $CFLAGS"
	fi

	header "Running \"./configure.sh $configure_flags\" with CC=\"$CC\" and CFLAGS=\"$CFLAGS\""
	CFLAGS="$CFLAGS" CC="$CC" ./configure.sh $configure_flags > /dev/null

}

build() {

	local CFLAGS="$1"
	shift

	local CC="$1"
	shift

	local configure_flags="$1"
	shift

	configure "$CFLAGS" "$CC" "$configure_flags"

	header "Building with CC=\"$CC\" and CFLAGS=\"$CFLAGS\""

	do_make > /dev/null 2> "$scriptdir/.test.txt"

	if [ -s "$scriptdir/.test.txt" ]; then
		printf '%s generated warning(s):\n' "$CC"
		printf '\n'
		cat "$scriptdir/.test.txt"
		exit 1
	fi
}

runtest() {

	header "Running tests"

	if [ "$#" -gt 0 ]; then
		do_make "$@"
	else
		do_make test
	fi
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

	if [ "$run_tests" -ne 0 ]; then
		header "Running tests with configure flags \"$configure_flags\", CC=\"$CC\", and CFLAGS=\"$CFLAGS\""
	else
		header "Building with configure flags \"$configure_flags\", CC=\"$CC\", and CFLAGS=\"$CFLAGS\""
	fi

	build "$CFLAGS" "$CC" "$configure_flags"
	if [ "$run_tests" -ne 0 ]; then
		runtest
	fi

	do_make clean

	build "$CFLAGS" "$CC" "$configure_flags -b"
	if [ "$run_tests" -ne 0 ]; then
		runtest
	fi

	do_make clean

	build "$CFLAGS" "$CC" "$configure_flags -d"
	if [ "$run_tests" -ne 0 ]; then
		runtest
	fi

	do_make clean
}

runtestseries() {

	local CFLAGS="$1"
	shift

	local CC="$1"
	shift

	local run_tests="$1"
	shift

	runconfigtests "$CFLAGS" "$CC" "" "$run_tests"
	runconfigtests "$CFLAGS" "$CC" "-E" "$run_tests"
	runconfigtests "$CFLAGS" "$CC" "-H" "$run_tests"
	runconfigtests "$CFLAGS" "$CC" "-N" "$run_tests"
	runconfigtests "$CFLAGS" "$CC" "-S" "$run_tests"
	runconfigtests "$CFLAGS" "$CC" "-EH" "$run_tests"
	runconfigtests "$CFLAGS" "$CC" "-EN" "$run_tests"
	runconfigtests "$CFLAGS" "$CC" "-ES" "$run_tests"
	runconfigtests "$CFLAGS" "$CC" "-HN" "$run_tests"
	runconfigtests "$CFLAGS" "$CC" "-HS" "$run_tests"
	runconfigtests "$CFLAGS" "$CC" "-NS" "$run_tests"
	runconfigtests "$CFLAGS" "$CC" "-EHN" "$run_tests"
	runconfigtests "$CFLAGS" "$CC" "-EHS" "$run_tests"
	runconfigtests "$CFLAGS" "$CC" "-ENS" "$run_tests"
	runconfigtests "$CFLAGS" "$CC" "-HNS" "$run_tests"
	runconfigtests "$CFLAGS" "$CC" "-EHNS" "$run_tests"
}

runtests() {

	local CFLAGS="$1"
	shift

	local CC="$1"
	shift

	local run_tests="$1"
	shift

	runtestseries "$CFLAGS" "$CC" "$run_tests"
}

karatsuba() {

	header "Running Karatsuba tests"
	do_make karatsuba_test
}

vg() {

	header "Running valgrind"

	build "$reldebug" "$CC" "-g"
	runtest valgrind

	do_make clean_config

	build "$reldebug" "$CC" "-gb"
	runtest valgrind

	do_make clean_config

	build "$reldebug" "$CC" "-gd"
	runtest valgrind

	do_make clean_config
}

debug() {

	local CC="$1"
	shift

	local run_tests="$1"
	shift

	runtests "$debug" "$CC" "$run_tests"

	if [ "$CC" = "clang" -a "$run_sanitizers" -ne 0 ]; then
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

build_set() {

	local CC="$1"
	shift

	local run_tests="$1"
	shift

	debug "$CC" "$run_tests"
	release "$CC" "$run_tests"
	reldebug "$CC" "$run_tests"
	minsize "$CC" "$run_tests"
}

clang_flags="-Weverything -Wno-padded -Wno-switch-enum -Wno-format-nonliteral"
clang_flags="$clang_flags -Wno-cast-align -Wno-missing-noreturn -Wno-disabled-macro-expansion"
clang_flags="$clang_flags -Wno-unreachable-code -Wno-unreachable-code-return"

cflags="-Wall -Wextra -Werror -pedantic -std=c99"

debug="$cflags -g -O0 -fno-omit-frame-pointer"
release="$cflags -DNDEBUG -O3"
reldebug="$cflags -O3 -g -fno-omit-frame-pointer"
minsize="$cflags -DNDEBUG -Os"

set -e

script="$0"
scriptdir=$(dirname "$script")

if [ "$#" -gt 0 ]; then
	run_tests="$1"
	shift
else
	run_tests=1
fi

if [ "$#" -gt 0 ]; then
	gen_tests="$1"
	shift
else
	gen_tests=1
fi

if [ "$#" -gt 0 ]; then
	test_with_gcc="$1"
	shift
else
	test_with_gcc=1
fi

if [ "$#" -gt 0 ]; then
	test_with_pcc="$1"
	shift
else
	test_with_pcc=1
fi

if [ "$#" -gt 0 ]; then
	run_sanitizers="$1"
	shift
else
	run_sanitizers=1
fi

if [ "$#" -gt 0 ]; then
	run_valgrind="$1"
	shift
else
	run_valgrind=1
fi

cd "$scriptdir"

build "$debug" "clang" ""

header "Running math library under --standard"

printf 'quit\n' | bin/bc -ls

version=$(make version)

do_make clean_tests

build_set "clang" "$run_tests"

if [ "$test_with_gcc" -ne 0 ]; then
	build_set "gcc" "$run_tests"
fi

if [ "$test_with_pcc" -ne 0 ]; then
	build_set "pcc" "$run_tests"
fi

if [ "$run_tests" -ne 0 ]; then

	build "$release" "clang" ""

	karatsuba

	if [ "$run_valgrind" -ne 0 ]; then
		vg
	fi

	configure "$reldebug" "afl-gcc" "-HS"

	printf '\n'
	printf 'Run make\n'
	printf '\n'
	printf 'Then run %s/tests/randmath.py and the fuzzer.\n' "$scriptdir"
	printf '\n'
	printf 'Then run the GitHub release script as follows:\n'
	printf '\n'
	printf '    <github_release> %s <msg> .travis.yml codecov.yml release.sh \\\n' "$version"
	printf '    RELEASE.md tests/afl.py tests/randmath.py tests/bc/scripts/timeconst.bc\n'

fi
