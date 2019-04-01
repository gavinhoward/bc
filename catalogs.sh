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

script="$0"
scriptdir=$(dirname "$script")

. "$scriptdir/functions.sh"

localedir="$scriptdir/locales"

for file in $localedir/*.msg; do

	base=$(basename "$file")
	name=$(echo "$base" | cut -f 1 -d '.')
	f="$localedir/catalogs/$name.cat"

	if [ -L "$file" ]; then
		link=$(readlink "$file")
		link=$(echo "$link" | cut -f 1 -d '.')
		rm -rf "$localedir/catalogs/$name.cat"
		ln -s "$link.cat" "$localedir/catalogs/$name.cat"
	else
		gencat "$f" "$file"
	fi

done
