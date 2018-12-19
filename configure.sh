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

	if [ $# -gt 0 ]; then

		val=1

		printf "%s\n\n" "$1"

	else
		val=0
	fi

	printf 'usage: %s [-b|-d|-c] [-hHS] [-g(-m|-r)|-N] [-k KARATSUBA_LEN]\n' "$0"
	printf '\n'
	printf '    -b\n'
	printf '        Build bc only. It is an error if "-d" is specified too.\n'
	printf '    -c\n'
	printf '        Generate test coverage code. Requires gcov and regcovr.\n'
	printf '        It is an error if either "-b" or "-d" is specified.\n'
	printf '        Implies "-N".\n'
	printf '    -d\n'
	printf '        Build dc only. It is an error if "-b" is specified too.\n'
	printf '    -g\n'
	printf '        Build in debug mode.\n'
	printf '    -h\n'
	printf '        Print this help message and exit.\n'
	printf '    -H\n'
	printf '        Disable history (currently not implemented).\n'
	printf '    -k KARATSUBA_LEN\n'
	printf '        Set the karatsuba length to KARATSUBA_LEN (default is 32).\n'
	printf '        It is an error if KARATSUBA_LEN is not a number or is less than 2.\n'
	printf '    -m\n'
	printf '        Enable minimum-size release flags (-Os -DNDEBUG -s).\n'
	printf '        It is an error if \"-r\" or \"-g\" are specified too.\n'
	printf '    -N\n'
	printf '        Disable default CFLAGS. It is an error to specify this option\n'
	printf '        with any of \"-g\", \"-m\", or \"-r\".\n'
	printf '    -r\n'
	printf '        Enable default release flags (-O3 -DNDEBUG -s). On by default.\n'
	printf '        If given with \"-g\", a debuggable release will be built.\n'
	printf '        It is an error if \"-m\" is specified too.\n'
	printf '    -S\n'
	printf '        Disable signal handling. On by default.\n'
	printf '\n'
	printf 'In addition, the following environment variables are used:\n'
	printf '\n'
	printf '    CC        C compiler. Must be compatible with gcc or clang options.\n'
	printf '    HOSTCC    Host C compiler.\n'
	printf '    CFLAGS    C compiler flags. You can use this for extra optimization flags.\n'
	printf '    CPPFLAGS  C preprocessor flags.\n'
	printf '    LDFLAGS   Linker flags.\n'
	printf '    PREFIX    the prefix to install to. Default is /usr/local.\n'
	printf '              If PREFIX is \"/usr\", install path will be \"/usr/bin\".\n'
	printf '    DESTDIR   For package creation.\n'
	printf '    GEN_EMU   Emulator to run string generator code under\n'
	printf '              (leave empty if not necessary).\n'
	exit "$val"
}

err_exit() {

	if [ "$#" -ne 1 ]; then
		printf 'Invalid number of args to err_exit\n'
		exit 1
	fi

	printf '%s\n' "$1"
	exit 1
}

replace_ext() {

	if [ "$#" -ne 3 ]; then
		err_exit "Invalid number of args to $0"
	fi

	file="$1"
	ext1="$2"
	ext2="$3"

	result=$(printf "$file" | sed -e "s@\.$ext1@\.$ext2@")

	printf '%s\n' "$result"
}

replace_exts() {

	if [ "$#" -ne 3 ]; then
		err_exit "Invalid number of args to $0"
	fi

	files="$1"
	ext1="$2"
	ext2="$3"

	for file in $files; do
		new_name=$(replace_ext "$file" "$ext1" "$ext2")
		result="$result $new_name"
	done

	printf '%s\n' "$result"
}

replace() {

	if [ "$#" -ne 3 ]; then
		err_exit "Invalid number of args to $0"
	fi

	str="$1"
	needle="$2"
	replacement="$3"

	result=$(printf '%s' "$str" | sed -e "s!%%$needle%%!$replacement!g")

	printf '%s\n' "$result"
}

gen_file_lists() {

	if [ "$#" -ne 3 ]; then
		err_exit "Invalid number of args to $0"
	fi

	contents="$1"
	filedir="$2"
	typ="$3"

	needle="${typ}SRC"
	replacement=$(ls $filedir/*.c | tr '\n' ' ')

	contents=$(replace "$contents" "$needle" "$replacement")

	needle="${typ}OBJ"
	replacement=$(replace_exts "$replacement" "c" "o")

	contents=$(replace "$contents" "$needle" "$replacement")

	needle="${typ}GCDA"
	replacement=$(replace_exts "$replacement" "o" "gcda")

	contents=$(replace "$contents" "$needle" "$replacement")

	needle="${typ}GCNO"
	replacement=$(replace_exts "$replacement" "gcda" "gcno")

	contents=$(replace "$contents" "$needle" "$replacement")

	printf '%s\n' "$contents"
}

bc_only=0
dc_only=0
coverage=0
karatsuba_len=32
min_size=0
debug=0
release=0
signals=1
hist=1
none=0

while getopts "bcdghHk:mNrS" opt; do

	case "$opt" in
		b) bc_only=1 ;;
		c) coverage=1 ; none=1 ;;
		d) dc_only=1 ;;
		g) debug=1 ;;
		h) usage ;;
		H) hist=0 ;;
		k) karatsuba_len="$OPTARG" ;;
		m) min_size=1 ;;
		N) none=1 ;;
		r) release=1 ;;
		S) signals=0 ;;
		t) hist=1 ;;
		?) usage "Invalid option" ;;
	esac

done

if [ "$bc_only" -eq 1 -a "$dc_only" -eq 1 ]; then
	usage "Can only specify one of -b or -d"
fi

case $karatsuba_len in
	(*[!0-9]*|'') usage "KARATSUBA_LEN is not a number" ;;
	(*) ;;
esac

if [ "$karatsuba_len" -lt 2 ]; then
	usage "KARATSUBA_LEN is less than 2"
fi

if [ "$release" -eq 1 -a "$min_size" -eq 1 ]; then
	usage "Can only specify one of -r or -m"
fi

if [ "$none" -eq 1 ]; then

	if [ "$release" -eq 1 -o "$min_size" -eq 1 -o "$debug" -eq 1 ]; then
		usage "Cannot specify -N (or -c) with -r, -m, or -g"
	fi

fi

set -e

script="$0"

scriptdir=$(dirname "$script")

contents=$(cat "$scriptdir/Makefile.in")

needle="WARNING"
replacement='# *** WARNING: Autogenerated from Makefile.in. DO NOT MODIFY ***\n#'

contents=$(replace "$contents" "$needle" "$replacement")

contents=$(gen_file_lists "$contents" "$scriptdir/src" "")
contents=$(gen_file_lists "$contents" "$scriptdir/src/bc" "BC_")
contents=$(gen_file_lists "$contents" "$scriptdir/src/dc" "DC_")

link='@printf "No link necessary\\\\n"'
main_exec="BC_EXEC"

bc_test="tests/all.sh bc"
dc_test="tests/all.sh dc"

timeconst="tests/bc/timeconst.sh"

# In order to have cleanup at exit, we need to be in
# debug mode, so don't run valgrind without that.
if [ "$debug" -ne 0 ]; then
	vg_bc_test="tests/all.sh bc valgrind \$(VALGRIND_ARGS) \$(BC_EXEC)"
	vg_dc_test="tests/all.sh dc valgrind \$(VALGRIND_ARGS) \$(DC_EXEC)"
	timeconst_vg='printf "100\\\\n" | valgrind \$(VALGRIND_ARGS) \$(BC_EXEC) tests/bc/scripts/timeconst.bc'
else
	vg_bc_test='@printf "Cannot run valgrind without debug flags\\\\n"'
	vg_dc_test='@printf "Cannot run valgrind without debug flags\\\\n"'
	timeconst_vg='@printf "Cannot run valgrind without debug flags\\\\n"'
fi

karatsuba='@printf "karatsuba cannot be run because one of bc or dc is not built\\\\n"'
karatsuba_test='@printf "karatsuba cannot be run because one of bc or dc is not built\\\\n"'

if [ "$bc_only" -eq 1 ]; then

	bc=1
	dc=0

	executables="bc"

	dc_test='@printf "No dc tests to run\\\\n"'
	vg_dc_test='@printf "No dc tests to run\\\\n"'

elif [ "$dc_only" -eq 1 ]; then

	bc=0
	dc=1

	executables="dc"

	main_exec="DC_EXEC"

	bc_test='@printf "No bc tests to run\\\\n"'
	vg_bc_test='@printf "No bc tests to run\\\\n"'

	timeconst='@printf "timeconst cannot be run because bc is not built\\\\n"'
	timeconst_vg='@printf "timeconst cannot be run because bc is not built\\\\n"'

else

	bc=1
	dc=1

	executables="bc and dc"

	link="\$(LINK) \$(BIN) \$(DC)"

	karatsuba="\$(KARATSUBA)"
	karatsuba_test="\$(KARATSUBA) 100 \$(BC_EXEC)"

fi

if [ "$debug" -eq 1 ]; then

	CFLAGS="$CFLAGS -g -fno-omit-frame-pointer"

	if [ "$release" -ne 1 -a "$min_size" -ne 1 ]; then
		CFLAGS="$CFLAGS -O0"
	fi
else
	CPPFLAGS="$CPPFLAGS -DNDEBUG"
	LDFLAGS="$LDFLAGS -s"
fi

if [ "$release" -eq 1 -o "$none" -eq 0 ]; then
	if [ "$debug" -ne 1 ]; then
		CFLAGS="$CFLAGS -O3"
	fi
fi

if [ "$min_size" -eq 1 ]; then
	CFLAGS="$CFLAGS -Os"
fi

if [ "$coverage" -eq 1 ]; then

	if [ "$bc_only" -eq 1 -o "$dc_only" -eq 1 ]; then
		usage "Can only specify -c without -b or -d"
	fi

	CFLAGS="$CFLAGS -fprofile-arcs -ftest-coverage -g -O0"
	CPPFLAGS="$CPPFLAGS -DNDEBUG"

	COVERAGE="gcov -pabcdf \$(GCDA) \$(BC_GCDA) \$(DC_GCDA)"
	COVERAGE="$COVERAGE;\$(RM) -f \$(GEN)*.gc*"
	COVERAGE="$COVERAGE;regcovr --html-details --output index.html"
	COVERAGE_PREREQS="all test_all"

else

	COVERAGE='@printf "Coverage not generated\\\\n"'
	COVERAGE_PREREQS=""

fi

if [ "$PREFIX" = "" ]; then
	PREFIX="/usr/local"
fi

gcc="gcc"
clang="clang"

if [ "$CC" = "" ]; then
	CC="c99"
fi

if [ "$HOSTCC" = "" ]; then
	HOSTCC="$CC"
fi

contents=$(replace "$contents" "BC_ENABLED" "$bc")
contents=$(replace "$contents" "DC_ENABLED" "$dc")
contents=$(replace "$contents" "LINK" "$link")

contents=$(replace "$contents" "SIGNALS" "$signals")
contents=$(replace "$contents" "HISTORY" "$hist")
contents=$(replace "$contents" "KARATSUBA_LEN" "$karatsuba_len")

contents=$(replace "$contents" "PREFIX" "$PREFIX")
contents=$(replace "$contents" "DESTDIR" "$DESTDIR")
contents=$(replace "$contents" "CFLAGS" "$CFLAGS")
contents=$(replace "$contents" "CPPFLAGS" "$CPPFLAGS")
contents=$(replace "$contents" "LDFLAGS" "$LDFLAGS")
contents=$(replace "$contents" "CC" "$CC")
contents=$(replace "$contents" "HOSTCC" "$HOSTCC")
contents=$(replace "$contents" "COVERAGE" "$COVERAGE")
contents=$(replace "$contents" "COVERAGE_PREREQS" "$COVERAGE_PREREQS")

contents=$(replace "$contents" "EXECUTABLES" "$executables")
contents=$(replace "$contents" "MAIN_EXEC" "$main_exec")

contents=$(replace "$contents" "BC_TEST" "$bc_test")
contents=$(replace "$contents" "DC_TEST" "$dc_test")

contents=$(replace "$contents" "VG_BC_TEST" "$vg_bc_test")
contents=$(replace "$contents" "VG_DC_TEST" "$vg_dc_test")

contents=$(replace "$contents" "TIMECONST_TARGET" "$timeconst_target")
contents=$(replace "$contents" "TIMECONST" "$timeconst")

contents=$(replace "$contents" "TIMECONST_VG_TARGET" "$timeconst_vg_target")
contents=$(replace "$contents" "TIMECONST_VG" "$timeconst_vg")

contents=$(replace "$contents" "KARATSUBA" "$karatsuba")
contents=$(replace "$contents" "KARATSUBA_TEST" "$karatsuba_test")

printf '%s\n' "$contents" > "$scriptdir/Makefile"

cd "$scriptdir"

make clean > /dev/null

printf 'Testing C compilers...\n'

libname=$(make libcname)

set +e

make "$libname" > /dev/null

err="$?"

if [ "$err" -ne 0 ]; then
	usage "\nHOSTCC ($HOSTCC) is not compatible with gcc/clang options"
fi

make > /dev/null

err="$?"

if [ "$err" -ne 0 ]; then
	usage "\nCC ($CC) is not compatible with gcc/clang options"
fi
