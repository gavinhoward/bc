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
	printf "usage: %s bin_dir link\n" "$0" 1>&2
	exit 1
}

test "$#" -gt 1 || usage

bindir="$1"
shift

link="$1"
shift

cd "$bindir"

for exe in ./*; do

	if [ ! -L "$exe" ]; then

		base=$(basename "$exe")
		ext="${base##*.}"

		if [ "$ext" != "$base" ]; then
			name="$link.$ext"
		else
			name="$link"
		fi

		ln -fs "$exe" "./$name"
	fi

done
