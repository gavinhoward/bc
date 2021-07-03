# Development

This document is meant for the day when I (Gavin D. Howard) get [hit by a
bus][1]. In other words, it's meant to make the [bus factor][1] a non-issue.

This document is supposed to contain all of the knowledge necessary to develop
`bc` and `dc`.

In addition, this document is meant to add to the [oral tradition of software
engineering][118], as described by Bryan Cantrill.

This document will reference other parts of the repository. That is so a lot of
the documentation can be closest to the part of the repo where it is actually
necessary.

## What Is It?

This repository contains an implementation of both [POSIX `bc`][2] and [Unix
`dc`][3].

POSIX `bc` is a standard utility required for POSIX systems. `dc` is a
historical utility that was included in early Unix and even predates both Unix
and C. They both are arbitrary-precision command-line calculators with their own
programming languages. `bc`'s language looks similar to C, with infix notation
and including functions, while `dc` uses [Reverse Polish Notation][4] and allows
the user to execute strings as though they were functions.

In addition, it is also possible to build the arbitrary-precision math as a
library, named `bcl`.

**Note**: for ease, I will refer to both programs as `bc` in this document.
However, if I say "just `bc`," I am referring to just `bc`, and if I say `dc`, I
am referring to just `dc`.

## History

This project started in January 2018 when a certain individual on IRC, hearing
that I knew how to write parsers, asked me to write a `bc` parser for his math
library. I did so. I thought about writing my own math library, but he
disparaged my programming skills and made me think that I couldn't do it.

However, he took so long to do it that I eventually decided to give it a try and
had a working math portion in two weeks. It taught me that I should not listen
to such people.

From that point, I decided to make it an extreme learning experience about how
to write quality software.

That individual's main goal had been to get his `bc` into [toybox][16], and I
managed to get my own `bc` in. I also got it in [busybox][17].

Eventually, in late 2018, I also decided to try my hand at implementing
[Karatsuba multiplication][18], an algorithm that that unnamed individual
claimed I could never implement. It took me a bit, but I did it.

This project became a passion project for me, and I continued. In mid-2019,
Stefan Eßer suggested I improve performance by putting more than 1 digit in each
section of the numbers. After I showed immaturity because of some burnout, I
implemented his suggestion, and the results were incredible.

Since that time, I have gradually been improving the `bc` as I have learned more
about things like fuzzing, [`scan-build`][19], [valgrind][20],
[AddressSanitizer][21] (and the other sanitizers), and many other things.

One of my happiest moments was when my `bc` was made the default in FreeBSD.

But since I believe in [finishing the software I write][22], I have done less
work on `bc` over time, though there are still times when I put a lot of effort
in, such as now (17 June 2021), when I am attempting to convince OpenBSD to use
my `bc`.

And that is why I am writing this document: someday, someone else is going to
want to change my code, and this document is my attempt to make it as simple as
possible.

## Values

[According to Bryan Cantrill][10], all software has values. I think he's
correct, though I [added one value for programming languages in particular][11].

However, for `bc`, his original list will do:

* Approachability
* Availability
* Compatibility
* Composability
* Debuggability
* Expressiveness
* Extensibility
* Interoperability
* Integrity
* Maintainability
* Measurability
* Operability
* Performance
* Portability
* Resiliency
* Rigor
* Robustness
* Safety
* Security
* Simplicity
* Stability
* Thoroughness
* Transparency
* Velocity

There are several values that don't apply. The reason they don't apply is
because `bc` and `dc` are existing utilities; this is just another
reimplementation. The designs of `bc` and `dc` are set in stone; there is
nothing we can do to change them, so let's get rid of those values that would
apply to their design:

* Compatibility
* Integrity
* Maintainability
* Measurability
* Performance
* Portability
* Resiliency
* Rigor
* Robustness
* Safety
* Security
* Simplicity
* Stability
* Thoroughness
* Transparency

Furthermore, some of the remaining ones don't matter to me, so let me get rid of
those and order the rest according to my *actual* values for this project:

* Robustness
* Stability
* Portability
* Compatibility
* Performance
* Security
* Simplicity

First is **robustness**. This `bc` and `dc` should be robust, accepting any
input, never crashing, and instead, returning an error.

Closely related to that is **stability**. The execution of `bc` and `dc` should
be deterministic and never change for the same inputs, including the
pseudo-random number generator (for the same seed).

Third is **portability**. These programs should run everywhere that POSIX
exists, as well as Windows. This means that just about every person on the
planet will have access to these programs.

Next is **compatibility**. These programs should, as much as possible, be
compatible with other existing implementations and standards.

Then we come to **performance**. A calculator is only usable if it's fast, so
these programs should run as fast as possible.

After that is **security**. These programs should *never* be the reason a user's
computer is compromised.

And finally, **simplicity**. Where possible, the code should be simple, while
deferring to the above values.

Keep these values in mind for the rest of this document, and for exploring any
other part of this repo.

### Portability

But before I go on, I want to talk about portability in particular.

Most of these principles just require good attention and care, but portability
is different. Sometimes, it requires pulling in code from other places and
adapting it. In other words, sometimes I need to duplicate and adapt code.

This happened in a few cases:

* Option parsing (see [`include/opt.h`][35]).
* History (see [`include/history.h`][36]).
* Pseudo-Random Number Generator (see [`include/rand.h`][37]).

This was done because I decided to ensure that `bc`'s dependencies were
basically zero. In particular, either users have a normal install of Windows or
they have a POSIX system.

A POSIX system limited me to C99, `sh`, and zero external dependencies. That
last item is why I pull code into `bc`: if I pull it in, it's not an external
dependency.

That's why `bc` has duplicated code. Remove it, and you risk `bc` not being
portable to some platforms.

## Useful External Tools

I have a few tools external to `bc` that are useful:

* A [Vim plugin with syntax files made specifically for my `bc` and `dc`][132].
* A [repo of `bc` and `dc` scripts][133].
* A set of `bash` aliases (see below).
* A `.bcrc` file with items useful for my `bash` setup (see below).

My `bash` aliases are these:

```sh
alias makej='make -j16'
alias mcmake='make clean && make'
alias mcmakej='make clean && make -j16'
alias bcdebug='CPPFLAGS="-DBC_DEBUG_CODE=1" CFLAGS="-Weverything -Wno-padded \
    -Wno-switch-enum -Wno-format-nonliteral -Wno-cast-align \
    -Wno-unreachable-code-return -Wno-missing-noreturn \
    -Wno-disabled-macro-expansion -Wno-unreachable-code -Wall -Wextra \
    -pedantic -std=c99" ./configure.sh'
alias bcconfig='CFLAGS="-Weverything -Wno-padded -Wno-switch-enum \
    -Wno-format-nonliteral -Wno-cast-align -Wno-unreachable-code-return \
    -Wno-missing-noreturn -Wno-disabled-macro-expansion -Wno-unreachable-code \
    -Wall -Wextra -pedantic -std=c99" ./configure.sh'
alias bcnoassert='CPPFLAGS="-DNDEBUG" CFLAGS="-Weverything -Wno-padded \
    -Wno-switch-enum -Wno-format-nonliteral -Wno-cast-align \
    -Wno-unreachable-code-return -Wno-missing-noreturn \
    -Wno-disabled-macro-expansion -Wno-unreachable-code -Wall -Wextra \
    -pedantic -std=c99" ./configure.sh'
alias bcdebugnoassert='CPPFLAGS="-DNDEBUG -DBC_DEBUG_CODE=1" \
    CFLAGS="-Weverything -Wno-padded -Wno-switch-enum -Wno-format-nonliteral \
    -Wno-cast-align -Wno-unreachable-code-return -Wno-missing-noreturn \
    -Wno-disabled-macro-expansion -Wno-unreachable-code -Wall -Wextra \
    -pedantic -std=c99" ./configure.sh'
alias bcunset='unset BC_LINE_LENGTH && unset BC_ENV_ARGS'
```

`makej` runs `make` with all of my cores.

`mcmake` runs `make clean` before running `make`. It will take a target on the
command-line.

`mcmakej` is a combination of `makej` and `mcmake`.

`bcdebug` configures `bc` for a full debug build, including `BC_DEBUG_CODE` (see
[Debugging][134] below).

`bcconfig` configures `bc` with Clang (Clang is my personal default compiler)
using full warnings, with a few really loud and useless warnings turned off.

`bcnoassert` configures `bc` to not have asserts built in.

`bcdebugnoassert` is like `bcnoassert`, except it also configures `bc` for debug
mode.

`bcunset` unsets my personal `bc` environment variables, which are set to:

```sh
export BC_ENV_ARGS="-l $HOME/.bcrc"
export BC_LINE_LENGTH="74"
```

Unsetting these environment variables are necessary for running
[`scripts/release.sh`][83] because otherwise, it will error when attempting to
run `bc -s` on my `$HOME/.bcrc`.

Speaking of which, the contents of that file are:

```bc
define void print_time_unit(t){
	if(t<10)print "0"
	if(t<1&&t)print "0"
	print t,":"
}
define void sec2time(t){
	auto s,m,h,d,r
	r=scale
	scale=0
	t=abs(t)
	s=t%60
	t-=s
	m=t/60%60
	t-=m
	h=t/3600%24
	t-=h
	d=t/86400
	if(d)print_time_unit(d)
	if(h)print_time_unit(h)
	print_time_unit(m)
	if(s<10)print "0"
	if(s<1&&s)print "0"
	s
	scale=r
}
define minutes(secs){
	return secs/60;
}
define hours(secs){
	return secs/3600;
}
define days(secs){
	return secs/3600/24;
}
define years(secs){
	return secs/3600/24/365.25;
}
define fbrand(b,p){
	auto l,s,t
	b=abs(b)$
	if(b<2)b=2
	s=scale
	t=b^abs(p)$
	l=ceil(l2(t),0)
	if(l>scale)scale=l
	t=irand(t)/t
	scale=s
	return t
}
define ifbrand(i,b,p){return irand(abs(i)$)+fbrand(b,p)}
```

This allows me to use `bc` as part of my `bash` prompt.

## Code Style

The code style for `bc` is...weird, and that comes from historical accident.

In [History][23], I mentioned how I got my `bc` in [toybox][16]. Well, in order
to do that, my `bc` originally had toybox style. Eventually, I changed to using
tabs, and assuming they were 4 spaces wide, but other than that, I basically
kept the same style, with some exceptions that are more or less dependent on my
taste.

The code style is as follows:

* Tabs are 4 spaces.
* Tabs are used at the beginning of lines for indent.
* Spaces are used for alignment.
* Lines are limited to 80 characters, period.
* Pointer asterisk (`*`) goes with the variable (on the right), not the type,
  unless it is for a pointer type returned from a function.
* The opening brace is put on the same line as the header for the function,
  loop, or `if` statement.
* Unless the header is more than one line, in which case the opening brace is
  put on its own line.
* If the opening brace is put on its own line, there is no blank line after it.
* If the opening brace is *not* put on its own line, there *is* a blank line
  after it, *unless* the block is only one or two lines long.
* Code lines are grouped into what I call "paragraphs." Basically, lines that
  seem like they should go together are grouped together. This one comes down
  to judgment.
* Bodies of `if` statements, `else` statements, and loops that are one line
  long are put on the same line as the statement, unless the header is more than
  one line long, and/or, the header and body cannot fit into 80 characters with
  a space inbetween them.
* If single-line bodies are on a separate line from their headers, and the
  headers are only a single line, then no braces are used.
* However, braces are *always* used if they contain another `if` statement or
  loop.
* Loops with empty bodies are ended with a semicolon.
* Expressions that return a boolean value are surrounded by paretheses.
* Macro backslashes are aligned as far to the left as possible.
* Binary operators have spaces on both sides.
* If a line with binary operators overflows 80 characters, a newline is inserted
  *after* binary operators.
* Function modifiers and return types are on the same line as the function name.
* With one exception, `goto`'s are only used to jump to the end of a function
  for cleanup.
* All structs, enums, and unions are `typedef`'ed.
* All constant data is in one file: [`src/data.c`][131], but the corresponding
  `extern` declarations are in the appropriate header file.
* All local variables are declared at the beginning of the scope where they
  appear. They may be initialized at that point, if it does not invoke UB or
  otherwise cause bugs.
* All precondition `assert()`'s (see [Asserts][135]) come *after* local variable
  declarations.
* Besides short `if` statements and loops, there should *never* be more than one
  statement per line.

### ClangFormat

I attempted three times to use [ClangFormat][24] to impose a standard,
machine-useful style on `bc`. All three failed. Otherwise, the style in this
repo would be more consistent.

## Repo Structure

Functions are documented with Doxygen-style doc comments. Functions that appear
in headers are documented in the headers, while static functions are documented
where they are defined.

### `bcl.sln`

A Visual Studio solution file for `bcl`. This, along with [`bcl.vcxproj`][63]
and [`bcl.vcxproj.filters`][64] is what makes it possible to build `bcl` on
Windows.

### `bcl.vcxproj`

A Visual Studio project file for `bcl`. This, along with [`bcl.sln`][65] and
[`bcl.vcxproj.filters`][64] is what makes it possible to build `bcl` on Windows.

### `bcl.vcxproj.filters`

A Visual Studio filters file for `bcl`. This, along with [`bcl.sln`][65] and
[`bcl.vcxproj`][63] is what makes it possible to build `bcl` on Windows.

### `bc.sln`

A Visual Studio solution file for `bc`. This, along with [`bc.vcxproj`][66]
and [`bc.vcxproj.filters`][67] is what makes it possible to build `bc` on
Windows.

### `bc.vcxproj`

A Visual Studio project file for `bcl`. This, along with [`bc.sln`][68] and
[`bc.vcxproj.filters`][67] is what makes it possible to build `bc` on Windows.

### `bc.vcxproj.filters`

A Visual Studio filters file for `bc`. This, along with [`bc.sln`][68] and
[`bc.vcxproj`][66] is what makes it possible to build `bc` on Windows.

### `configure`

A symlink to [`configure.sh`][69].

### `configure.sh`

This is the script to configure `bc` and `bcl` for building.

This `bc` has a custom build system. The reason for this is because of
[*portability*][136].

If `bc` used an outside build system, that build system would be an external
dependency. Thus, I had to write a build system for `bc` that used nothing but
C99 and POSIX utilities.

One of those utilities is POSIX `sh`, which technically implements a
Turing-complete programming language. It's a terrible one, but it works.

A user that wants to build `bc` on a POSIX system (not Windows) first runs
`configure.sh` with the options he wants. `configure.sh` uses those options and
the `Makefile` template ([`Makefile.in`][70]) to generate an actual valid
`Makefile`. Then `make` can do the rest.

For more information about the build process, see the [build manual][14].

For more information about shell scripts, see [POSIX Shell Scripts][76].

`configure.sh` does the following:

1.	It processes command-line arguments and figure out what the user wants to
	build.
2.	It reads in [`Makefile.in`][70].
3.	One-by-one, it replaces placeholders (in [`Makefile.in`][70]) of the form
	`%%<placeholder_name>%%` based on the build type.
4.	It appends a list of file targets based on the build type.
5.	It appends the correct test targets.
6.	It copies the correct manpage and markdown manual for `bc` and `dc` into a
	location from which they can be copied for install.
7.	It does a `make clean` to reset the build state.

### `.gitattributes`

A `.gitattributes` file. This is needed to preserve the `crlf` line endings in
the Visual Studio files.

### `.gitignore`

The `.gitignore`

### `LICENSE.md`

This is the `LICENSE` file, including the licenses of various software that I
have borrowed.

### `Makefile.in`

This is the `Makefile` template for [`configure.sh`][69] to use for generating a
`Makefile`.

For more information, see [`configure.sh`][69] and the [build manual][14].

Because of [portability][136], the generated `Makefile.in` should be a pure
[POSIX `make`][74]-compatible `Makefile` (minus the placeholders). Here are a
few snares for the unwary programmer in this file:

1.	No extensions allowed, including and especially GNU extensions.
2.	If new headers are added, they must also be added to `Makefile.in`.
3.	Don't delete the `.POSIX:` empty target at the top; that's what tells `make`
	implementations that pure [POSIX `make`][74] is needed.

In particular, there is no way to set up variables other than the `=` operator.
There are no conditionals, so all of the conditional stuff must be in
[`configure.sh`][69]. This is, in fact, why [`configure.sh`][69] exists in the
first place: [POSIX `make`][74] is barebones and only does a build with no
configuration.

### `NEWS.md`

A running changelog with an entry for each version. This should be updated at
the same time that [`include/version.h`][75] is.

### `NOTICE.md`

The `NOTICE` file with proper attributions.

### `README.md`

The `README`. Read it.

### `gen/`

A folder containing the files necessary to generate C strings that will be
embedded in the executable.

All of the files in this folder have license headers, but the program and script
that can generate strings from them include code to strip the license header out
before strings are generated.

#### `bc_help.txt`

A text file containing the text displayed for `bc -h` or `bc --help`.

This text just contains the command-line options and a short summary of the
differences from GNU and BSD `bc`'s. It also directs users to the manpage.

The reason for this is because otherwise, the help would be far too long to be
useful.

#### `dc_help.txt`

A text file containing the text displayed for `dc -h` or `dc --help`.

This text just contains the command-line options and a short summary of the
differences from GNU and BSD `dc`'s. It also directs users to the manpage.

The reason for this is because otherwise, the help would be far too long to be
useful.

#### `lib.bc`

A `bc` script containing the [standard math library][5] required by POSIX. See
the [POSIX standard][6] for what is required.

This file does not have any extraneous whitespace, except for tabs at the
beginning of lines. That is because this data goes directly into the binary,
and whitespace is extra bytes in the binary. Thus, not having any extra
whitespace shrinks the resulting binary.

However, tabs at the beginning of lines are kept for two reasons:

1.	Readability. (This file is still code.)
2.	The program and script that generate strings from this file can remove
	tabs at the beginning of lines.

For more details about the algorithms used, see the [algorithms manual][25].

However, there are a few snares for unwary programmers.

First, all constants must be one digit. This is because otherwise, multi-digit
constants could be interpreted wrongly if the user uses a different `ibase`.
This does not happen with single-digit numbers because they are guaranteed to be
interpreted what number they would be if the `ibase` was as high as possible.

This is why `A` is used in the library instead of `10`, and things like `2*9*A`
for `180` in [`lib2.bc`][26].

As an alternative, you can set `ibase` in the function, but if you do, make sure
to set it with a single-digit number and beware the snare below...

Second, `scale`, `ibase`, and `obase` must be safely restored before returning
from any function in the library. This is because without the `-g` option,
functions are allowed to change any of the globals.

Third, all local variables in a function must be declared in an `auto` statement
before doing anything else. This includes arrays. However, function parameters
are considered predeclared.

Fourth, and this is only a snare for `lib.bc`, not [`lib2.bc`][26], the code
must not use *any* extensions. It has to work when users use the `-s` or `-w`
flags.

#### `lib2.bc`

A `bc` script containing the [extended math library][7].

Like [`lib.bc`][8], and for the same reasons, this file should have no
extraneous whitespace, except for tabs at the beginning of lines.

For more details about the algorithms used, see the [algorithms manual][25].

Also, be sure to check [`lib.bc`][8] for the snares that can trip up unwary
programmers when writing code for `lib2.bc`.

#### `strgen.c`

Code for the program to generate C strings from text files. This is the original
program, although [`strgen.sh`][9] was added later.

The reason I used C here is because even though I knew `sh` would be available
(it must be available to run `configure.sh`), I didn't know how to do what I
needed to do with POSIX utilities and `sh`.

Later, [`strgen.sh`][9] was contributed by Stefan Eßer of FreeBSD, showing that
it *could* be done with `sh` and POSIX utilities.

However, `strgen.c` exists *still* exists because the versions generated by
[`strgen.sh`][9] may technically hit an environmental limit. (See the [draft C99
standard][12], page 21.) This is because [`strgen.sh`][9] generates string
literals, and in C99, string literals can be limited to 4095 characters, and
`gen/lib2.bc` is above that.

Fortunately, the limit for "objects," which include `char` arrays, is much
bigger: 65535 bytes, so that's what `strgen.c` generates.

However, the existence of `strgen.c` does come with a cost: the build needs C99
compiler that targets the host machine. For more information, see the ["Cross
Compiling" section][13] of the [build manual][14].

Read the comments in `strgen.c` for more detail about it, the arguments it
takes, and how it works.

#### `strgen.sh`

An `sh` script that will generate C strings that uses only POSIX utilities. This
exists for those situations where a host C99 compiler is not available, and the
environment limits mentioned above in [`strgen.c`][15] don't matter.

`strgen.sh` takes the same arguments as [`strgen.c`][15], and the arguments mean
the exact same things, so see the comments in [`strgen.c`][15] for more detail
about that, and see the comments in `strgen.sh` for more details about it and
how it works.

For more information about shell scripts, see [POSIX Shell Scripts][76].

### `include/`

A folder containing the headers.

The headers are not included among the source code because I like it better that
way. Also there were folders within `src/` at one point, and I did not want to
see `#include "../some_header.h"` or things like that.

So all headers are here, even though only one (`bcl.h`) is meant for end users
(to be installed in `INCLUDEDIR`).

#### `args.h`

This file is the API for processing command-line arguments.

#### `bc.h`

This header is the API for `bc`-only items. This includes the `bc_main()`
function and the `bc`-specific lexing and parsing items.

The `bc` parser is perhaps the most sensitive part of the entire codebase. See
the documentation in `bc.h` for more information.

The code associated with this header is in [`src/bc.c`][40],
[`src/bc_lex.c`][41], and [`src/bc_parse.c`][42].

#### `bcl.h`

This header is the API for the `bcl` library.

This header is meant for distribution to end users and contains the API that end
users of `bcl` can use in their own software.

This header, because it's the public header, is also the root header. That means
that it has platform-specific fixes for Windows. (If the fixes were not in this
header, the build would fail on Windows.)

The code associated with this header is in [`src/library.c`][43].

#### `dc.h`

This header is the API for `dc`-only items. This includes the `dc_main()`
function and the `dc`-specific lexing and parsing items.

The code associated with this header is in [`src/dc.c`][44],
[`src/dc_lex.c`][45], and [`src/dc_parse.c`][46].

#### `file.h`

This header is for `bc`'s internal buffered I/O API.

Why did I implement my own buffered I/O for `bc`? Because I use `setjmp()` and
`longjmp()` for error handling, and the buffered I/O in `libc` does not interact
well with the use of those procedures.

For more information about `bc`'s error handling and custom buffered I/O, see
[Error Handling][97] and [Custom I/O][114], along with [`vm.h`][27] and the
notes about version [`3.0.0`][32] in the [`NEWS`][32].

The code associated with this header is in [`src/file.c`][47].

#### `history.h`

This header is for `bc`'s implementation of command-line editing/history, which
is based on a [UTF-8-aware fork][28] of [`linenoise`][29].

At one point, I attempted to get history to work on Windows. It did not work
out. Windows

The code associated with this header is in [`src/history.c`][48].

#### `lang.h`

This header defines the data structures and bytecode used for actual execution
of `bc` and `dc` code.

Yes, it's misnamed; that's an accident of history where the first things I put
into it all seemed related to the `bc` language.

The code associated with this header is in [`src/lang.c`][49].

#### `lex.h`

This header defines the common items that both programs need for lexing.

The code associated with this header is in [`src/lex.c`][50],
[`src/bc_lex.c`][41], and [`src/dc_lex.c`][45].

#### `library.h`

This header defines the things needed for `bcl` that users should *not* have
access to. In other words, [`bcl.h`][30] is the *public* header for the library,
and this header is the *private* header for the library.

The code associated with this header is in [`src/library.c`][43].

#### `num.h`

This header is the API for numbers and math.

The code associated with this header is in [`src/num.c`][39].

#### `opt.h`

This header is the API for parsing command-line arguments.

It's different from [`args.h`][31] in that [`args.h`][31] is for the main code
to process the command-line arguments into global data *after* they have already
been parsed by `opt.h` into proper tokens. In other words, `opt.h` actually
parses the command-line arguments, and [`args.h`][31] turns that parsed data
into flags (bits), strings, and expressions that will be used later.

Why are they separate? Because originally, `bc` used `getopt_long()` for
parsing, so [`args.h`][31] was the only one that existed. After it was
discovered that `getopt_long()` has different behavior on different platforms, I
adapted a [public-domain option parsing library][34] to do the job instead. And
in doing so, I gave it its own header.

They could probably be combined, but I don't really care enough at this point.

The code associated with this header is in [`src/opt.c`][51].

#### `parse.h`

This header defines the common items that both programs need for parsing.

Note that the parsers don't produce abstract syntax trees (AST's) or any
intermediate representations. They produce bytecode directly. In other words,
they don't have special data structures except what they need to do their job.

The code associated with this header is in [`src/parse.c`][50],
[`src/bc_lex.c`][42], and [`src/dc_lex.c`][46].

#### `program.h`

This header defines the items needed to manage the data structures in
[`lang.h`][38] as well as any helper functions needed to generate bytecode or
execute it.

The code associated with this header is in [`src/program.c`][53].

#### `rand.h`

This header defines the API for the pseudo-random number generator (PRNG).

The PRNG only generates fixed-size integers. The magic of generating random
numbers of arbitrary size is actually given to the code that does math
([`src/num.c`][39]).

The code associated with this header is in [`src/rand.c`][54].

#### `read.h`

This header defines the API for reading from files and `stdin`.

Thus, [`file.h`][55] is really for buffered *output*, while this file is for
*input*. There is no buffering needed for `bc`'s inputs.

The code associated with this header is in [`src/read.c`][56].

#### `status.h`

This header has several things:

* A list of possible errors that internal `bc` code can use.
* Compiler-specific fixes.
* Platform-specific fixes.
* Macros for `bc`'s [error handling][97].

There is no code associated with this header.

#### `vector.h`

This header defines the API for the vectors (resizable arrays) that are used for
data structures.

Vectors are what do the heavy lifting in almost all of `bc`'s data structures.
Even the maps of identifiers and arrays use vectors.

#### `version.h`

This header defines the version of `bc`.

There is no code associated with this header.

#### `vm.h`

This header defines the code for setting up and running `bc` and `dc`. It is so
named because I think of it as the "virtual machine" of `bc`, though that is
probably not true as [`program.h`][57] is probably the "virtual machine". Thus,
the name is more historical accident.

The code associated with this header is in [`src/vm.c`][58].

### `locales/`

This folder contains a bunch of `.msg` files and soft links to the real `.msg`
files. This is how locale support is implemented in `bc`.

The files are in the format required by the [`gencat`][59] POSIX utility. They
all have the same messages, in the same order, with the same numbering, under
the same groups. This is because the locale system expects those messages in
that order.

The softlinks exist because for many locales, they would contain the exact same
information. To prevent duplication, they are simply linked to a master copy.

The naming format for all files is:

```
<language_code>_<country_code>.<encoding>.msg
```

This naming format must be followed for all locale files.

### `manuals/`

This folder contains the documentation for `bc`, `dc`, and `bcl`, along with a
few other manuals.

#### `algorithms.md`

This file explains the mathematical algorithms that are used.

The hope is that this file will guide people in understanding how the math code
works.

#### `bc.1.md.in`

This file is a template for the markdown version of the `bc` manual and
manpages.

For more information about how the manpages and markdown manuals are generated,
and for why, see [`scripts/manpage.sh`][60] and [Manuals][86].

#### `bcl.3`

This is the manpage for the `bcl` library. It is generated from
[`bcl.3.md`][61] using [`scripts/manpage.sh`][60].

For the reason why I check generated data into the repo, see
[`scripts/manpage.sh`][60] and [Manuals][86].

#### `bcl.3.md`

This is the markdown manual for the `bcl` library. It is the source for the
generated [`bcl.3`][62] file.

#### `benchmarks.md`

This is a document that compares this `bc` to GNU `bc` in various benchmarks. It
was last updated when version [`3.0.0`][32] was released.

It has very little documentation value, other than showing what compiler options
are useful for performance.

#### `build.md`

This is the [build manual][14].

This `bc` has a custom build system. The reason for this is because of
[*portability*][136].

If `bc` used an outside build system, that build system would be an external
dependency. Thus, I had to write a build system for `bc` that used nothing but
C99 and POSIX utilities, including barebones [POSIX `make`][74].

For more information about the build system, see the [build manual][14],
[`configure.sh`][69], and [`Makefile.in`][70].

#### `dc.1.md.in`

This file is a template for the markdown version of the `dc` manual and
manpages.

For more information about how the manpages and markdown manuals are generated,
and for why, see [`scripts/manpage.sh`][60] and [Manuals][86].

#### `development.md`

The file you are reading right now.

TODO:

* Document all code assumptions with asserts.
* Document all functions with Doxygen comments.
* The purpose of every file.

#### `header_bcl.txt`

Used by [`scripts/manpage.sh`][60] to give the [`bcl.3`][62] manpage a proper
header.

For more information about generating manuals, see [`scripts/manpage.sh`][60]
and [Manuals][86].

#### `header_bc.txt`

Used by [`scripts/manpage.sh`][60] to give the [generated `bc` manpages][79] a
proper header.

For more information about generating manuals, see [`scripts/manpage.sh`][60]
and [Manuals][86].

#### `header_dc.txt`

Used by [`scripts/manpage.sh`][60] to give the [generated `dc` manpages][80] a
proper header.

For more information about generating manuals, see [`scripts/manpage.sh`][60]
and [Manuals][86].

#### `header.txt`

Used by [`scripts/manpage.sh`][60] to give all generated manpages a license
header.

For more information about generating manuals, see [`scripts/manpage.sh`][60]
and [Manuals][86].

#### `release.md`

A checklist that I try to somewhat follow when making a release.

#### `bc/`

A folder containing the `bc` manuals.

Each `bc` manual corresponds to a [build type][81]. See that link for more
details.

For each manual, there are two copies: the markdown version generated from the
template, and the manpage generated from the markdown version.

#### `dc/`

A folder containing the `dc` manuals.

Each `dc` manual corresponds to a [build type][81]. See that link for more
details.

For each manual, there are two copies: the markdown version generated from the
template, and the manpage generated from the markdown version.

### `scripts/`

This folder contains helper scripts. Most of them are written in pure [POSIX
`sh`][72], but one ([`karatsuba.py`][78]) is written in Python 3.

For more information about the shell scripts, see [POSIX Shell Scripts][76].

#### `afl.py`

This script is meant to be used as part of the fuzzing workflow.

It does one of two things: checks for valid crashes, or runs `bc` and or `dc`
under all of the paths found by [AFL++][125].

See [Fuzzing][82] for more information about fuzzing, including this script.

#### `alloc.sh`

This script is a quick and dirty script to test whether or not the garbage
collection mechanism of the [`BcNum` caching][96] works. It has been little-used
because it tests something that is not important to correctness.

#### `exec-install.sh`

This script is the magic behind making sure `dc` is installed properly if it's
a symlink to `bc`. It checks to see if it is a link, and if so, it just creates
a new symlink in the install directory. Of course, it also installs `bc` itself,
or `dc` when it's alone.

#### `functions.sh`

This file is a bunch of common functions for most of the POSIX shell scripts. It
is not supposed to be run; instead, it is *sourced* by other POSIX shell
scripts, like so:

```
. "$scriptdir/functions.sh"
```

or the equivalent, depending on where the sourcing script is.

For more information about the shell scripts, see [POSIX Shell Scripts][76].

#### `fuzz_prep.sh`

Fuzzing is a regular activity when I am preparing for a release.

This script handles all the options and such for building a fuzzable binary.
Instead of having to remember a bunch of options, I just put them in this script
and run the script when I want to fuzz.

For more information about fuzzing, see [Fuzzing][82].

#### `karatsuba.py`

This script has at least one of two major differences from most of the other
scripts:

* It's in Python 3.
* It's meant for software packagers.

For example, [`scripts/afl.py`][94] and [`scripts/randmath.py`][95] are both in
Python 3, but they are not meant for the end user or software packagers and are
not included in source distributions. But this script is.

This script breaks my rule of only POSIX utilities necessary for package
maintainers, but there's a very good reason for that: it's only meant to be run
*once* when the package is created for the first time, and maybe not even then.

You see, this script does two things: it tests the Karatsuba implementation at
various settings for `KARATSUBA_LEN`, and it figures out what the optimal
`KARATSUBA_LEN` is for the machine that it is running on.

Package maintainers can use this script, when creating a package for this `bc`,
to figure out what is optimal for their users. Then they don't have to run it
ever again. So this script only has to run on the packagers machine.

I tried to write the script in `sh`, by the way, and I finally accepted the
tradeoff of using Python 3 when it became too hard.

However, I also mentioned that it's for testing Karatsuba with various settings
of `KARATSUBA_LEN`. Package maintainers will want to run the test suite, right?

Yes, but this script is not part of the test suite; it's used for testing in the
[`scripts/release.sh`][83] script, which is maintainer use only.

However, there is one snare with `karatsuba.py`: I didn't want the user to have
to install any Python libraries to run it. Keep that in mind if you change it.

#### `link.sh`

This script is the magic behind making `dc` a symlink of `bc` when both
calculators are built.

#### `locale_install.sh`

This script does what its name says: it installs locales.

It turns out that this is complicated.

There is a magic environment variable, `$NLSPATH`, that tells you how and where
you are supposed to install locales.

Yes, *how*. And where.

But now is not the place to rant about `$NLSPATH`. For more information on
locales and `$NLSPATH`, see [Locales][85].

#### `locale_uninstall.sh`

This script does what its name says: it uninstalls locales.

This is far less complicated than installing locales. I basically generate a
wildcard path and then list all paths that fit that wildcard. Then I delete each
one of those paths. Easy.

For more information on locales, see [Locales][85].

#### `manpage.sh`

This script is the one that generates markdown manuals from a template and a
manpage from a markdown manual.

For more information about generating manuals, see [Manuals][86].

#### `package.sh`

This script is what helps `bc` maintainers cut a release. It does the following:

1.	Creates the appropriate `git` tag.
2.	Pushes the `git` tag.
3.	Copies the repo to a temp directory.
4.	Removes files that should not be included in source distributions.
5.	Creates the tarballs.
6.	Signs the tarballs.
7.	Zips and signs the Windows executables if they exist.
8.	Calculates and outputs SHA512 and SHA256 sums for all of the files,
	including the signatures.

This script is for `bc` maintainers to use when cutting a release. It is not
meant for outside use. This means that some non-POSIX utilities can be used,
such as `git` and `gpg`.

In addition, before using this script, it expects that the folders that Windows
generated when building `bc`, `dc`, and `bcl`, are in the parent directory of
the repo, exactly as Windows generated them. If they are not there, then it will
not zip and sign, nor calculate sums of, the Windows executables.

Because this script creates a tag and pushes it, it should *only* be run *ONCE*
per release.

#### `radamsa.sh`

A script to test `bc`'s command-line expression parsing code, which, while
simple, strives to handle as much as possible.

What this script does is it uses the test cases in [`radamsa.txt`][98] an input
to the [Radamsa fuzzer][99].

For more information, see the [Radamsa][128] section.

#### `radamsa.txt`

Initial test cases for the [`radamsa.sh`][100] script.

#### `randmath.py`

This script generates random math problems and checks that `bc`'s and `dc`'s
output matches the GNU `bc` and `dc`. (For this reason, it is necessary to have
GNU `bc` and `dc` installed before using this script.)

One snare: be sure that this script is using the GNU `bc` and `dc`, not a
previously-installed version of this `bc` and `dc`.

If you want to check for memory issues or failing asserts, you can build the
`bc` using `./scripts/fuzz_prep.sh -a`, and then run it under this script. Any
errors or crashes should be caught by the script and given to the user as part
of the "checklist" (see below).

The basic idea behind this script is that it generates as many math problems as
it can, biasing towards situations that may be likely to have bugs, and testing
each math problem against GNU `bc` or `dc`.

If GNU `bc` or `dc` fails, it just continues. If this `bc` or `dc` fails, it
stores that problem. If the output mismatches, it also stores the problem.

Then, when the user sends a `SIGINT`, the script stops testing and goes into
report mode. One-by-one, it will go through the "checklist," the list of failed
problems, and present each problem to the user, as well as whether this `bc` or
`dc` crashed, and its output versus GNU. Then the user can decide to add them as
test cases, which it does automatically to the appropriate test file.

#### `release_settings.txt`

A text file of settings combinations that [`release.sh`][83] uses to ensure that
`bc` and `dc` build and work with various default settings. [`release.sh`][83]
simply reads it line by line and uses each line for one build.

#### `release.sh`

This script is for `bc` maintainers only. It runs `bc`, `dc`, and `bcl` through
a gauntlet that is mostly meant to be used in preparation for a release.

It does the following:

1.	Builds every build type, with every setting combo in
	[`release_settings.txt`][93] with both calculators, `bc` alone, and `dc`
	alone.
2.	Builds every build type, with every setting combo in
	[`release_settings.txt`][93] with both calculators, `bc` alone, and `dc`
	alone for 32-bit.
3.	Does #1 and #2 for Debug, Release, Release with Debug Info, and Min Size
	Release builds.
4.	Runs the test suite on every build, if desired.
5.	Runs the test suite under [ASan, UBSan, and MSan][21] for every build
	type/setting combo.
6.	Runs [`scripts/karatsuba.py`][78] in test mode.
7.	Runs the test suite for both calculators, `bc` alone, and `dc` alone under
	[valgrind][20] and errors if there are any memory bugs or memory leaks.

#### `safe-install.sh`

A script copied from [musl][101] to atomically install files.

#### `test_settings.sh`

A quick and dirty script to help automate rebuilding while manually testing the
various default settings.

This script uses [`test_settings.txt`][103] to generate the various settings
combos.

For more information about settings, see [Settings][102] in the [build
manual][14].

#### `test_settings.txt`

A list of the various settings combos to be used by [`test_settings.sh`][104].

### `src/`

TODO

This folder is, obviously, where the actual heart and soul of `bc`, the source
code, is.

#### `args.c`

Code for processing command-line arguments.

#### `bc.c`

The code for the `bc` main function `bc_main()`.

#### `bc_lex.c`

The code for lexing that only `bc` needs.

#### `dc.c`

The code for the `dc` main function `dc_main()`.

#### `dc_lex.c`

The code for lexing that only `dc` needs.

### `tests/`

TODO

## Test Suite

TODO

* Normal files and results files.
* Scripts.
* Generating tests.
* Error tests.
* `stdin` tests.
* `read` tests.
* Other tests.
* Integration with the build system.

While the source code may be the heart and soul of `bc`, the test suite is the
arms and legs: it gives `bc` the power to do anything it needs to do.

The test suite is what allowed `bc` to climb to such high heights of quality.
This even goes for fuzzing because fuzzing depends on the test suite for its
input corpuses. (See the [Fuzzing][82] section.)

Understanding how the test suite works should be, I think, the first thing that
maintainers learn. This is because the test suite, properly used, gives
confidence that changes have not caused bugs or regressions.

That is why I spent the time to make the test suite as easy to use and as fast
as possible.

To use the test suite (assuming `bc` and/or `dc` are already built), run the
following command:

```
make test
```

That's it. That's all.

It will return an error code if the test suite failed. It will also print out
information about the failure.

If you want the test suite to go fast, then run the following command:

```
make -j<cores> test
```

Where `<cores>` is the number of cores that your computer has. Of course, this
requires a `make` implementation that supports that option, but most do. (And I
will use this convention throughout the rest of this section.)

I have even tried as much as possible, to put longer-running tests near the
beginning of the run so that the entire suite runs as fast as possible.

However, if you want to be sure which test is failing, then running a bare
`make test` is a great way to do that.

But enough about how you have no excuses to use the test suite as much as
possible; let's talk about how it works and what you *can* do with it.

### Test Suite Portability

The test suite is meant to be run by users and packagers as part of their
install process.

This puts some constraints on the test suite, but the biggest is that the test
suite must be as [portable as `bc` itself][136].

This means that the test suite must be implemented in pure POSIX `make`, `sh`,
and C99.

#### Testing History

Unfortunately, testing history as part of the automatic test suite is not really
possible, or rather, it is not easy with portable tools, and since the test
suite is designed to be run by users, it needs to use portable tools.

However, history can be tested manually, and I do suggest doing so for any
release that changed any history code.

### Test Coverage

In order to get test coverage information, you need `gcc`, `gcov`, and `gcovr`.

If you have them, run the following commands:

```
CC=gcc ./configure -gO3 -c
make -j<cores>
make coverage
```

Note that `make coverage` does not have a `-j<cores>` part; it cannot be run in
parallel. If you try, you will get errors. And note that `CC=gcc` is used.

After running those commands, you can open your web browser and open the
`index.html` file in the root directory of the repo. From there, you can explore
all of the coverage results.

If you see lines or branches that you think you could hit with a manual
execution, do such manual execution, and then run the following command:

```
make coverage_output
```

and the coverage output will be updated.

If you want to rerun `make coverage`, you must do a `make clean` and build
first, like this:

```
make clean
make -j<cores>
make coverage
```

Otherwise, you will get errors.

Note that history does, by default, show no coverage at all. This is because, as
mentioned above, testing history with the test suite is not really possible.

However, it can be tested manually before running `make coverage_output`. Then
it should start showing coverage.

### [AddressSanitizer][21] and Friends

To run the test suite under [AddressSanitizer][21] or any of its friends, use
the following commands:

```
CFLAGS="-fsanitize=<sanitizer> ./configure -gO3 -m
make -j<cores>
make -j<cores> test
```

where `<sanitizer>` is the correct name of the desired sanitizer. There is one
exception to the above: `UndefinedBehaviorSanitizer` should be run on a build
that has zero optimization, so for `UBSan`, use the following commands:

```
CFLAGS="-fsanitize=undefined" ./configure -gO0 -m
make -j<cores>
make -j<cores> test
```

### [Valgrind][20]

To run the test suite under [Valgrind][20], run the following commands:

```
./configure -gO3 -v
make -j<cores>
make -j<cores> test
```

It really is that easy. I have directly added infrastructure to the build system
and the test suite to ensure that if [Valgrind][20] detects any memory errors or
any memory leaks at all, it will tell the test suite infrastructure to report an
error and exit accordingly.

## POSIX Shell Scripts

There is a lot of shell scripts in this repository, and every single one of them
is written in pure [POSIX `sh`][72].

The reason that they are written in [POSIX `sh`][72] is for *portability*: POSIX
systems are only guaranteed to have a barebones implementation of `sh`
available.

There are *many* snares for unwary programmers attempting to modify
[`configure.sh`][69], any of the scripts in this directory, [`strgen.sh`][9], or
any of the scripts in [`tests/`][77]. Here are some of them:

1.	No `bash`-isms.
2.	Only POSIX standard utilities are allowed.
3.	Only command-line options defined in the POSIX standard for POSIX utilities
	are allowed.
4.	Only the standardized behavior of POSIX utilities is allowed.
5.	Functions return data by *printing* it. Using `return` sets their exit code.

In other words, the script must only use what is standardized in the [`sh`][72]
and [Shell Command Language][73] standards in POSIX. This is *hard*. It precludes
things like `local` and the `[[ ]]` notation.

These are *enormous* restrictions and must be tested properly. I put out at
least one release with a change to `configure.sh` that wasn't portable. That was
an embarrassing mistake.

The lack of `local`, by the way, is why variables in functions are named with
the form:

```
_<function_name>_<var_name>
```

This is done to prevent any clashes of variable names with already existing
names. And this applies to *all* shell scripts. However, there are a few times
when that naming convention is *not* used; all of them are because those
functions are required to change variables in the global scope.

### Maintainer-Only Scripts

If a script is meant to be used for maintainers (of `bc`, not package
maintainers), then rules 2, 3, and 4 don't need to be followed as much because
it is assumed that maintainers will be able to install whatever tools are
necessary to do the job.

## Manuals

The manuals for `bc` are all generated, and the manpages for `bc`, `dc`, and
`bcl` are also generated.

Why?

I don't like the format of manpages, and I am not confident in my ability to
write them. Also, they are not easy to read on the web.

So that explains why `bcl`'s manpage is generated from its markdown version. But
why are the markdown versions of the `bc` and `dc` generated?

Because the content of the manuals needs to change based on the [build
type][81]. For example, if `bc` was built with no history support, it should not
have the **COMMAND LINE HISTORY** section in its manual. If it did, that would
just confuse users.

So the markdown manuals for `bc` and `dc` are generated from templates
([`manuals/bc.1.md.in`][89] and [`manuals/dc.1.md.in`][90]). And from there,
the manpages are generated from the generated manuals.

The generated manpage for `bcl` ([`manuals/bcl.3`][62]) is checked into version
control, and the generated markdown manuals and manpages for `bc`
([`manuals/bc`][91]) and `dc` ([`manuals/dc`][92]) are as well.

This is because generating the manuals and manpages requires a heavy dependency
that only maintainers should care about: [Pandoc][92]. Because users should not
have to install *any* dependencies, the files are generated, checked into
version control, and included in distribution tarballs.

For more on how generating manuals and manpages works, see
[`scripts/manpage.sh`][60].

## Locales

The locale system of `bc` is enormously complex, but that's because
POSIX-compatible locales are terrible.

How are they terrible?

First, `gencat` does not work for generating cross-compilation. In other words,
it does not generate machine-portable files. There's nothing I can do about
this except for warn users.

Second, the format of `.msg` files is...interesting. Thank goodness it is text
because otherwise, it would be impossible to get them right.

Third, `.msg` files are not used. In other words, `gencat` exists. Why?

Fourth, `$NLSPATH` is an awful way to set where and *how* to install locales.

Yes, where and *how*.

Obviously, from it's name, it's a path, and that's the where. The *how* is more
complicated.

It's actually *not* a path, but a path template. It's a format string, and it
can have a few format specifiers. For more information on that, see [this
link][84]. But in essence, those format specifiers configure how each locale is
supposed to be installed.

With all those problems, why use POSIX locales? Portability, as always. I can't
assume that `gettext` will be available, but I *can* pretty well assume that
POSIX locales will be available.

The locale system of `bc` includes all files under [`locales/`][85],
[`scripts/locale_install.sh`][87], [`scripts/locale_uninstall.sh`][88],
[`scripts/functions.sh`][105], the `bc_err_*` constants in [`src/data.c`][131],
and the parts of the build system needed to activate it. There is also code in
[`src/vm.c`][58] (in `bc_vm_gettext()` for loading the current locale.

If the order of error messages and/or categories are changed, the order of
errors must be changed in the enum, the default error messages and categories in
[`src/data.c`][131], and all of the messages and categories in the `.msg` files
under [`locales/`][85].

## Fuzzing

The quality of this `bc` is directly related to the amount of fuzzing I did. As
such, I spent a lot of work making the fuzzing convenient and fast, though I do
admit that it took me a long time to admit that it did need to be faster.

First, there were several things which make fuzzing fast:

* Using [AFL++][125]'s deferred initialization.
* Splitting `bc`'s corpuses.
* Parallel fuzzing.

Second, there are several things which make fuzzing convenient:

* Preprepared input corpuses.
* [`scripts/fuzz_prep.sh`][119].
* `tmux` and `tmuxp` configs.
* [`scripts/afl.py`][94].

### Fuzzing Performance

Fuzzing with [AFL++][125] can be ***SLOW***. Spending the time to make it as
fast as possible is well worth the time.

However, there is a caveat to the above: it is easy to make [AFL++][125] crash,
be unstable, or be unable to find "paths" (see [AFL++ Quickstart][129]) if the
performance enhancements are done poorly.

To stop [AFL++][125] from crashing on test cases, and to be stable, these are
the requirements:

* The state at startup must be *exactly* the same.
* The virtual memory setup at startup must be *exactly* the same.

The first isn't too hard; it's the second that is difficult.

`bc` allocates a lot of memory at start. ("A lot" is relative; it's far less
than most programs.) After going through an execution run, however, some of that
memory, while it could be cleared and reset, is in different places because of
vectors. Since vectors reallocate, their allocations are not guaranteed to be in
the same place.

So to make all three work, I had to set up the deferred initialization and
persistent mode *before* any memory was allocated (except for `vm.jmp_bufs`,
which is probably what caused the stability to drop below 100%). However, using
deferred alone let me put the [AFL++][125] initialization further back. This
works because [AFL++][125] sets up a `fork()` server that `fork()`'s `bc` right
at that call. Thus, every run has the exact same virtual memory setup, and each
run can skip all of the setup code.

I tested `bc` using [AFL++][125]'s deferred initialization, plus persistent
mode, plus shared memory fuzzing. In order to do it safely, with stability above
99%, all of that was actually *slower* than using just deferred initialization
with the initialization *right before* `stdin` was read. And as a bonus, the
stability in that situation is 100%.

As a result, my [AFL++][125] setup only uses deferred initialization. That's the
`__AFL_INIT()` call.

(Note: there is one more big item that must be done in order to have 100%
stability: the pseudo-random number generator *must* start with *exactly* the
same seed for every run. This is set up with the `tmux` and `tmuxp` configs that
I talk about below in [Convenience][130]. This seed is set before the
`__AFL_INIT()` call, so setting it has no runtime cost for each run, but without
it, stability would be abysmal.)

On top of that, while `dc` is plenty fast under fuzzing (because of a faster
parser and less test cases), `bc` can be slow. So I have split the `bc` input
corpus into three parts, and I set fuzzers to run on each individually. This
means that they will duplicate work, but they will also find more stuff.

On top of all of that, each input corpus (the three `bc` corpuses and the one
`dc` corpus) is set to run with 4 fuzzers. That works out perfectly for two
reasons: first, my machine has 16 cores, and second, the [AFL++][125] docs
recommend 4 parallel fuzzers, at least, to run different "power schedules."

### Convenience

The preprepared input corpuses are contained in the
`tests/fuzzing/bc_inputs{1,2,3}/`, and `tests/fuzzing/dc_inputs` directories.
There are three `bc` directories and only one `dc` directory because `bc`'s
input corpuses are about three times as large, and `bc` is a larger program;
it's going to need much more fuzzing.

(They do share code though, so fuzzing all of them still tests a lot of the same
math code.)

The next feature of convenience is the [`scripts/fuzz_prep.sh`][119] script. It
assumes the existence of `afl-clang-lto` in the `$PATH`, but if that exists, it
automatically configures and builds `bc` with a fuzz-ideal build.

A fuzz-ideal build has several things:

* `afl-clang-lto` as the compiler. (See [AFL++ Quickstart][129].)
* Debug mode, to crash as easily as possible.
* Full optimization (including [Link-Time Optimization][126]), for performance.
* [AFL++][125]'s deferred initialization (see [Fuzzing Performance][127] above).
* And `AFL_HARDEN=1` during the build to harden the build. See the [AFL++][125]
  documentation for more information.

There is one big thing that a fuzz-ideal build does *not* have: it does not use
[AFL++][125]'s `libdislocator.so`. This is because `libdislocator.so` crashes if
it fails to allocate memory. I do not want to consider those as crashes because
my `bc` does, in fact, handle them gracefully by exiting with a set error code.
So `libdislocator.so` is not an option.

However, to add to [`scripts/fuzz_prep.sh`][119] making a fuzz-ideal build, in
`tests/fuzzing/`, there are two `yaml` files: [`tests/fuzzing/bc_afl.yaml`][120]
and [`tests/fuzzing/bc_afl_continue.yaml`][121]. These files are meant to be
used with [`tmux`][122] and [`tmuxp`][123]. While other programmers will have to
adjust the `start_directory` item, once it is adjusted, then using this command:

```
tmuxp load tests/fuzzing/bc_afl.yaml
```

will start fuzzing.

In other words, to start fuzzing, the sequence is:

```
./scripts/fuzz_prep.sh
tmuxp load tests/fuzzing/bc_afl.yaml
```

Doing that will load, in `tmux`, 16 separate instances of [AFL++][125], 12 on
`bc` and 4 on `dc`. The outputs will be put into the
`tests/fuzzing/bc_outputs{1,2,3}/` and `tests/fuzzing/dc_outputs/` directories.

Sometimes, [AFL++][125] will report crashes when there are none. When crashes
are reported, I always run the following command:

```
./scripts/afl.py <dir>
```

where `dir` is one of `bc1`, `bc2`, `bc3`, or `dc`, depending on which of the
16 instances reported the crash. If it was one of the first four (`bc11` through
`bc14`), I use `bc1`. If it was one of the second four (`bc21` through `bc24`, I
use `bc2`. If it was one of the third four (`bc31` through `bc34`, I use `bc3`.
And if it was `dc`, I use `dc`.

The [`scripts/afl.py`][94] script will report whether [AFL++][125] correctly
reported a crash or not. If so, it will copy the crashing test case to
`.test.txt` and tell you whether it was from running it as a file or through
`stdin`.

From there, I personally always investigate the crash and fix it. Then, when the
crash is fixed, I either move `.test.txt` to `tests/bc/errors/<idx>.txt` as an
error test (if it produces an error) or I create a new `tests/bc/misc<idx>.txt`
test for it and a corresponding results file. (See [Test Suite][124] for more
information about the test suite.) In either case, `<idx>` is the next number
for a file in that particular place. For example, if the last file in
`tests/bc/errors/` is `tests/bc/errors/18.txt`, I move `.test.txt` to
`tests/bc/error/19.txt`.

Then I immediately run [`scripts/afl.py`][94] again to find the next crash
because often, [AFL++][125] found multiple test cases that trigger the same
crash. If it finds another, I repeat the process until it is happy.

Once it *is* happy, I do the same `fuzz_prep.sh`, `tmuxp load` sequence and
restart fuzzing. Why do I restart instead of continuing? Because with the
changes, the test outputs could be stale and invalid.

However, there *is* a case where I continue: if [`scripts/afl.py`][94] finds
that every crash reported by [AFL++][125] is invalid. If that's the case, I can
just continue with the command:

```
tmuxp load tests/fuzzing/bc_afl_continue.yaml
```

(Note: I admit that I usually run [`scripts/afl.py`][94] while the fuzzer is
still running, so often, I don't find a need to continue since there was no
stop. However, the capability is there, if needed.)

In addition, my fuzzing setup, including the `tmux` and `tmuxp` configs,
automatically set up [AFL++][125] power schedules (see [Fuzzing
Performance][127] above). They also set up the parallel fuzzing such that there
is one fuzzer in each group of 4 that does deterministic fuzzing. It's always
the first one in each group.

For more information about deterministic fuzzing, see the [AFL++][125]
documentation.

### Corpuses

I occasionally add to the input corpuses. These files come from new files in the
[Test Suite][124]. In fact, I use soft links when the files are the same.

However, when I add new files to an input corpus, I sometimes reduce the size of
the file by removing some redundancies.

### [AFL++][125] Quickstart

The way [AFL++][125] works is complicated.

First, it is the one to invoke the compiler. It leverages the compiler to add
code to the binary to help it know when certain branches are taken.

Then, when fuzzing, it uses that branch information to generate information
about the "path" that was taken through the binary.

I don't know what AFL++ counts as a new path, but each new path is added to an
output corpus, and it is later used as a springboard to find new paths.

This is what makes AFL++ so effective: it's not just blindly thrashing a binary;
it adapts to the binary by leveraging information about paths.

### Fuzzing Runs

For doing a fuzzing run, I expect about a week or two where my computer is
basically unusable, except for text editing and light web browsing.

Yes, it can take two weeks for me to do a full fuzzing run, and that does *not*
include the time needed to find and fix crashes; it only counts the time on the
*last* run, the one that does not find any crashes. This means that the entire
process can take a month or more.

What I use as an indicator that the fuzzing run is good enough is when the
number of "Pending" paths (see [AFL++ Quickstart][129] above) for all fuzzer
instances, except maybe the deterministic instances, is below 50. And even then,
I try to let deterministic instances get that far as well.

You can see how many pending paths are left in the "path geometry" section of
the [AFL++][125] dashboard.

Also, to make [AFL++][125] quit, you need to send it a `SIGINT`, either with
`Ctrl+c` or some other method. It will not quit until you tell it to.

### Radamsa

I rarely use [Radamsa][99] instead of [AFL++][125]. In fact, it's only happened
once.

The reason I use [Radamsa][99] instead of [AFL++][125] is because it is easier
to use with varying command-line arguments, which was needed for testing `bc`'s
command-line expression parsing code, and [AFL++][125] is best when testing
input from `stdin`.

[`scripts/radamsa.sh`][100] does also do fuzzing on the [AFL++][125] inputs, but
it's not as effective at that, so I don't really use it for that either.

[`scripts/radamsa.sh`][100] and [Radamsa][99] were only really used once; I have
not had to touch the command-line expression parsing code since.

### [AddressSanitizer][21] with Fuzzing

One advantage of using [AFL++][125] is that it saves every test case that
generated a new path (see [AFL++ Quickstart][129] above), and it doesn't delete
them when the user makes it quit.

Keeping them around is not a good idea, for several reasons:

* They are frequently large.
* There are a lot of them.
* They go stale; after `bc` is changed, the generated paths may not be valid
  anymore.

However, before they are deleted, they can definitely be leveraged for even
*more* bug squashing by running *all* of the paths through a build of `bc` with
[AddressSanitizer][21].

This can easily be done with these four commands:

```
./scripts/fuzz_prep.sh -a
./scripts/afl.py --asan bc1
./scripts/afl.py --asan bc2
./scripts/afl.py --asan bc3
./scripts/afl.py --asan dc
```

(By the way, the last four commands could be run in separate terminals to do the
processing in parallel.)

These commands build an [ASan][21]-enabled build of `bc` and `dc` and then they
run `bc` and `dc` on all of the found crashes and path output corpuses. This is
to check that no path or crash has found any memory errors, including memory
leaks.

Because the output corpuses can contain test cases that generate infinite loops
in `bc` or `dc`, [`scripts/afl.py`][94] has a timeout of 8 seconds, which is far
greater than the timeout that [AFL++][125] uses and should be enough to catch
any crash.

If [AFL++][125] fails to find crashes *and* [ASan][21] fails to find memory
errors on the outputs of [AFL++][125], that is an excellent indicator of very
few bugs in `bc`, and a release can be made with confidence.

## Code Concepts

This section is about concepts that, if understood, will make it easier to
understand the code as it is written.

The concepts in this section are not found in a single source file, but they are
littered throughout the code. That's why I am writing them all down in a single
place.

### [Async-Signal-Safe][115] Signal Handling

TODO

* Async-signal-safe functions in handlers.
* `volatile sig_atomic_t`.
* Setting flags to specific values, to make it atomic. (No adds because it
  might not be atomic at that point.)

### Asserts

If you asked me what procedure is used the most in `bc`, I would reply without
hesitation, "`assert()`."

I use `assert()` everywhere. In fact, it is what made fuzzing with [AFL++][125]
so effective. [AFL++][125] is incredibly good at finding crashes, and a failing
`assert()` counts as one.

So while a lot of bad bugs might have corrupted data and *not* caused crashes,
because I put in so many `assert()`'s, they were *turned into* crashing bugs,
and [AFL++][125] found them.

By far, the most bugs it found this way was in the `bc` parser. (See the [`bc`
Parsing][110] for more information.) And even though I was careful to put
`assert()`'s everywhere, most parser bugs manifested during execution of
bytecode because the virtual machine assumes the bytecode is valid.

Sidenote: one of those bugs caused an infinite recursion when running the sine
(`s()`) function in the math library, so yes, parser bugs can be *very* weird.

Anyway, they way I did `assert()`'s was like this: whenever I realized that I
had put assumptions into the code, I would put an `assert()` there to test it
**and** to *document* it.

Yes, documentation. In fact, by far the best documentation of the code in `bc`
is actually the `assert()`'s. The only time I would not put an `assert()` to
test an assumption is if that assumption was already tested by an `assert()`
earlier.

As an example, if a function calls another function and passes a pointer that
the caller previously `assert()`'ed was *not* `NULL`, then the callee does not
have to `assert()` it too, unless *also* called by another function that does
not `assert()` that.

At first glance, it may seem like putting asserts for pointers being non-`NULL`
everywhere would actually be good, but unfortunately, not for fuzzing. Each
`assert()` is a branch, and [AFL++][125] rates its own effectiveness based on
how many branches it covers. If there are too many `assert()`'s, it may think
that it is not being effective and that more fuzzing is needed.

This means that `assert()`'s show up most often in two places: function
preconditions and function postconditions.

Function preconditions are `assert()`'s that test conditions relating to the
arguments a function was given. They appear at the top of the function, usually
before anything else (except maybe initializing a local variable).

Function postconditions are `assert()`'s that test the return values or other
conditions when a function exits. These are at the bottom of a function or just
before a `return` statement.

The other `assert()`'s cover various miscellaneous assumptions.

If you change the code, I ***HIGHLY*** suggest that you use `assert()`'s to
document your assumptions. And don't remove them when [AFL++][125] gleefully
crashes `bc` and `dc` over and over again.

### Vectors

In `bc`, vectors mean resizable arrays, and they are the most fundamental piece
of code in the entire codebase.

I had previously written a [vector implementation][112], which I used to guide
my decisions, but I wrote a new one so that `bc` would not have a dependency. I
also didn't make it as sophisticated; the one in `bc` is very simple.

Vectors store some information about the type that they hold:

* The size (as returned by `sizeof`).
* The destructor.

If the destructor is `NULL`, it is counted as the type not having a destructor.

But by storing the size, the vector can then allocate `size * cap` bytes, where
`cap` is the capacity. Then, when growing the vector, the `cap` is doubled again
and again until it is bigger than the requested size.

But to store items, or to push items, or even to return items, the vector has to
figure out where they are, since to it, the array just looks like an array of
bytes.

It does this by calculating a pointer to the underlying type with
`v + (i * size)`, where `v` is the array of bytes, `i` is the index of the
desired element, and `size` is the size of the underlying type.

Doing that, vectors can avoid undefined behavior (because `char` pointers can
be cast to any other pointer type), while calculating the exact position of
every element.

Because it can do that, it can figure out where to push new elements by
calculating `v + (len * size)`, where `len` is the number of items actually in
the vector.

By the way, `len` is different from `cap`. While `cap` is the amount of storage
*available*, `len` is the number of actual elements in the vector at the present
point in time.

Growing the vector happens when `len` is equal to `cap` *before* pushing new
items, not after.

#### Pointer Invalidation

There is one big danger with the vectors as currently implemented: pointer
invalidation.

If a piece of code receives a pointer from a vector, then adds an item to the
vector before they finish using the pointer, that code must then update the
pointer from the vector again.

This is because any pointer inside the vector is calculated based off of the
array in the vector, and when the vector grows, it can `realloc()` the array,
which may move it in memory. If that is done, any pointer returned by
`bc_vec_item()`, `bc_vec_top()` and `bc_vec_item_rev()` will be invalid.

This fact was the single most common cause of crashes in the early days of this
`bc`; wherever I have put a comment about pointers becoming invalidated and
updating them with another call to `bc_vec_item()` and friends, *do **NOT**
remove that code!*

#### Maps

Maps in `bc` are...not.

They are really a combination of two vectors. Those combinations are easily
recognized in the source because one vector is named `<name>s` (plural), and the
other is named `<name>_map`.

There are currently three, all in `BcProgram`:

* `fns` and `fn_map` (`bc` functions).
* `vars` and `var_map` (variables).
* `arrs` and `arr_map` (arrays).

They work like this: the `<name>_map` vector holds `BcId`'s, which just holds a
string and an index. The string is the name of the item, and the index is the
index of that item in the `<name>s` vector.

Obviously, I could have just done a linear search for items in the `<name>s`
vector, but that would be slow with a lot of functions/variables/arrays.
Instead, I ensure that whenever an item is inserted into the `<name>_map`
vector, the item is inserted in sorted order. This means that the `<name>_map`
is always sorted (by the names of the items).

So when looking up an item in the "map", what is really done is this:

1.	A binary search is carried out on the names in the `<name>_map` vector.
2.	When one is found, it returns the index in the `<name>_map` vector where the
	item was found.
3.	This index is then used to retrieve the `BcId`.
4.	The index from the `BcId` is then used to index into the `<name>s` vector,
	which returns the *actual* desired item.

Why were the `<name>s` and `<name>_map` vectors not combined for ease? The
answer is that sometime, when attempting to insert into the "map", code might
find that something is already there. For example, a function with that name may
already exist, or the variable might already exist.

If the insert fails, then the name already exists, and the inserting code can
forego creating a new item to put into the vector. However, if there is no item,
the inserting code must create a new item and insert it.

If the two vectors were combined together, it would not be possible to separate
the steps such that creating a new item could be avoided if it already exists.

### Command-Line History

TODO

### Error Handling

TODO

* Note about vectors and numbers needing special treatment for error handling.

The error handling on `bc` got an overhall for version [`3.0.0`][32], and it
became one of the things that taught me the most about C in particular and
programming in general.

Before then, error handling was manual. Almost all functions returned a
`BcStatus` indicating if an error had occurred. This led to a proliferation of
lines like:

```
if (BC_ERR(s)) return s;
```

In fact, a quick and dirty count of such lines in version `2.7.2` (the last
version before [`3.0.0`][32] turned up 252 occurrences of that sort of line.

And that didn't even guarantee that return values were checked *everywhere*.

But before I can continue, let me back up a bit.

From the beginning, I decided that I would not do what GNU `bc` does on errors;
it tries to find a point at which it can recover. Instead, I decided that I
would have `bc` reset to a clean slate, which I believed, would reduce the
number of bugs where an unclean state caused errors with continuing execution.

So from the beginning, errors would essentially unwind the stack until they got
to a safe place from which to clean the slate, reset, and ask for more input.

Well, if that weren't enough, `bc` also has to handle [POSIX signals][113]. As
such, it had a signal handler that set a flag. But it could not safely interrupt
execution, so that's all it could do.

In order to actually respond to the signal, I had to litter checks for the flag
*everywhere* in the code. And I mean *everywhere*. They had to be checked on
every iteration of *every* loop. They had to be checked going into and out of
certain functions.

It was a mess.

But fortunately for me, signals did the same thing that errors did: they unwound
the stack to the *same* place.

Do you see where I am going with this?

It turns out that what I needed was a [async-signal-safe][115] form of what
programmers call "exceptions" in other languages.

I knew that [`setjmp()`][116] and [`longjmp()`][117] are used in C to implement
exceptions, so I thought I would learn how to use them. How hard could it be?

Quite hard, it turns out, especially in the presence of signals. And that's
because there are many snares:

1.	The value of any local variables are not guaranteed to be preserved after a
	`longjmp()` back into a function.
2.	While `longjmp()` is required to be [async-signal-safe][115], if it is
	invoked by a signal handler that interrupted a non-[async-signal-safe][115]
	function, then the behavior is undefined.

Oh boy.

For number 1, the answer to this is to hide data that must stay changed behind
pointers. Only the *pointers* are considered local, so as long as I didn't do
any modifying pointer arithmetic, pointers and their data would be safe. For
cases where I have local data that must change and stay changed, I needed to
*undo* the `setjmp()`, do the change, and the *redo* the `setjmp()`.

#### Custom I/O

TODO

* Talk about interaction with command-line history.

### Execution

TODO

* Bytecode.
* Stack machine.
* `stack` vs. `results`.
* `BcInstPtr` for marking where execution is.
* Variables are arrays, in order to push arguments on them and to implement
  autos.
* Arrays are arrays as well.

#### Bytecode Indices

TODO

### Lexing

TODO

### `dc` Parsing

(In fact, the easiness of parsing [Reverse Polish notation][108] is probably
why it was used for `dc` when it was first created at Bell Labs.)

### `bc` Parsing

`bc`'s parser is, by far, the most sensitive piece of code in this software, and
there is a very big reason for that: `bc`'s standard is awful and defined a very
poor language.

The standard says that either semicolons or newlines can end statements. Trying
to parse the end of a statement when it can either be a newline or a semicolon
is subtle. Doing it in the presence of control flow constructs that do not have
to use braces is even harder.

And then comes the biggest complication of all: `bc` has to assume that it is
*always* at a REPL (Read-Eval-Print Loop). `bc` is, first and foremost, an
*interactive* utility.

#### Flags

All of this means that `bc` has to be able to partially parse something, store
enough data to recreate that state later, and return, making sure to not
execute anything in the meantime.

*That* is what the flags in [`include/bc.h`][106] are: they are the state that
`bc` is saving for itself.

It saves them in a stack, by the way, because it's possible to nest
structures, just like any other programming language. Thus, not only does it
have to store state, it needs to do it arbitrarily, and still be able to
come back to it.

So `bc` stores its parser state with flags in a stack. Careful setting of these
flags, along with properly using them and maintaining the flag stack, are what
make `bc` parsing work, but it's complicated. In fact, as I mentioned, the `bc`
parser is the single most subtle, fickle, and sensitive piece of code in the
entire codebase. Only one thing came close once: square root, and that was only
sensitive because I wrote it wrong. This parser is pretty good, and it is
*still* sensitive. And flags are the reason why.

For more information about what individual flags there are, see the comments in
[`include/bc.h`][106].

#### Labels

`bc`'s language is Turing-complete. That means that code needs the ability to
jump around, specifically to implement control flow like `if` statements and
loops.

`bc` handles this while parsing with what I called "labels."

Labels are markers in the bytecode. They are stored in functions alongside the
bytecode, and they are just indices into the bytecode.

When the `bc` parser creates a label, it pushes an index onto the labels array,
and the index of the label in that array is the index that will be inserted into
the bytecode.

Then, when a jump happens, the index pulled out of the bytecode is used to index
the labels array, and the label (index) at the index is then used to set the
instruction pointer.

#### Cond Labels

"Cond" labels are so-called because they are used by conditionals.

The key to them is that they come *before* the code that uses them. In other
words, when jumping to a condition, code is jumping *backwards*.

This means that when a cond label is created, the value that should go there is
well-known. Cond labels are easy.

However, they are still stored on a stack so that the parser knows what cond
label to use.

#### Exit Labels

Exit labels are not so easy.

"Exit" labels are so-called because they are used by code "exiting" out of `if`
statements or loops.

The key to them is that they come *after* the code that uses them. In other
words, when jumping to an exit, code is jumping *forwards*.

But this means that when an exit label is created, the value that should go
there is *not* known. The code that needs it must be parsed and generated first.

That means that exit labels are created with the index of `SIZE_MAX`, which is
then specifically checked for with an assert in `bc_program_exec()` before using
those indices.

There should ***NEVER*** be a case when an exit label is not filled in properly
if the parser has no bugs. This is because every `if` statement, every loop,
must have an exit, so the exit must be set. If not, there is a bug.

Exit labels are also stored on a stack so that the parser knows what exit label
to use.

#### Expression Parsing

`bc` has expressions like you might expect in a typical programming language.
This means [infix notation][107].

One thing about infix notation is that you can't just generate code straight
from it like you can with [Reverse Polish notation][108]. It requires more work
to shape it into a form that works for execution on a stack machine.

That extra work is called the [Shunting-Yard algorithm][109], and the form it
translates infix notation into is...[Reverse Polish notation][108].

In order to understand the rest of this section, you must understand the
[Shunting-Yard algorithm][109]. Go do that before you read on.

##### Operator Stack

In `bc`, the [Shunting-Yard algorithm][109] is implemented with bytecode as the
output and an explicit operator stack (the `ops` field in `BcParse`) as the
operator stack. It stores tokens from `BcLex`.

However, there is one **HUGE** hangup: multiple expressions can stack. This
means that multiple expressions can be parsed at one time (think an array element
expression in the middle of a larger expression). Because of that, we need to
keep track of where the previous expression ended. That's what `start` parameter
to `bc_parse_operator()` is.

Parsing multiple expressions on one operator stack only works because
expressions can only *stack*; this means that, if an expression begins before
another ends, it must *also* end before that other expression ends. This
property ensures that operators will never interfere with each other on the
operator stack.

##### Recursion

Because expressions can stack, parsing expressions actually requires recursion.
Well, it doesn't *require* it, but the code is much more readable that way.

This recursion is indirect; the functions that `bc_parse_expr_err()` (the actual
expression parsing function) calls can, in turn, call it.

##### Expression Flags

There is one more big thing: not all expressions in `bc` are equal.

Some expressions have requirements that others don't have. For example, only
array arguments can be arrays (which are technically not expressions, but are
treated as such for parsing), and some operators (in POSIX) are not allowed in
certain places.

For this reason, functions that are part of the expression parsing
infrastructure in `bc`'s parser usually take a `flags` argument. This is meant
to be passed to children, and somewhere, they will be checked to ensure that the
resulting expression meets its requirements.

There are also places where the flags are changed. This is because the
requirements change.

Maintaining the integrity of the requirements flag set is an important part of
the `bc` parser. However, they do not have to be stored on a stack because their
stack is implicit from the recursion that expression parsing uses.

### Callbacks

There are many places in `bc` and `dc` where function pointers are used:

* To implement destructors in vectors. (See the [Vectors][111] section.)
* To select the correct lex and parse functions for `bc` and `dc`.
* To select the correct function to execute unary operators.
* To select the correct function to execute binary operators.
* To calculate the correct number size for binary operators.
* To print a "digit" of a number.
* To seed the pseudo-random number generator.

And there might be more.

In every case, they are used for reducing the amount of code. Instead of
`if`/`else` chains, such as:

```
if (BC_IS_BC) {
	bc_parse_parse(vm.parse);
}
else {
	dc_parse_parse(vm.parse);
}
```

The best example of this is `bc_num_binary()`. It is called by every binary
operator. It figures out if it needs to allocate space for a new `BcNum`. If so,
it allocates the space and then calls the function pointer to the *true*
operation.

Doing it like that shrunk the code *immensely*. First, instead of every single
binary operator duplicating the allocation code, it only exists in one place.
Second, `bc_num_binary()` itself does not have a massive `if`/`else` chain or a
`switch` statement.

But perhaps the most important use was for destructors in vectors.

Most of the data structures in `bc` are stored in vectors. If I hadn't made
destructors available for vectors, then ensuring that `bc` had no memory leaks
would have been nigh impossible. As it is, I check `bc` for memory leaks every
release when I change the code, and I have not released `bc` after version
`1.0.0` with any memory leaks, as far as I can remember anyway.

### Numbers

TODO

### Strings as Numbers

TODO

### Pseudo-Random Number Generator

TODO

* Integer portion is increment.
* Real portion is the real seed.

## Debugging

TODO

* `BC_DEBUG_CODE`.
	* Hidden behind a `#define` to ensure it does not leak into actual code.

## Performance

TODO

* `BC_ERR`/`BC_UNLIKELY`
* `BC_NO_ERR`/`BC_LIKELY`
* Link-time optimization.

### Benchmarks

TODO

* Need to generate benchmarks and run them.
* `ministat`.

### Caching of Numbers

TODO

### Slabs and Slab Vectors

TODO

## `bcl`

TODO

* What is included.
* What is *not* included.
* Contexts and why.
* Signal handling.
* Encapsulation of numbers.

[1]: https://en.wikipedia.org/wiki/Bus_factor
[2]: https://pubs.opengroup.org/onlinepubs/9699919799/utilities/bc.html#top
[3]: https://en.wikipedia.org/wiki/Dc_(Unix)
[4]: https://en.wikipedia.org/wiki/Reverse_Polish_notation
[5]: ./bc/A.1.md#standard-library
[6]: https://pubs.opengroup.org/onlinepubs/9699919799/utilities/bc.html#top
[7]: ./bc/A.1.md#extended-library
[8]: #libbc
[9]: #strgensh
[10]: https://vimeo.com/230142234
[11]: https://gavinhoward.com/2019/12/values-for-yao/
[12]: http://www.open-std.org/jtc1/sc22/wg14/www/docs/n1256.pdf
[13]: ./build.md#cross-compiling
[14]: ./build.md
[15]: #strgenc
[16]: http://landley.net/toybox/about.html
[17]: https://www.busybox.net/
[18]: https://en.wikipedia.org/wiki/Karatsuba_algorithm
[19]: https://clang-analyzer.llvm.org/scan-build.html
[20]: https://www.valgrind.org/
[21]: https://clang.llvm.org/docs/AddressSanitizer.html
[22]: https://gavinhoward.com/2019/11/finishing-software/
[23]: #history
[24]: https://clang.llvm.org/docs/ClangFormat.html
[25]: ./algorithms.md
[26]: #lib2bc
[27]: #vmh
[28]: https://github.com/rain-1/linenoise-mob
[29]: https://github.com/antirez/linenoise
[30]: #bclh
[31]: #argsh
[32]: ../NEWS.md#3-0-0
[33]: ../NEWS.md
[34]: https://github.com/skeeto/optparse
[35]: #opth
[36]: #historyh
[37]: #randh
[38]: #langh
[39]: #numc
[40]: #bcc
[41]: #bc_lexc
[42]: #bc_parsec
[43]: #libraryc
[44]: #dcc
[45]: #dc_lexc
[46]: #dc_parsec
[47]: #filec
[48]: #historyc
[49]: #langc
[50]: #lexc
[51]: #optc
[52]: #parsec
[53]: #programc
[54]: #randc
[55]: #fileh
[56]: #readc
[57]: #programh
[58]: #vmc
[59]: https://pubs.opengroup.org/onlinepubs/9699919799/utilities/gencat.html#top
[60]: #manpagesh
[61]: #bcl3md
[62]: #bcl3
[63]: #bclvcxproj
[64]: #bclvcxprojfilters
[65]: #bclsln
[66]: #bcvcxproj
[67]: #bcvcxprojfilters
[68]: #bcsln
[69]: #configuresh
[70]: #makefilein
[71]: #functionsh
[72]: https://pubs.opengroup.org/onlinepubs/9699919799/utilities/sh.html#top
[73]: https://pubs.opengroup.org/onlinepubs/9699919799/utilities/V3_chap02.html#tag_18
[74]: https://pubs.opengroup.org/onlinepubs/9699919799/utilities/make.html#top
[75]: #versionh
[76]: ##posix-shell-scripts
[77]: #tests
[78]: #karatsubapy
[79]: #bc
[80]: #dc
[81]: ./build.md#build-type
[82]: #fuzzing
[83]: #releasesh
[84]: https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/V1_chap08.html#tag_08_02
[85]: #locales
[86]: #manuals
[87]: #locale_installsh
[88]: #locale_uninstallsh
[89]: #bc1mdin
[90]: #dc1mdin
[91]: #bc
[92]: https://pandoc.org/
[93]: #release_settingstxt
[94]: #aflpy
[95]: #randmathpy
[96]: #caching-of-numbers
[97]: #error-handling
[98]: #radamsatxt
[99]: https://gitlab.com/akihe/radamsa
[100]: #radamsash
[101]: https://musl.libc.org/
[102]: ./build.md#settings
[103]: #test_settingstxt
[104]: #test_settingssh
[105]: #functionssh
[106]: #bch
[107]: https://en.wikipedia.org/wiki/Infix_notation
[108]: https://en.wikipedia.org/wiki/Reverse_Polish_notation
[109]: https://en.wikipedia.org/wiki/Shunting-yard_algorithm
[110]: #bc-parsing
[111]: #vectors
[112]: https://git.yzena.com/Yzena/Yc/src/branch/master/include/yc/vector.h
[113]: https://en.wikipedia.org/wiki/Signal_(IPC)
[114]: #custom-io
[115]: https://pubs.opengroup.org/onlinepubs/9699919799/functions/V2_chap02.html#tag_15_04_03_03
[116]: https://pubs.opengroup.org/onlinepubs/9699919799/functions/setjmp.html
[117]: https://pubs.opengroup.org/onlinepubs/9699919799/functions/longjmp.html
[118]: https://www.youtube.com/watch?v=4PaWFYm0kEw
[119]: #fuzz_prepsh
[120]: #bc_aflyaml
[121]: #bc_afl_continueyaml
[122]: https://github.com/tmux/tmux
[123]: https://tmuxp.git-pull.com/
[124]: #test-suite
[125]: https://aflplus.plus/
[126]: #link-time-optimization
[127]: #fuzzing-performance
[128]: #radamsa
[129]: #afl-quickstart
[130]: #convenience
[131]: #datac
[132]: https://git.yzena.com/gavin/vim-bc
[133]: https://git.yzena.com/gavin/bc_libs
[134]: #debugging
[135]: #asserts
[136]: #portability
