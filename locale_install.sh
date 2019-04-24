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
	printf "usage: %s NLSPATH main_exec [DESTDIR]\n" "$0" 1>&2
	exit 1
}

gencatfile() {

	_gencatfile_loc="$1"
	shift

	_gencatfile_file="$1"
	shift

	mkdir -p $(dirname "$_gencatfile_loc")
	gencat "$_gencatfile_loc" "$_gencatfile_file" > /dev/null 2>&1
}

localeexists() {

	_localeexists_locales="$1"
	shift

	_localeexists_locale="$1"
	shift

	_localeexists_destdir="$1"
	shift

	if [ "$_localeexists_destdir" != "" ]; then
		_localeexists_char="@"
		_localeexists_locale="${_localeexists_locale%%_localeexists_char*}"
		_localeexists_char="."
		_localeexists_locale="${_localeexists_locale##*$_localeexists_char}"
	fi

	test ! -z "${_localeexists_locales##*$_localeexists_locale*}"
	return $?
}

script="$0"
scriptdir=$(dirname "$script")

. "$scriptdir/functions.sh"

test "$#" -ge 2 || usage

nlspath="$1"
shift

main_exec="$1"
shift

if [ "$#" -ge 1 ]; then
	destdir="$1"
	shift
else
	destdir=""
fi

"$scriptdir/locale_uninstall.sh" "$nlspath" "$main_exec" "$destdir"

locales_dir="$scriptdir/locales"

# What this does is if installing to a package, it installs all locales that
# match supported charsets instead of installing all directly supported locales.
if [ "$destdir" = "" ]; then
	locales=$(locale -a)
else
	locales=$(locale -m)
fi

for file in $locales_dir/*.msg; do

	locale=$(basename "$file" ".msg")
	loc=$(gen_nlspath "$destdir/$nlspath" "$locale" "$main_exec")

	localeexists "$locales" "$locale" "$destdir"
	err="$?"

	if [ "$err" -eq 0 ]; then
		continue
	fi

	if [ -L "$file" ]; then
		continue
	fi

	gencatfile "$loc" "$file"

done

for file in $locales_dir/*.msg; do

	locale=$(basename "$file" ".msg")
	loc=$(gen_nlspath "$destdir/$nlspath" "$locale" "$main_exec")

	localeexists "$locales" "$locale" "$destdir"
	err="$?"

	if [ "$err" -eq 0 ]; then
		continue
	fi

	mkdir -p $(dirname "$loc")

	if [ -L "$file" ]; then

		link=$(readlink "$file")
		linkdir=$(dirname "$file")
		locale=$(basename "$link" .msg)
		linksrc=$(gen_nlspath "$nlspath" "$locale" "$main_exec")

		if [ ! -f "$destdir/$linksrc" ]; then
			gencatfile "$destdir/$linksrc" "$linkdir/$link"
		fi

		ln -s "$linksrc" "$loc"
	fi

done
