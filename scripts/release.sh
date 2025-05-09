#! /bin/sh
#
# SPDX-License-Identifier: BSD-2-Clause
#
# Copyright (c) 2018-2025 Gavin D. Howard and contributors.
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

# For macOS from Ventura on, run using the following:
#
# scripts/release.sh 1 1 0 1 0 0 0 0 1 0 1 1 1 0 0 0 0
#
# For OpenBSD, run using the following:
#
# scripts/release.sh 1 0 0 1 0 0 0 0 1 0 0 0 0 0 0 1 0
#
# For FreeBSD, run using the following:
#
# scripts/release.sh 1 1 0 1 1 0 0 0 1 0 1 0 1 0 0 1 1
#
# For Linux, run two separate ones (in different checkouts), like so:
#
# scripts/release.sh 1 1 1 1 0 1 0 0 1 0 1 0 1 0 0 1 1
# cd build; ../scripts/release.sh 1 1 1 0 1 0 1 0 1 0 1 0 0 1 1 1 1
#
# Yes, I usually do sanitizers with Clang and Valgrind with GCC, and I also do
# out-of-source builds with GCC.
#
# The reason I run history tests with GCC and not with Clang is because Clang
# already runs slower as a result of running with sanitizers, and the history
# tests are a little sensitive to load on a system.
#
# If this script fails on any platform when starting the Karatsuba test, check
# that Python is installed, especially if the error says something like:
# "karatsuba.py: not found".

# Print the usage and exit with an error. Each parameter should be an integer.
# Non-zero activates, and zero deactivates.
# @param 1  A message to print.
usage() {
	if [ $# -eq 1 ]; then
		printf '%s\n\n' "$1"
	fi
	printf 'usage: %s [run_tests] [generate_tests] [run_problematic_tests] \n' "$script"
	printf '          [test_with_clang] [test_with_gcc] [run_sanitizers] [run_valgrind] \n'
	printf '          [test_settings] [run_64_bit] [run_gen_script] [test_c11] \n'
	printf '          [test_128_bit] [test_computed_goto] [test_karatsuba] [test_history] \n'
	printf '          [test_editline] [test_readline]\n'
	exit 1
}

# Print a header with a message. This is just to make it easy to track progress.
# @param msg  The message to print in the header.
header() {

	_header_msg="$1"
	shift

	printf '\n'
	printf '*******************\n'
	printf "$_header_msg"
	printf '\n'
	printf '*******************\n'
	printf '\n'
}

# Easy way to call make.
do_make() {
	# No reason to do 64 except to see if I actually can overload my system. :)
	# Well, also that it might actually improve throughput as other jobs can run
	# while some are waiting.
	make -j64 "$@"
}

# Run configure.sh.
# @param CFLAGS           The CFLAGS.
# @param CC               The C compiler.
# @param configure_flags  The flags for configure.sh itself.
# @param GEN_HOST         The setting for GEN_HOST.
# @param LONG_BIT         The setting for LONG_BIT.
configure() {

	_configure_CFLAGS="$1"
	shift

	_configure_CC="$1"
	shift

	_configure_configure_flags="$1"
	shift

	_configure_GEN_HOST="$1"
	shift

	_configure_LONG_BIT="$1"
	shift

	# Make sure to not generate tests if necessary.
	if [ "$gen_tests" -eq 0 ]; then
		_configure_configure_flags="-G $_configure_configure_flags"
	fi

	# Make sure to skip problematic tests if necessary.
	if [ "$problematic_tests" -eq 0 ]; then
		_configure_configure_flags="-P $_configure_configure_flags"
	fi

	# Choose the right extra flags.
	if [ "$_configure_CC" = "clang" ]; then

		_configure_CFLAGS="$clang_flags $_configure_CFLAGS"

		# We need to quiet this warning from Clang because the configure.sh docs
		# have this warning, so people should know. Also, I want this script to
		# work.
		if [ "$_configure_GEN_HOST" -eq 0 ]; then
			_configure_CFLAGS="$_configure_CFLAGS -Wno-overlength-strings"
		fi

	elif [ "$_configure_CC" = "gcc" ]; then

		_configure_CFLAGS="$gcc_flags $_configure_CFLAGS"

		# We need to quiet this warning from GCC because the configure.sh docs
		# have this warning, so people should know. Also, I want this script to
		# work.
		if [ "$_configure_GEN_HOST" -eq 0 ]; then
			_configure_CFLAGS="$_configure_CFLAGS -Wno-overlength-strings"
		fi

	fi

	# Print the header and do the job.
	_configure_header=$(printf 'Running configure.sh %s ...' "$_configure_configure_flags")
	_configure_header=$(printf "$_configure_header\n    CC=\"%s\"\n" "$_configure_CC")
	_configure_header=$(printf "$_configure_header\n    CFLAGS=\"%s\"\n" "$_configure_CFLAGS")
	_configure_header=$(printf "$_configure_header\n    LONG_BIT=%s" "$_configure_LONG_BIT")
	_configure_header=$(printf "$_configure_header\n    GEN_HOST=%s" "$_configure_GEN_HOST")

	header "$_configure_header"
	CFLAGS="$_configure_CFLAGS" CC="$_configure_CC" GEN_HOST="$_configure_GEN_HOST" \
		LONG_BIT="$_configure_LONG_BIT" "$real/configure.sh" $_configure_configure_flags > /dev/null 2> /dev/null
}

# Build with make. This function also captures and outputs any warnings if they
# exists because as far as I am concerned, warnings are not acceptable for
# release.
# @param CFLAGS           The CFLAGS.
# @param CC               The C compiler.
# @param configure_flags  The flags for configure.sh itself.
# @param GEN_HOST         The setting for GEN_HOST.
# @param LONG_BIT         The setting for LONG_BIT.
build() {

	_build_CFLAGS="$1"
	shift

	_build_CC="$1"
	shift

	_build_configure_flags="$1"
	shift

	_build_GEN_HOST="$1"
	shift

	_build_LONG_BIT="$1"
	shift

	configure "$_build_CFLAGS" "$_build_CC" "$_build_configure_flags" "$_build_GEN_HOST" "$_build_LONG_BIT"

	_build_header=$(printf 'Building...\n    CC=%s' "$_build_CC")
	_build_header=$(printf "$_build_header\n    CFLAGS=\"%s\"" "$_build_CFLAGS")
	_build_header=$(printf "$_build_header\n    LONG_BIT=%s" "$_build_LONG_BIT")
	_build_header=$(printf "$_build_header\n    GEN_HOST=%s" "$_build_GEN_HOST")

	header "$_build_header"

	set +e

	# Capture and print warnings.
	do_make > /dev/null 2> "./.test.txt"
	err=$?

	set -e

	if [ -s "./.test.txt" ]; then
		printf '%s generated warning(s):\n' "$_build_CC"
		printf '\n'
		cat "./.test.txt"
		exit 1
	fi

	if [ "$err" -ne 0 ]; then
		exit "$err"
	fi
}

# Run tests with make.
runtest() {

	header "Running tests"

	if [ "$#" -gt 0 ]; then
		do_make "$@"
	else

		do_make test

		if [ "$test_history" -ne 0 ]; then
			do_make test_history
		fi
	fi
}

# Builds and runs tests with both calculators, then bc only, then dc only. If
# run_tests is false, then it just does the builds.
# @param CFLAGS           The CFLAGS.
# @param CC               The C compiler.
# @param configure_flags  The flags for configure.sh itself.
# @param GEN_HOST         The setting for GEN_HOST.
# @param LONG_BIT         The setting for LONG_BIT.
# @param run_tests        Whether to run tests or not.
runconfigtests() {

	_runconfigtests_CFLAGS="$1"
	shift

	_runconfigtests_CC="$1"
	shift

	_runconfigtests_configure_flags="$1"
	shift

	_runconfigtests_GEN_HOST="$1"
	shift

	_runconfigtests_LONG_BIT="$1"
	shift

	_runconfigtests_run_tests="$1"
	shift

	if [ "$_runconfigtests_run_tests" -ne 0 ]; then
		_runconfigtests_header=$(printf 'Running tests with configure flags')
	else
		_runconfigtests_header=$(printf 'Building with configure flags')
	fi

	_runconfigtests_header=$(printf "$_runconfigtests_header \"%s\" ...\n" "$_runconfigtests_configure_flags")
	_runconfigtests_header=$(printf "$_runconfigtests_header\n    CC=%s\n" "$_runconfigseries_CC")
	_runconfigtests_header=$(printf "$_runconfigtests_header\n    CFLAGS=\"%s\"" "$_runconfigseries_CFLAGS")
	_runconfigtests_header=$(printf "$_runconfigtests_header\n    LONG_BIT=%s" "$_runconfigtests_LONG_BIT")
	_runconfigtests_header=$(printf "$_runconfigtests_header\n    GEN_HOST=%s" "$_runconfigtests_GEN_HOST")

	header "$_runconfigtests_header"

	build "$_runconfigtests_CFLAGS" "$_runconfigtests_CC" \
		"$_runconfigtests_configure_flags" "$_runconfigtests_GEN_HOST" \
		"$_runconfigtests_LONG_BIT"

	if [ "$_runconfigtests_run_tests" -ne 0 ]; then
		runtest
	fi

	do_make clean

	build "$_runconfigtests_CFLAGS" "$_runconfigtests_CC" \
		"$_runconfigtests_configure_flags -b" "$_runconfigtests_GEN_HOST" \
		"$_runconfigtests_LONG_BIT"

	if [ "$_runconfigtests_run_tests" -ne 0 ]; then
		runtest
	fi

	do_make clean

	build "$_runconfigtests_CFLAGS" "$_runconfigtests_CC" \
		"$_runconfigtests_configure_flags -d" "$_runconfigtests_GEN_HOST" \
		"$_runconfigtests_LONG_BIT"

	if [ "$_runconfigtests_run_tests" -ne 0 ]; then
		runtest
	fi

	do_make clean
}

# Builds and runs tests with runconfigtests(), but also does 64-bit, 32-bit, and
# 128-bit rand, if requested. It also does it with the gen script (strgen.sh) if
# requested. If run_tests is false, it just does the builds.
# @param CFLAGS           The CFLAGS.
# @param CC               The C compiler.
# @param configure_flags  The flags for configure.sh itself.
# @param run_tests        Whether to run tests or not.
runconfigseries() {

	_runconfigseries_CFLAGS="$1"
	shift

	_runconfigseries_CC="$1"
	shift

	_runconfigseries_configure_flags="$1"
	shift

	_runconfigseries_run_tests="$1"
	shift

	if [ "$run_64_bit" -ne 0 ]; then

		if [ "$test_128_bit" -ne 0 ]; then
			runconfigtests "$_runconfigseries_CFLAGS" "$_runconfigseries_CC" \
				"$_runconfigseries_configure_flags" 1 64 "$_runconfigseries_run_tests"
		fi

		if [ "$run_gen_script" -ne 0 ]; then
			runconfigtests "$_runconfigseries_CFLAGS" "$_runconfigseries_CC" \
				"$_runconfigseries_configure_flags" 0 64 "$_runconfigseries_run_tests"
		fi

		runconfigtests "$_runconfigseries_CFLAGS -DBC_RAND_BUILTIN=0" "$_runconfigseries_CC" \
			"$_runconfigseries_configure_flags" 1 64 "$_runconfigseries_run_tests"

		# Test Editline and Readline if history is not turned off.
		if [ "${_runconfigseries_configure_flags#*H}" = "${_runconfigseries_configure_flags}" ]; then

			if [ "$test_editline" -ne 0 ]; then
				runconfigtests "$_runconfigseries_CFLAGS -DBC_RAND_BUILTIN=0" "$_runconfigseries_CC" \
					"$_runconfigseries_configure_flags -e" 1 64 "$_runconfigseries_run_tests"
			fi

			if [ "$test_readline" -ne 0 ]; then
				runconfigtests "$_runconfigseries_CFLAGS -DBC_RAND_BUILTIN=0" "$_runconfigseries_CC" \
					"$_runconfigseries_configure_flags -r" 1 64 "$_runconfigseries_run_tests"
			fi

		fi

	fi

	runconfigtests "$_runconfigseries_CFLAGS" "$_runconfigseries_CC" \
		"$_runconfigseries_configure_flags" 1 32 "$_runconfigseries_run_tests"

	if [ "$run_gen_script" -ne 0 ]; then
		runconfigtests "$_runconfigseries_CFLAGS" "$_runconfigseries_CC" \
			"$_runconfigseries_configure_flags" 0 32 "$_runconfigseries_run_tests"
	fi
}

# Builds and runs tests with each setting combo running runconfigseries(). If
# run_tests is false, it just does the builds.
# @param CFLAGS           The CFLAGS.
# @param CC               The C compiler.
# @param configure_flags  The flags for configure.sh itself.
# @param run_tests        Whether to run tests or not.
runsettingsseries() {

	_runsettingsseries_CFLAGS="$1"
	shift

	_runsettingsseries_CC="$1"
	shift

	_runsettingsseries_configure_flags="$1"
	shift

	_runsettingsseries_run_tests="$1"
	shift

	if [ "$test_settings" -ne 0 ]; then

		while read _runsettingsseries_s; do
			runconfigseries "$_runsettingsseries_CFLAGS" "$_runsettingsseries_CC" \
				"$_runsettingsseries_configure_flags $_runsettingsseries_s" \
				"$_runsettingsseries_run_tests"
		done < "$scriptdir/release_settings.txt"

	else
		runconfigseries "$_runsettingsseries_CFLAGS" "$_runsettingsseries_CC" \
			"$_runsettingsseries_configure_flags" "$_runsettingsseries_run_tests"
	fi
}

# Builds and runs tests with each build type running runsettingsseries(). If
# run_tests is false, it just does the builds.
# @param CFLAGS           The CFLAGS.
# @param CC               The C compiler.
# @param configure_flags  The flags for configure.sh itself.
# @param run_tests        Whether to run tests or not.
runtestseries() {

	_runtestseries_CFLAGS="$1"
	shift

	_runtestseries_CC="$1"
	shift

	_runtestseries_configure_flags="$1"
	shift

	_runtestseries_run_tests="$1"
	shift

	_runtestseries_flags="E H N EH EN HN EHN"

	runsettingsseries "$_runtestseries_CFLAGS" "$_runtestseries_CC" \
		"$_runtestseries_configure_flags" "$_runtestseries_run_tests"

	for _runtestseries_f in $_runtestseries_flags; do
		runsettingsseries "$_runtestseries_CFLAGS" "$_runtestseries_CC" \
			"$_runtestseries_configure_flags -$_runtestseries_f" "$_runtestseries_run_tests"
	done
}

# Builds and runs the tests for bcl. If run_tests is false, it just does the
# builds.
# @param CFLAGS           The CFLAGS.
# @param CC               The C compiler.
# @param configure_flags  The flags for configure.sh itself.
# @param run_tests        Whether to run tests or not.
runlibtests() {

	_runlibtests_CFLAGS="$1"
	shift

	_runlibtests_CC="$1"
	shift

	_runlibtests_configure_flags="$1"
	shift

	_runlibtests_run_tests="$1"
	shift

	_runlibtests_configure_flags="$_runlibtests_configure_flags -a"

	build "$_runlibtests_CFLAGS" "$_runlibtests_CC" "$_runlibtests_configure_flags" 1 64

	if [ "$_runlibtests_run_tests" -ne 0 ]; then
		runtest test
	fi

	build "$_runlibtests_CFLAGS" "$_runlibtests_CC" "$_runlibtests_configure_flags" 1 32

	if [ "$_runlibtests_run_tests" -ne 0 ]; then
		runtest test
	fi
}

# Builds and runs tests under C99, then C11, if requested, using
# runtestseries(). If run_tests is false, it just does the builds.
# @param CFLAGS           The CFLAGS.
# @param CC               The C compiler.
# @param configure_flags  The flags for configure.sh itself.
# @param run_tests        Whether to run tests or not.
runtests() {

	_runtests_CFLAGS="$1"
	shift

	_runtests_CC="$1"
	shift

	_runtests_configure_flags="$1"
	shift

	_runtests_run_tests="$1"
	shift

	runtestseries "-std=c99 $_runtests_CFLAGS" "$_runtests_CC" "$_runtests_configure_flags" "$_runtests_run_tests"

	if [ "$test_c11" -ne 0 ]; then
		runtestseries "-std=c11 $_runtests_CFLAGS" "$_runtests_CC" "$_runtests_configure_flags" "$_runtests_run_tests"
	fi
}

# Runs the karatsuba tests.
karatsuba() {

	header "Running Karatsuba tests"
	do_make karatsuba_test
}

# Builds and runs under valgrind. It runs both, bc only, then dc only.
vg() {

	header "Running valgrind"

	if [ "$run_64_bit" -ne 0 ]; then
		_vg_bits=64
	else
		_vg_bits=32
	fi

	build "$debug -std=c99" "gcc" "-O3 -gv" "1" "$_vg_bits"
	runtest test

	do_make clean_config

	build "$debug -std=c99" "gcc" "-O3 -gvb" "1" "$_vg_bits"
	runtest test

	do_make clean_config

	build "$debug -std=c99" "gcc" "-O3 -gvd" "1" "$_vg_bits"
	runtest test

	do_make clean_config

	build "$debug -std=c99" "gcc" "-O3 -gva" "1" "$_vg_bits"
	runtest test

	do_make clean_config
}

# Builds the debug series and runs the tests if run_tests allows. If sanitizers
# are enabled, it also does UBSan.
# @param CC         The C compiler.
# @param run_tests  Whether to run tests or not.
debug() {

	_debug_CC="$1"
	shift

	_debug_run_tests="$1"
	shift


	if [ "$_debug_CC" = "clang" -a "$run_sanitizers" -ne 0 ]; then
		runtests "$debug -fsanitize=undefined" "$_debug_CC" "-gm" "$_debug_run_tests"
	else
		runtests "$debug" "$_debug_CC" "-g" "$_debug_run_tests"
	fi

	if [ "$_debug_CC" = "clang" -a "$run_sanitizers" -ne 0 ]; then
		runlibtests "$debug -fsanitize=undefined" "$_debug_CC" "-gm" "$_debug_run_tests"
	else
		runlibtests "$debug" "$_debug_CC" "-g" "$_debug_run_tests"
	fi
}

# Builds the release series and runs the test if run_tests allows.
# @param CC         The C compiler.
# @param run_tests  Whether to run tests or not.
release() {

	_release_CC="$1"
	shift

	_release_run_tests="$1"
	shift

	runtests "$release" "$_release_CC" "-O3" "$_release_run_tests"

	runlibtests "$release" "$_release_CC" "-O3" "$_release_run_tests"
}

# Builds the release debug series and runs the test if run_tests allows. If
# sanitizers are enabled, it also does ASan and MSan.
# @param CC         The C compiler.
# @param run_tests  Whether to run tests or not.
reldebug() {

	_reldebug_CC="$1"
	shift

	_reldebug_run_tests="$1"
	shift


	if [ "$_reldebug_CC" = "clang" -a "$run_sanitizers" -ne 0 ]; then
		runtests "$debug -fsanitize=address" "$_reldebug_CC" "-mgO3" "$_reldebug_run_tests"
		runtests "$debug -fsanitize=memory" "$_reldebug_CC" "-mgO3" "$_reldebug_run_tests"
	else
		runtests "$debug" "$_reldebug_CC" "-gO3" "$_reldebug_run_tests"
	fi


	if [ "$_reldebug_CC" = "clang" -a "$run_sanitizers" -ne 0 ]; then
		runlibtests "$debug -fsanitize=address" "$_reldebug_CC" "-mgO3" "$_reldebug_run_tests"
		runlibtests "$debug -fsanitize=memory" "$_reldebug_CC" "-mgO3" "$_reldebug_run_tests"
	else
		runlibtests "$debug" "$_reldebug_CC" "-gO3" "$_reldebug_run_tests"
	fi
}

# Builds the min size release series and runs the test if run_tests allows.
# @param CC         The C compiler.
# @param run_tests  Whether to run tests or not.
minsize() {

	_minsize_CC="$1"
	shift

	_minsize_run_tests="$1"
	shift

	runtests "$release" "$_minsize_CC" "-Os" "$_minsize_run_tests"

	runlibtests "$release" "$_minsize_CC" "-Os" "$_minsize_run_tests"
}

# Builds all sets: debug, release, release debug, and min size, and runs the
# tests if run_tests allows.
# @param CC         The C compiler.
# @param run_tests  Whether to run tests or not.
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

set -e

script="$0"
scriptdir=$(dirname "$script")

. "$scriptdir/functions.sh"

# Unset all bc and dc environment variables. This is intended to allow this
# script to run in a clean environment.
unset POSIXLY_CORRECT
unset BC_BANNER
unset BC_ENV_ARGS
unset DC_ENV_ARGS
unset BC_LINE_LENGTH
unset DC_LINE_LENGTH
unset BC_SIGINT_RESET
unset DC_SIGINT_RESET
unset BC_TTY_MODE
unset DC_TTY_MODE
unset BC_PROMPT
unset DC_PROMPT
unset BC_EXPR_EXIT
unset DC_EXPR_EXIT
unset BC_DIGIT_CLAMP
unset DC_DIGIT_CLAMP

os=$(uname)
# Set some strict warning flags. Clang's -Weverything can be way too strict, so
# we actually have to turn off some things.
clang_flags="-Weverything -Wno-padded -Wno-unsafe-buffer-usage"
clang_flags="$clang_flags -Wno-poison-system-directories -Wno-switch-default"
clang_flags="$clang_flags -Wno-unknown-warning-option -Wno-pre-c11-compat"
gcc_flags="-Wno-clobbered"

# Common CFLAGS.
cflags="-Wall -Wextra -Werror -pedantic"

# Common debug and release flags.
debug="$cflags -fno-omit-frame-pointer"
release="$cflags -DNDEBUG"

real=$(realpath "$scriptdir/../")

# Whether to run tests.
if [ "$#" -gt 0 ]; then
	run_tests="$1"
	shift
	check_bool_arg "$run_tests"
else
	run_tests=1
	check_bool_arg "$run_tests"
fi

# Whether to generate tests. On platforms like OpenBSD, there is no GNU bc to
# generate tests, so this must be off.
if [ "$#" -gt 0 ]; then
	gen_tests="$1"
	shift
	check_bool_arg "$gen_tests"
else
	gen_tests=1
	check_bool_arg "$gen_tests"
fi

# Whether to run problematic tests. This needs to be off on FreeBSD.
if [ "$#" -gt 0 ]; then
	problematic_tests="$1"
	shift
	check_bool_arg "$problematic_tests"
else
	problematic_tests=1
	check_bool_arg "$problematic_tests"
fi

# Whether to test with clang.
if [ "$#" -gt 0 ]; then
	test_with_clang="$1"
	shift
	check_bool_arg "$test_with_clang"
else
	test_with_clang=1
	check_bool_arg "$test_with_clang"
fi

# Whether to test with gcc.
if [ "$#" -gt 0 ]; then
	test_with_gcc="$1"
	shift
	check_bool_arg "$test_with_gcc"
else
	test_with_gcc=1
	check_bool_arg "$test_with_clang"
fi

# Whether to test with sanitizers.
if [ "$#" -gt 0 ]; then
	run_sanitizers="$1"
	check_bool_arg "$run_sanitizers"
	shift
else
	run_sanitizers=1
	check_bool_arg "$run_sanitizers"
fi

# Whether to test with valgrind.
if [ "$#" -gt 0 ]; then
	run_valgrind="$1"
	shift
	check_bool_arg "$run_valgrind"
else
	run_valgrind=1
	check_bool_arg "$run_valgrind"
fi

# Whether to test all settings combos.
if [ "$#" -gt 0 ]; then
	test_settings="$1"
	shift
	check_bool_arg "$test_settings"
else
	test_settings=1
	check_bool_arg "$test_settings"
fi

# Whether to test 64-bit in addition to 32-bit.
if [ "$#" -gt 0 ]; then
	run_64_bit="$1"
	shift
	check_bool_arg "$run_64_bit"
else
	run_64_bit=1
	check_bool_arg "$run_64_bit"
fi

# Whether to test with strgen.sh in addition to strgen.c.
if [ "$#" -gt 0 ]; then
	run_gen_script="$1"
	shift
	check_bool_arg "$run_gen_script"
else
	run_gen_script=0
	check_bool_arg "$run_gen_script"
fi

# Whether to test on C11 in addition to C99.
if [ "$#" -gt 0 ]; then
	test_c11="$1"
	shift
	check_bool_arg "$test_c11"
else
	test_c11=0
	check_bool_arg "$test_c11"
fi

# Whether to test 128-bit integers in addition to no 128-bit integers.
if [ "$#" -gt 0 ]; then
	test_128_bit="$1"
	shift
	check_bool_arg "$test_128_bit"
else
	test_128_bit=0
	check_bool_arg "$test_128_bit"
fi

# Whether to test with computed goto or not.
if [ "$#" -gt 0 ]; then
	test_computed_goto="$1"
	shift
	check_bool_arg "$test_computed_goto"
else
	test_computed_goto=0
	check_bool_arg "$test_computed_goto"
fi

# Whether to test history or not.
if [ "$#" -gt 0 ]; then
	test_karatsuba="$1"
	shift
	check_bool_arg "$test_karatsuba"
else
	test_karatsuba=1
	check_bool_arg "$test_karatsuba"
fi

# Whether to test history or not.
if [ "$#" -gt 0 ]; then
	test_history="$1"
	shift
	check_bool_arg "$test_history"
else
	test_history=0
	check_bool_arg "$test_history"
fi

# Whether to test editline or not.
if [ "$#" -gt 0 ]; then
	test_editline="$1"
	shift
	check_bool_arg "$test_editline"
else
	test_editline=0
	check_bool_arg "$test_editline"
fi

# Whether to test editline or not.
if [ "$#" -gt 0 ]; then
	test_readline="$1"
	shift
	check_bool_arg "$test_readline"
else
	test_readline=0
	check_bool_arg "$test_readline"
fi

if [ "$run_64_bit" -ne 0 ]; then
	bits=64
else
	bits=32
fi

if [ "$test_computed_goto" -eq 0 ]; then
	clang_flags="-DBC_NO_COMPUTED_GOTO $clang_flags"
	gcc_flags="-DBC_NO_COMPUTED_GOTO $gcc_flags"
fi

# Setup a default compiler.
if [ "$test_with_clang" -ne 0 ]; then
	defcc="clang"
elif [ "$test_with_gcc" -ne 0 ]; then
	defcc="gcc"
else
	defcc="c99"
fi

export ASAN_OPTIONS="abort_on_error=1,allocator_may_return_null=1:strict_string_checks=1:detect_stack_use_after_return=1:check_initialization_order=1:strict_init_order=1:detect_invalid_pointer_pairs=2"
export UBSAN_OPTIONS="print_stack_trace=1,silence_unsigned_overflow=1"

build "$debug -std=c99" "$defcc" "-g" "1" "$bits"

header "Running math library under --standard"

# Make sure the math library is POSIX compliant.
printf 'quit\n' | bin/bc -ls

do_make clean_tests

# Run the clang build sets.
if [ "$test_with_clang" -ne 0 ]; then
	build_set "clang" "$run_tests"
fi

# Run the gcc build sets.
if [ "$test_with_gcc" -ne 0 ]; then
	build_set "gcc" "$run_tests"
fi

if [ "$run_tests" -ne 0 ]; then

	build "$release" "$defcc" "-O3" "1" "$bits"

	# Run karatsuba.
	if [ "$test_karatsuba" -ne 0 ]; then
		karatsuba
	fi

	# Valgrind.
	if [ "$run_valgrind" -ne 0 -a "$test_with_gcc" -ne 0 ]; then
		vg
	fi

	printf '\n'
	printf 'Tests successful.\n'

fi
