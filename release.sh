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
	printf 'usage: %s [run_tests] [generate_tests] [test_with_gcc] [run_sanitizers] [run_valgrind]\n' "$script"
	exit 1
}

header() {

	_header_msg="$1"
	shift

	printf '\n'
	printf '*******************\n'
	printf '%s...\n' "$_header_msg"
	printf '*******************\n'
	printf '\n'
}

do_make() {
	make -j4 "$@"
}

configure() {

	_configure_CFLAGS="$1"
	shift

	_configure_CC="$1"
	shift

	_configure_configure_flags="$1"
	shift

	if [ "$gen_tests" -eq 0 ]; then
		_configure_configure_flags="-G $_configure_configure_flags"
	fi

	if [ "$_configure_CC" = "clang" ]; then
		_configure_CFLAGS="$clang_flags $_configure_CFLAGS"
	fi

	header "Running \"./configure.sh $_configure_configure_flags\" with CC=\"$_configure_CC\" and CFLAGS=\"$_configure_CFLAGS\""
	CFLAGS="$_configure_CFLAGS" CC="$_configure_CC" ./configure.sh $_configure_configure_flags > /dev/null

}

build() {

	_build_CFLAGS="$1"
	shift

	_build_CC="$1"
	shift

	_build_configure_flags="$1"
	shift

	configure "$_build_CFLAGS" "$_build_CC" "$_build_configure_flags"

	header "Building with CC=\"$_build_CC\" and CFLAGS=\"$_build_CFLAGS\""

	do_make > /dev/null 2> "$scriptdir/.test.txt"

	if [ -s "$scriptdir/.test.txt" ]; then
		printf '%s generated warning(s):\n' "$_build_CC"
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

	_runconfigtests_CFLAGS="$1"
	shift

	_runconfigtests_CC="$1"
	shift

	_runconfigtests_configure_flags="$1"
	shift

	_runconfigtests_run_tests="$1"
	shift

	if [ "$_runconfigtests_run_tests" -ne 0 ]; then
		header "Running tests with configure flags \"$_runconfigtests_configure_flags\", CC=\"$_runconfigtests_CC\", and CFLAGS=\"$_runconfigtests_CFLAGS\""
	else
		header "Building with configure flags \"$_runconfigtests_configure_flags\", CC=\"$_runconfigtests_CC\", and CFLAGS=\"$_runconfigtests_CFLAGS\""
	fi

	build "$_runconfigtests_CFLAGS" "$_runconfigtests_CC" "$_runconfigtests_configure_flags"
	if [ "$_runconfigtests_run_tests" -ne 0 ]; then
		runtest
	fi

	do_make clean

	build "$_runconfigtests_CFLAGS" "$_runconfigtests_CC" "$_runconfigtests_configure_flags -b"
	if [ "$_runconfigtests_run_tests" -ne 0 ]; then
		runtest
	fi

	do_make clean

	build "$_runconfigtests_CFLAGS" "$_runconfigtests_CC" "$_runconfigtests_configure_flags -d"
	if [ "$_runconfigtests_run_tests" -ne 0 ]; then
		runtest
	fi

	do_make clean
}

runtestseries() {

	_runtestseries_CFLAGS="$1"
	shift

	_runtestseries_CC="$1"
	shift

	_runtestseries_configure_flags="$1"
	shift

	_runtestseries_run_tests="$1"
	shift

	runconfigtests "$_runtestseries_CFLAGS" "$_runtestseries_CC" "$_runtestseries_configure_flags" "$_runtestseries_run_tests"
	runconfigtests "$_runtestseries_CFLAGS" "$_runtestseries_CC" "$_runtestseries_configure_flags -E" "$_runtestseries_run_tests"
	runconfigtests "$_runtestseries_CFLAGS" "$_runtestseries_CC" "$_runtestseries_configure_flags -H" "$_runtestseries_run_tests"
	runconfigtests "$_runtestseries_CFLAGS" "$_runtestseries_CC" "$_runtestseries_configure_flags -N" "$_runtestseries_run_tests"
	runconfigtests "$_runtestseries_CFLAGS" "$_runtestseries_CC" "$_runtestseries_configure_flags -S" "$_runtestseries_run_tests"
	runconfigtests "$_runtestseries_CFLAGS" "$_runtestseries_CC" "$_runtestseries_configure_flags -EH" "$_runtestseries_run_tests"
	runconfigtests "$_runtestseries_CFLAGS" "$_runtestseries_CC" "$_runtestseries_configure_flags -EN" "$_runtestseries_run_tests"
	runconfigtests "$_runtestseries_CFLAGS" "$_runtestseries_CC" "$_runtestseries_configure_flags -ES" "$_runtestseries_run_tests"
	runconfigtests "$_runtestseries_CFLAGS" "$_runtestseries_CC" "$_runtestseries_configure_flags -HN" "$_runtestseries_run_tests"
	runconfigtests "$_runtestseries_CFLAGS" "$_runtestseries_CC" "$_runtestseries_configure_flags -HS" "$_runtestseries_run_tests"
	runconfigtests "$_runtestseries_CFLAGS" "$_runtestseries_CC" "$_runtestseries_configure_flags -NS" "$_runtestseries_run_tests"
	runconfigtests "$_runtestseries_CFLAGS" "$_runtestseries_CC" "$_runtestseries_configure_flags -EHN" "$_runtestseries_run_tests"
	runconfigtests "$_runtestseries_CFLAGS" "$_runtestseries_CC" "$_runtestseries_configure_flags -EHS" "$_runtestseries_run_tests"
	runconfigtests "$_runtestseries_CFLAGS" "$_runtestseries_CC" "$_runtestseries_configure_flags -ENS" "$_runtestseries_run_tests"
	runconfigtests "$_runtestseries_CFLAGS" "$_runtestseries_CC" "$_runtestseries_configure_flags -HNS" "$_runtestseries_run_tests"
	runconfigtests "$_runtestseries_CFLAGS" "$_runtestseries_CC" "$_runtestseries_configure_flags -EHNS" "$_runtestseries_run_tests"
}

runtests() {

	_runtests_CFLAGS="$1"
	shift

	_runtests_CC="$1"
	shift

	_runtests_configure_flags="$1"
	shift

	_runtests_run_tests="$1"
	shift

	runtestseries "$_runtests_CFLAGS" "$_runtests_CC" "$_runtests_configure_flags" "$_runtests_run_tests"
}

karatsuba() {

	header "Running Karatsuba tests"
	do_make karatsuba_test
}

vg() {

	header "Running valgrind"

	build "$debug" "$CC" "-g"
	runtest valgrind

	do_make clean_config

	build "$debug" "$CC" "-gb"
	runtest valgrind

	do_make clean_config

	build "$debug" "$CC" "-gd"
	runtest valgrind

	do_make clean_config
}

debug() {

	_debug_CC="$1"
	shift

	_debug_run_tests="$1"
	shift

	runtests "$debug" "$_debug_CC" "-g" "$_debug_run_tests"

	if [ "$_debug_CC" = "clang" -a "$run_sanitizers" -ne 0 ]; then
		runtests "$debug -fsanitize=address" "$_debug_CC" "-g" "$_debug_run_tests"
		runtests "$debug -fsanitize=undefined" "$_debug_CC" "-g" "$_debug_run_tests"
		runtests "$debug -fsanitize=memory" "$_debug_CC" "-g" "$_debug_run_tests"
	fi
}

release() {

	_release_CC="$1"
	shift

	_release_run_tests="$1"
	shift

	runtests "$release" "$_release_CC" "-O3" "$_release_run_tests"
}

reldebug() {

	_reldebug_CC="$1"
	shift

	_reldebug_run_tests="$1"
	shift

	runtests "$debug" "$_reldebug_CC" "-gO3" "$_reldebug_run_tests"
}

minsize() {

	_minsize_CC="$1"
	shift

	_minsize_run_tests="$1"
	shift

	runtests "$release" "$_minsize_CC" "-Os" "$_minsize_run_tests"
}

build_set() {

	_build_set_CC="$1"
	shift

	_build_set_run_tests="$1"
	shift

	debug "$_build_set_CC" "$_build_set_run_tests"
	release "$_build_set_CC" "$_build_set_run_tests"
	reldebug "$_build_set_CC" "$_build_set_run_tests"
	minsize "$_build_set_CC" "$_build_set_run_tests"
}

clang_flags="-Weverything -Wno-padded -Wno-switch-enum -Wno-format-nonliteral"
clang_flags="$clang_flags -Wno-cast-align -Wno-missing-noreturn -Wno-disabled-macro-expansion"
clang_flags="$clang_flags -Wno-unreachable-code -Wno-unreachable-code-return"

cflags="-Wall -Wextra -Werror -pedantic -std=c99"

debug="$cflags -fno-omit-frame-pointer"
release="$cflags -DNDEBUG"

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

if [ "$run_tests" -ne 0 ]; then

	build "$release" "clang" ""

	karatsuba

	if [ "$run_valgrind" -ne 0 ]; then
		vg
	fi

	configure "$debug" "afl-gcc" "-HSg"

	printf '\n'
	printf 'Run make\n'
	printf '\n'
	printf 'Then run %s/tests/randmath.py and the fuzzer.\n' "$scriptdir"
	printf '\n'
	printf 'Then run ASan on the fuzzer test cases with the following build:\n'
	printf '\n'
	printf '    CFLAGS=-fsanitize=address ./configure.sh -O3\n'
	printf '    make\n'
	printf '\n'
	printf 'Then run the GitHub release script as follows:\n'
	printf '\n'
	printf '    <github_release> %s <msg> .travis.yml codecov.yml release.sh \\\n' "$version"
	printf '    RELEASE.md tests/afl.py tests/randmath.py tests/bc/scripts/timeconst.bc\n'

fi
