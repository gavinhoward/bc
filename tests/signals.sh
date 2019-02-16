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

set -e

script="$0"

testdir=$(dirname "${script}")

if [ "$#" -eq 0 ]; then
	printf 'usage: %s dir [exec args...]\n' "$script"
	exit 1
else
	d="$1"
	shift
fi

if [ "$#" -gt 0 ]; then
	exe="$1"
	shift
else
	exe="$testdir/../bin/$d"
fi

name="$testdir/$d/signals.txt"

printf 'Running %s signal tests...' "$d"

if [ "$d" = "bc" ]; then
	options="-q"
else
	options="-x"
fi

"$exe" "$options" 2>&1 > /dev/null &
chpid=$!

kill -s INT "$chpid"
kill -s TERM "$chpid"

set +e

kill -s KILL "$chpid"

"$exe" "$options" "$name" 2>&1 > /dev/null &
chpid=$!

kill -s INT "$chpid"
kill -s TERM "$chpid"

set +e

kill -s KILL "$chpid"

"$exe" -l "$options" "$name" 2>&1 > /dev/null &
chpid=$!

kill -s INT "$chpid"
kill -s TERM "$chpid"

set +e

kill -s KILL "$chpid"
