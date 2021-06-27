#! /bin/sh
#
# SPDX-License-Identifier: BSD-2-Clause
#
# Copyright (c) 2018-2021 Gavin D. Howard and contributors.
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

# Just print the usage and exit with an error.
usage() {
	printf 'usage: %s [-a] dir benchmark\n' "$0" 1>&2
	exit 1
}

script="$0"
scriptdir=$(dirname "$script")

if [ "$#" -lt 2 ]; then
	usage
fi

d="$1"
shift

benchmark="$1"
shift

cd "$scriptdir/.."

if [ "$d" = "bc" ]; then
	opts="-lq"
	halt="halt"
else
	opts="-x"
	halt="q"
fi

if [ ! -f "./benchmarks/$d/$benchmark.txt" ]; then
	printf 'Benchmarking generation of benchmarks/%s/%s.txt...\n' "$d" "$benchmark"
	printf '%s\n' "$halt" | /usr/bin/time -v bin/$d $opts "./benchmarks/$d/$benchmark.$d" \
		> "./benchmarks/$d/$benchmark.txt"
fi

printf 'Benchmarking benchmarks/%s/%s.txt...\n' "$d" "$benchmark"
printf '%s\n' "$halt" | /usr/bin/time -v bin/$d $opts "./benchmarks/$d/$benchmark.txt" \
	> /dev/null
