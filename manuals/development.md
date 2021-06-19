# Development

This document is supposed to contain all of the knowledge necessary to develop
`bc` and `dc`.

This document is meant for the day when I (Gavin D. Howard) get [hit by a
bus][1]. In other words, it's meant to make the [bus factor][1] a non-issue.

This document will reference other parts of the repository. That is so a lot of
the documentation can be closest to the part of the repo where it is actually
necessary.

## What Is It?

This repository contains an implementation of both [POSIX `bc`][2] and [Unix
`dc`][3].

POSIX `bc` is a standard utility required for POSIX systems. `dc` is a
historical utility that was included in early Unix. They both are
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

### ClangFormat

TODO: Attempt once more to make ClangFormat work.

I attempted twice to use [ClangFormat][24] to impose a standard, machine-useful
style on `bc`. Both failed. Otherwise, the style in this repo would be more
consistent.

## Repo Structure

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

#### `lib2.bc`

TODO: Document algorithms.

A `bc` script containing the [extended math library][7].

Like [`lib.bc`][8], and for the same reasons, this file should have no
extraneous whitespace, except for tabs at the beginning of lines.

For more details about the algorithms used, see the [algorithms manual][25].

#### `strgen.c`

TODO: Document actual source file.

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

TODO: Document actual source file.

An `sh` script that will generate C strings that uses only POSIX utilities. This
exists for those situations where a host C99 compiler is not available, and the
environment limits mentioned above in [`strgen.c`][15] don't matter.

`strgen.sh` takes the same arguments as [`strgen.c`][15], and the arguments mean
the exact same things, so see the comments in [`strgen.c`][15] for more detail
about that, and see the comments in `strgen.sh` for more details about it and
how it works.

## TODO

* Document all code assumptions with asserts.
* Document all functions with Doxygen comments.
* The purpose of every file.
* How locale works.
	* How locales are installed.
	* How the locales are used.
* Why generated manpages (including markdown) are checked into git.
* How all manpage versions are generated.
* Fuzzing.
	* Including my `tmuxp` files.
	* Can't use `libdislocator.so`.
	* Use `AFL_HARDEN` during build for hardening.
	* Use `CC=afl-clang-lto` and `CFLAGS="-flto"`.

[1]: https://en.wikipedia.org/wiki/Bus_factor
[2]: https://pubs.opengroup.org/onlinepubs/9699919799/utilities/bc.html#top
[3]: https://en.wikipedia.org/wiki/Dc_(Unix)
[4]: https://en.wikipedia.org/wiki/Reverse_Polish_notation
[5]: ../manuals/A.1.md#standard-library
[6]: https://pubs.opengroup.org/onlinepubs/9699919799/utilities/bc.html#top
[7]: ../manuals/A.1.md#extended-library
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
