#! /bin/sh
#
# SPDX-License-Identifier: BSD-2-Clause
#
# Copyright (c) 2018-2024 Gavin D. Howard and contributors.
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

# Just print the usage and exit with an error. This can receive a message to
# print.
# @param 1  A message to print.
usage() {
	if [ $# -eq 1 ]; then
		printf '%s\n\n' "$1"
	fi
	printf 'usage: %s [-a] [afl_compiler]\n' "$0"
	printf '\n'
	printf '       If -a is given, then an ASan ready build is created.\n'
	printf '       Otherwise, a normal fuzz build is created.\n'
	printf '       The ASan-ready build is for running under\n'
	printf '       `tests/afl.py --asan`, which checks that there were no\n'
	printf '       memory errors in any path found by the fuzzer.\n'
	printf '       It might also be useful to run scripts/randmath.py on an\n'
	printf '       ASan-ready binary.\n'
	exit 1
}

script="$0"
scriptdir=$(dirname "$script")

. "$scriptdir/functions.sh"

asan=0

# Process command-line arguments.
while getopts "a" opt; do

	case "$opt" in
		a) asan=1 ;;
		?) usage "Invalid option: $opt" ;;
	esac

done
shift $(($OPTIND - 1))

if [ $# -lt 1 ]; then
	CC=afl-clang-lto
else
	CC="$1"
fi

# We want this for extra sensitive crashing
AFL_HARDEN=1

cd "$scriptdir/.."

set -e

CFLAGS="-flto -fstack-protector-all -fsanitize=shadow-call-stack -ffixed-x18 -fsanitize=cfi -fvisibility=hidden"

if [ "$asan" -ne 0 ]; then
	CFLAGS="$CFLAGS -fsanitize=address"
fi

# These are to get better instrumentation.
export AFL_LLVM_LAF_SPLIT_SWITCHES=1
export AFL_LLVM_LAF_TRANSFORM_COMPARES=1
export AFL_LLVM_LAF_SPLIT_COMPARES=1
export AFL_LLVM_LTO_CALLER=1
export AFL_LLVM_LTO_CALLER_DEPTH=5

# We want a debug build because asserts are counted as crashes too.
CC="$CC" CFLAGS="$CFLAGS" ./configure.sh -gO3 -z

make -j16
