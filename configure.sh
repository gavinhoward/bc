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

	printf 'usage: %s [-bD|-dB|-c] [-EgGhHRS] [-O OPT_LEVEL] [-k KARATSUBA_LEN]\n' "$0"
	printf '\n'
	printf '    -b\n'
	printf '        Build bc only. It is an error if "-d" or "-B" are specified too.\n'
	printf '    -B\n'
	printf '        Disable bc. It is an error if "-b" or "-D" are specified too.\n'
	printf '    -c\n'
	printf '        Generate test coverage code. Requires gcov and regcovr.\n'
	printf '        It is an error if either "-b" ("-D") or "-d" ("-B") is specified.\n'
	printf '        Requires a compiler that use gcc-compatible coverage options\n'
	printf '    -d\n'
	printf '        Build dc only. It is an error if "-b" is specified too.\n'
	printf '    -D\n'
	printf '        Disable dc. It is an error if "-d" or "-B" are specified too.\n'
	printf '    -E\n'
	printf '        Disable extra math. This includes: "$" operator (truncate to integer),\n'
	printf '        "@" operator (set number of decimal places), and r(x, p) (rounding\n'
	printf '        function). Additionally, this option disables the extra printing\n'
	printf '        functions in the math library.\n'
	printf '    -g\n'
	printf '        Build in debug mode. Adds the "-g" flag, and if there are no\n'
	printf '        other CFLAGS, and "-O" was not given, this also adds the "-O0"\n'
	printf '        flag. If this flag is *not* given, "-DNDEBUG" is added to CPPFLAGS\n'
	printf '        and a strip flag is added to the link stage.\n'
	printf '    -G\n'
	printf '        Disable generating tests. This is for platforms that do not have a\n'
	printf '        GNU bc-compatible bc to generate tests.\n'
	printf '    -h\n'
	printf '        Print this help message and exit.\n'
	printf '    -H\n'
	printf '        Disable history.\n'
	printf '    -k KARATSUBA_LEN\n'
	printf '        Set the karatsuba length to KARATSUBA_LEN (default is 32).\n'
	printf '        It is an error if KARATSUBA_LEN is not a number or is less than 2.\n'
	printf '        It is an error if \"-m\" is specified too.\n'
	printf '    -O OPT_LEVEL\n'
	printf '        Set the optimization level. This can also be included in the CFLAGS,\n'
	printf '        but it is provided, so maintainers can build optimized debug builds.\n'
	printf '        This is passed through to the compiler, so it must be supported.\n'
	printf '    -R\n'
	printf '        Disable the array references extension. This feature is an\n'
	printf '        undocumented feature of the GNU bc, but this bc supports it.\n'
	printf '        Additionally, since this feature is only available to bc,\n'
	printf '        specifying "-d" ("-B") implies this option.\n'
	printf '    -S\n'
	printf '        Disable signal handling. On by default.\n'
	printf '\n'
	printf 'In addition, the following environment variables are used:\n'
	printf '\n'
	printf '    CC        C compiler. Must be compatible with POSIX c99.\n'
	printf '    HOSTCC    Host C compiler. Must be compatible with POSIX c99.\n'
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

	if [ "$#" -lt 3 ]; then
		err_exit "Invalid number of args to $0"
	fi

	contents="$1"
	shift

	filedir="$1"
	shift

	typ="$1"
	shift

	# If there is an extra argument, and it
	# is zero, we keep the file lists empty.
	if [ "$#" -gt 0 ]; then
		use="$1"
	else
		use="1"
	fi

	needle_src="${typ}SRC"
	needle_obj="${typ}OBJ"
	needle_gcda="${typ}GCDA"
	needle_gcno="${typ}GCNO"

	if [ "$use" -ne 0 ]; then

		replacement=$(ls $filedir/*.c | tr '\n' ' ')
		contents=$(replace "$contents" "$needle_src" "$replacement")

		replacement=$(replace_exts "$replacement" "c" "o")
		contents=$(replace "$contents" "$needle_obj" "$replacement")

		replacement=$(replace_exts "$replacement" "o" "gcda")
		contents=$(replace "$contents" "$needle_gcda" "$replacement")

		replacement=$(replace_exts "$replacement" "gcda" "gcno")
		contents=$(replace "$contents" "$needle_gcno" "$replacement")

	else
		contents=$(replace "$contents" "$needle_src" "")
		contents=$(replace "$contents" "$needle_obj" "")
		contents=$(replace "$contents" "$needle_gcda" "")
		contents=$(replace "$contents" "$needle_gcno" "")
	fi

	printf '%s\n' "$contents"
}

bc_only=0
dc_only=0
coverage=0
karatsuba_len=32
debug=0
signals=1
hist=1
refs=1
extra_math=1
optimization=""
generate_tests=1

while getopts "bBcdDEgGhHk:O:RS" opt; do

	case "$opt" in
		b) bc_only=1 ;;
		B) dc_only=1 ;;
		c) coverage=1 ;;
		d) dc_only=1 ;;
		D) bc_only=1 ;;
		E) extra_math=0 ;;
		g) debug=1 ;;
		G) generate_tests=0 ;;
		h) usage ;;
		H) hist=0 ;;
		k) karatsuba_len="$OPTARG" ;;
		O) optimization="$OPTARG" ;;
		R) refs=0 ;;
		S) signals=0 ;;
		?) usage "Invalid option" ;;
	esac

done

if [ "$bc_only" -eq 1 -a "$dc_only" -eq 1 ]; then
	usage "Can only specify one of -b(-D) or -d(-B)"
fi

case $karatsuba_len in
	(*[!0-9]*|'') usage "KARATSUBA_LEN is not a number" ;;
	(*) ;;
esac

if [ "$karatsuba_len" -lt 2 ]; then
	usage "KARATSUBA_LEN is less than 2"
fi

set -e

script="$0"

scriptdir=$(dirname "$script")

link="@printf 'No link necessary\\\\n'"
main_exec="BC_EXEC"

bc_test="@tests/all.sh bc $extra_math $refs $generate_tests"
dc_test="@tests/all.sh dc $extra_math $refs $generate_tests"

timeconst="@tests/bc/timeconst.sh"

# In order to have cleanup at exit, we need to be in
# debug mode, so don't run valgrind without that.
if [ "$debug" -ne 0 ]; then
	vg_bc_test="@tests/all.sh bc $extra_math $refs $generate_tests valgrind \$(VALGRIND_ARGS) \$(BC_EXEC)"
	vg_dc_test="@tests/all.sh dc $extra_math $refs $generate_tests valgrind \$(VALGRIND_ARGS) \$(DC_EXEC)"
else
	vg_bc_test="@printf 'Cannot run valgrind without debug flags\\\\n'"
	vg_dc_test="@printf 'Cannot run valgrind without debug flags\\\\n'"
fi

karatsuba="@printf 'karatsuba cannot be run because one of bc or dc is not built\\\\n'"
karatsuba_test="@printf 'karatsuba cannot be run because one of bc or dc is not built\\\\n'"

bc_lib="\$(GEN_DIR)/lib.o"
bc_help="\$(GEN_DIR)/bc_help.o"
dc_help="\$(GEN_DIR)/dc_help.o"

if [ "$bc_only" -eq 1 ]; then

	bc=1
	dc=0

	dc_help=""

	executables="bc"

	dc_test="@printf 'No dc tests to run\\\\n'"
	vg_dc_test="@printf 'No dc tests to run\\\\n'"

elif [ "$dc_only" -eq 1 ]; then

	bc=0
	dc=1

	bc_lib=""
	bc_help=""

	printf 'dc only; disabling references...\n'
	refs=0

	executables="dc"

	main_exec="DC_EXEC"

	bc_test="@printf 'No bc tests to run\\\\n'"
	vg_bc_test="@printf 'No bc tests to run\\\\n'"

	timeconst="@printf 'timeconst cannot be run because bc is not built\\\\n'"

else

	bc=1
	dc=1

	executables="bc and dc"

	link="\$(LINK) \$(BIN) \$(DC)"

	karatsuba="@\$(KARATSUBA)"
	karatsuba_test="@\$(KARATSUBA) 100 \$(BC_EXEC)"

fi

if [ "$debug" -eq 1 ]; then

	if [ "$CFLAGS" = "" -a "$optimization" = "" ]; then
		CFLAGS="-O0"
	fi

	CFLAGS="$CFLAGS -g"

else
	CPPFLAGS="$CPPFLAGS -DNDEBUG"
	if [ "$bc" -ne 0 -a "$dc" -ne 0 ]; then
		link="$link 1"
	fi
fi

if [ "$optimization" != "" ]; then
	CFLAGS="$CFLAGS -O$optimization"
fi

if [ "$coverage" -eq 1 ]; then

	if [ "$bc_only" -eq 1 -o "$dc_only" -eq 1 ]; then
		usage "Can only specify -c without -b or -d"
	fi

	CFLAGS="$CFLAGS -fprofile-arcs -ftest-coverage -g -O0"
	CPPFLAGS="$CPPFLAGS -DNDEBUG"

	COVERAGE="@gcov -pabcdf \$(GCDA) \$(BC_GCDA) \$(DC_GCDA)"
	COVERAGE="$COVERAGE;\$(RM) -f \$(GEN)*.gc*"
	COVERAGE="$COVERAGE;gcovr --html-details --output index.html"
	COVERAGE_PREREQS=" test_all"

else
	COVERAGE="@printf 'Coverage not generated\\\\n'"
	COVERAGE_PREREQS=""
fi

if [ "$PREFIX" = "" ]; then
	PREFIX="/usr/local"
fi

if [ "$CC" = "" ]; then
	CC="c99"
fi

if [ "$HOSTCC" = "" ]; then
	HOSTCC="$CC"
fi

if [ "$hist" -eq 1 ]; then

	set +e

	printf 'Testing history...\n'

	flags="-DBC_ENABLE_HISTORY=1 -DBC_ENABLED=$bc -DDC_ENABLED=$dc -DBC_ENABLE_SIGNALS=$signals -I./include/"

	"$CC" $CFLAGS $flags -c "src/history/history.c" > /dev/null 2>&1

	err="$?"

	rm -rf "$scriptdir/history.o"

	# If this errors, it is probably because of building on Windows,
	# and history is not supported on Windows, so disable it.
	if [ "$err" -ne 0 ]; then
		printf 'History does not work.\n'
		printf 'Disabling history...\n'
		hist=0
	else
		printf 'History works.\n'
	fi

	set -e

fi

if [ "$extra_math" -eq 1 -a "$bc" -ne 0 ]; then
	BC_LIB2_O="\$(GEN_DIR)/lib2.o"
else
	BC_LIB2_O=""
fi

contents=$(cat "$scriptdir/Makefile.in")

needle="WARNING"
replacement='*** WARNING: Autogenerated from Makefile.in. DO NOT MODIFY ***'

contents=$(replace "$contents" "$needle" "$replacement")

contents=$(gen_file_lists "$contents" "$scriptdir/src" "")
contents=$(gen_file_lists "$contents" "$scriptdir/src/bc" "BC_" "$bc")
contents=$(gen_file_lists "$contents" "$scriptdir/src/dc" "DC_" "$dc")
contents=$(gen_file_lists "$contents" "$scriptdir/src/history" "HISTORY_" "$hist")

contents=$(replace "$contents" "BC_ENABLED" "$bc")
contents=$(replace "$contents" "DC_ENABLED" "$dc")
contents=$(replace "$contents" "LINK" "$link")

contents=$(replace "$contents" "SIGNALS" "$signals")
contents=$(replace "$contents" "HISTORY" "$hist")
contents=$(replace "$contents" "REFERENCES" "$refs")
contents=$(replace "$contents" "EXTRA_MATH" "$extra_math")
contents=$(replace "$contents" "BC_LIB_O" "$bc_lib")
contents=$(replace "$contents" "BC_HELP_O" "$bc_help")
contents=$(replace "$contents" "DC_HELP_O" "$dc_help")
contents=$(replace "$contents" "BC_LIB2_O" "$BC_LIB2_O")
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

contents=$(replace "$contents" "TIMECONST" "$timeconst")

contents=$(replace "$contents" "KARATSUBA" "$karatsuba")
contents=$(replace "$contents" "KARATSUBA_TEST" "$karatsuba_test")

contents=$(replace "$contents" "GEN_EMU" "$GEN_EMU")

printf '%s\n' "$contents" > "$scriptdir/Makefile"

cd "$scriptdir"

make clean > /dev/null
