#! /bin/sh
#
# SPDX-License-Identifier: BSD-2-Clause
#
# Copyright (c) 2018-2026 Gavin D. Howard and contributors.
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

set -e

script="$0"
testdir=$(dirname "$script")

. "$testdir/../scripts/functions.sh"

# Just print the usage and exit with an error. This can receive a message to
# print.
# @param 1  A message to print.
usage() {
	if [ $# -eq 1 ]; then
		printf '%s\n\n' "$1"
	fi
	printf 'usage: %s dir [exe [args...]]\n' "$script"
	exit 1
}

trim_output() {
	printf '%s\n' "$1" | tr -d '\r' | sed '/^$/d'
}

# Command-line processing.
if [ "$#" -lt 1 ]; then
	usage "Not enough arguments"
fi

d="$1"
shift
check_d_arg "$d"

if [ "$d" != "bc" ]; then
	printf 'Skipping non-regular streaming test for %s\n' "$d"
	exit 0
fi

if [ "$#" -gt 0 ]; then
	exe="$1"
	shift
	check_exec_arg "$exe"
else
	exe="$testdir/../bin/$d"
	check_exec_arg "$exe"
fi

if ! command -v mkfifo >/dev/null 2>&1; then
	printf 'Skipping non-regular streaming test: mkfifo unavailable\n'
	exit 0
fi

if ! command -v bash >/dev/null 2>&1; then
	printf 'Skipping non-regular streaming test: bash unavailable\n'
	exit 0
fi

if ! command -v mktemp >/dev/null 2>&1; then
	printf 'Skipping non-regular streaming test: mktemp unavailable\n'
	exit 0
fi

# I use these, so unset them to make the tests work.
unset BC_ENV_ARGS
unset BC_LINE_LENGTH
unset DC_ENV_ARGS
unset DC_LINE_LENGTH

tmpdir=$(mktemp -d)
trap 'st=$?; rm -rf "$tmpdir"; exit "$st"' EXIT HUP INT TERM

printf 'Running %s non-regular streaming tests...' "$d"

# /dev/stdin should be read as file input and produce output.
if [ ! -r /dev/stdin ]; then
	printf 'Skipping /dev/stdin streaming case: /dev/stdin unavailable\n'
else
	stdin_out=$(printf '4+4\nquit\n' | "$exe" "$@" -q /dev/stdin 2>/dev/null)
	stdin_out=$(trim_output "$stdin_out")
	if [ "$stdin_out" != "8" ]; then
		die "$d" "failed /dev/stdin streaming case" "nonregular_streaming" 1
	fi
fi

# FIFO should be read as file input and produce output.
fifo="$tmpdir/in.fifo"
mkfifo "$fifo"

(
	printf '2+2\nquit\n' > "$fifo"
) &
writer="$!"

fifo_out=$("$exe" "$@" -q "$fifo" </dev/null 2>/dev/null)
wait "$writer" 2>/dev/null || true
fifo_out=$(trim_output "$fifo_out")
if [ "$fifo_out" != "4" ]; then
	die "$d" "failed FIFO streaming case" "nonregular_streaming" 1
fi

# Process substitution should be read as file input and produce output.
proc_out=$(BC_TEST_EXE="$exe" bash -lc '"$BC_TEST_EXE" -q <(printf "3+3\nquit\n")' 2>/dev/null)
proc_out=$(trim_output "$proc_out")
if [ "$proc_out" != "6" ]; then
	die "$d" "failed process substitution streaming case" "nonregular_streaming" 1
fi

printf 'pass\n'
