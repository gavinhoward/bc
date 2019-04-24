#!/bin/sh

export LANG=C
export LC_CTYPE=C

progname=${0##*/}

if [ $# -lt 4 ]; then
	echo "usage: $progname input output name header [label [define [remove_tabs]]]"
	exit 1
fi

input="$1"
output="$2"
name="$3"
header="$4"
label="$5"
define="$6"
remove_tabs="$7"

exec < "$input"
exec > "$output"

if [ -n "$label" ]; then
	nameline="const char *${label} = \"${input}\";"
fi

if [ -n "$define" ]; then
	condstart="#if ${define}"
	condend="#endif"
fi

if [ -n "$remove_tabs" ]; then
	if [ "$remove_tabs" -ne 0 ]; then
		remtabsexpr='s:	::g;'
	fi
fi

cat<<EOF
// Licensed under the 2-clause BSD license.
// *** AUTOMATICALLY GENERATED FROM ${input}. DO NOT MODIFY. ***

${condstart}
#include <${header}>

$nameline

const char ${name}[] =
$(sed -e "$remtabsexpr " -e '1,/^$/d; s:\\n:\\\\n:g; s:":\\":g; s:^:":; s:$:\\n":')
;
${condend}
EOF
