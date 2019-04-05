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
	printf "usage: %s install_dir main_exec\n" "$0" 1>&2
	exit 1
}

script="$0"
scriptdir=$(dirname "$script")

. "$scriptdir/functions.sh"

INSTALL="$scriptdir/safe-install.sh"

test "$#" -ge 2 || usage

install_dir="$1"
shift

main_exec="$1"
shift

locales_dir="$scriptdir/locales"

for file in $locales_dir/*.msg; do

	base=$(basename "$file")
	name=$(removeext "$base")
	d="$install_dir/$name/LC_MESSAGES"
	loc="$install_dir/$name/LC_MESSAGES/$main_exec.cat"

	mkdir -p "$d"

	if [ -L "$file" ]; then
		link=$(readlink "$file")
		linkname=$(removeext "$link")
		"$INSTALL" -Dlm 755 "../../$linkname/LC_MESSAGES/$main_exec.cat" "$loc"
	else
		err_msgs=$(gencat "$loc" "$file" 2>&1)
		if [ "$err_msgs" != "" ]; then
			printf '\nWarning: gencat produced the following errors:\n\n%s\n\n' "$err_msgs"
		fi
	fi

done
