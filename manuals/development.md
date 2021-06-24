# Development

This document is meant for the day when I (Gavin D. Howard) get [hit by a
bus][1]. In other words, it's meant to make the [bus factor][1] a non-issue.

This document is supposed to contain all of the knowledge necessary to develop
`bc` and `dc`.

This document will reference other parts of the repository. That is so a lot of
the documentation can be closest to the part of the repo where it is actually
necessary.

## What Is It?

This repository contains an implementation of both [POSIX `bc`][2] and [Unix
`dc`][3].

POSIX `bc` is a standard utility required for POSIX systems. `dc` is a
historical utility that was included in early Unix and even predates both Unix
and C. They both are
arbitrary-precision command-line calculators with their own programming
languages. `bc`'s language looks similar to C, with infix notation and includes
functions, while `dc` uses [Reverse Polish Notation][4] and allows the user to
execute strings as though they were functions.

In addition, it is also possible to build the arbitrary-precision math into a
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

## Code Style

TODO

The code style for `bc` is...weird, and that comes from historical accident.

In [History][23], I mentioned how I got my `bc` in toybox. Well, in order to do
that, my `bc` originally had toybox style. Eventually, I changed to using tabs,
and assuming they were 4 spaces wide, but other than that, I basically kept the
same style, with some exceptions that are more or less dependent on my taste.

The code style is as follows:

* Tabs are 4 spaces.
* Tabs are used at the beginning of lines for indent.
* Spaces are used for alignment.
* Lines are limited to 80 characters, period.
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
  one line long, and/or, the body cannot fit into 80 characters.
* If single-line bodies are on a separate line from their headers, and the
  headers are only a single line, then no braces are used.
* However, braces are *always* used if they contain another `if` statement or
  loop.
* Loops with empty bodies are ended with a semicolon.
* Expressions that return a boolean value are surrounded by paretheses.
* Macro backslashes are aligned as far to the left as possible.
* Binary operators have spaces on both sides.
* If a line with binary operators overflows 80 character, a newline is inserted
  *after* binary operators.
* Function modifiers and return types are on the same line as the function name.
* With one exception, `goto`'s are only used to jump to the end of a function
  for cleanup.

### ClangFormat

I attempted twice to use [ClangFormat][24] to impose a standard, machine-useful
style on `bc`. Both failed. Otherwise, the style in this repo would be more
consistent.

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
*portability*.

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

Because of portability, the generated `Makefile.in` should be a pure [POSIX
`make`][74]-compatible `Makefile` (minus the placeholders). Here are a few
snares for the unwary programmer in this file:

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
[Error Handling][97], along with [`vm.h`][27] and the notes about version
[`3.0.0`][32] in the [`NEWS`][32].

The code associated with this header is in [`src/file.c`][47].

#### `history.h`

This header is for `bc`'s implementation of command-line editing/history, which
is based on a [UTF-8-aware fork][28] of [`linenoise`][29].

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

This header has a few things:

* Compiler-specific fixes.
* Platform-specific fixes.
* A list of possible errors that internal `bc` code can use.

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
was last updated when version `3.0.0` was released.

It has very little documentation value, other than showing what compiler options
are useful for performance.

#### `build.md`

This is the [build manual][14].

This `bc` has a custom build system. The reason for this is because of
*portability*.

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
* Compilers and their quirks, as well as warning settings on Clang.
	* My various `bc` aliases.
* My vim-bc repo.
* The purpose of every file.
* How locale works.
	* How locales are installed.
	* How the locales are used.
* Why generated manpages (including markdown) are checked into git.
* How all manpage versions are generated.
* Fuzzing.
	* Including my `tmuxp` files.
	* Can't use `libdislocator.so`. It causes crashes when it can't allocate
	  memory.
	* Use `AFL_HARDEN` during build for hardening.
	* Use `CC=afl-clang-lto` and `CFLAGS="-flto"`.

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
under all of the paths found by AFL++.

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

The reason I use [Radamsa][99] instead of AFL++ is because it is easier to use
with varying command-line arguments, which is what's needed here. (AFL++ is best
when testing input from `stdin`.)

This script does also do fuzzing on the AFL++ inputs, but it's not as effective
at that, so I don't really use it for that either.

This script was only really used once; I have not had to touch the command-line
expression parsing code since.

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

### `tests/`

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
functions are required to change variables in the parent script.

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
[`scripts/functions.sh`][105], and the parts of the build system needed to
activate it. There is also code in [`src/vm.c`][58] for loading the current
locale.

## Fuzzing

## Code Concepts

This section is about concepts that, if understood, will make it easier to
understand the code as it is written.

The concepts in this section are not found in a single source file, but they are
littered throughout the code. That's why I am writing them all down in a single
place.

### Error Handling

### Lexing

### Parsing

### Bytecode

#### Bytecode Indices

### Function Pointers

### Strings as Numbers

### Caching of Numbers

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
