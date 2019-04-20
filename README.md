# `bc`

[![Build Status][13]][14]
[![codecov][15]][16]
[![Coverity Scan Build Status][17]][18]

This is an implementation of the [POSIX `bc` calculator][12] that implements
[GNU `bc`][1] extensions, as well as the period (`.`) extension for the BSD
flavor of `bc`.

For more information, see this `bc`'s [full manual][2].

This `bc` also includes an implementation of `dc` in the same binary, accessible
via a symbolic link, which implements all FreeBSD and GNU extensions. If a
single `dc` binary is desired, `bc` can be copied and renamed to `dc`. The `!`
command is omitted; I believe this poses security concerns and that such
functionality is unnecessary.

For more information, see the `dc`'s [full manual][3].

This `bc` is Free and Open Source Software (FOSS). It is offered under the BSD
2-clause License. Full license text may be found in the [`LICENSE.md`][4] file.

## Prerequisites

This `bc` only requires a C99-compatible compiler and a (mostly) POSIX
2008-compatible system with the XSI (X/Open System Interfaces) option group.

Since POSIX 2008 with XSI requires the existence of a C99 compiler as `c99`, any
POSIX and XSI-compatible system will have everything needed.

Systems that are known to work:

* Linux
* FreeBSD
* OpenBSD
* Mac OSX

Please submit bug reports if this `bc` does not build out of the box on any
system besides Windows. If Windows binaries are needed, they can be found at
[xstatic][6].

## Build

This `bc` should build unmodified on any POSIX-compliant system.

For more complex build requirements than the ones below, see the
[build manual][5].

### Pre-built Binaries

It is possible to download pre-compiled binaries for a wide list of platforms,
including Linux- and Windows-based systems, from [xstatic][6]. This link always
points to the latest release of `bc`.

### Default

For the default build with optimization, use the following commands in the root
directory:

```
./configure.sh -O3
make
```

### One Calculator

To only build `bc`, use the following commands:

```
./configure.sh --disable-dc
make
```

To only build `dc`, use the following commands:

```
./configure.sh --disable-bc
make
```

### Debug

For debug builds, use the following commands in the root directory:

```
./configure.sh -g
make
```

### Install

To install, use the following command:

```
make install
```

By default, `bc` and `dc` will be installed in `/usr/local`. For installing in
other locations, see the [build manual][5].

## Status

This `bc` is robust.

It is well-tested, fuzzed, and fully standards-compliant (though not certified)
with POSIX `bc`. The math has been tested with 40+ million random problems, so
it is as correct as I can make it.

This `bc` can be used as a drop-in replacement for any existing `bc`. This `bc`
is also compatible with MinGW toolchains, though history is not supported on
Windows.

### Performance

This `bc` has similar performance to GNU `bc`. It is slightly slower on certain
operations and slightly faster on others. Full benchmark data are not yet
available.

## Algorithms

To see what algorithms this `bc` uses, see the [algorithms manual][7].

## Locales

Currently, this `bc` only has support for English (and US English), French and 
German locales. Patches are welcome for translations; use the existing `*.msg` 
files in `locales/` as a starting point.

The message files provided assume that locales apply to all regions where a 
language is used, but this might not be true for, e.g., `fr_CA` and `fr_CH`. 
Any corrections or a confirmation that the current texts are acceptable for 
those regions would be appreciated, too.

## Other Projects

Other projects based on this bc are:

* [busybox `bc`][8]. The busybox maintainers have made their own changes, so any
  bugs in the busybox `bc` should be reported to them.

* [toybox `bc`][9]. The maintainer has also made his own changes, so bugs in the
  toybox `bc` should be reported there.

## Language

This `bc` is written in pure ISO C99, using POSIX 2008 APIs.

## Commit Messages

This `bc` uses the commit message guidelines laid out in [this blog post][10].

## Semantic Versioning

This `bc` uses [semantic versioning][11].

## Contents

Items labeled with `(maintainer use only)` are not included in release source
tarballs.

Files:

	.gitignore           The git ignore file (maintainer use only).
	.travis.yml          The Travis CI file (maintainer use only).
	codecov.yml          The Codecov file (maintainer use only).
	configure.sh         The configure script.
	functions.sh         A script with functions used by other scripts.
	install.sh           Install script.
	karatsuba.py         Script to find the optimal Karatsuba number.
	LICENSE.md           A Markdown form of the BSD 2-clause License.
	link.sh              A script to link dc to bc.
	locale_install.sh    A script to install locales, if desired.
	locale_uninstall.sh  A script to uninstall locales.
	Makefile.in          The Makefile template.
	NOTICE.md            List of contributors and copyright owners.
	RELEASE.md           A checklist for making a release (maintainer use only).
	release.sh           A script to test for release (maintainer use only).
	safe-install.sh      Safe install script from musl libc.

Folders:

	gen      The bc math library, help texts, and code to generate C source.
	include  All header files.
	locales  Locale files, in .msg format. Patches welcome for translations.
	manuals  Manuals for both programs.
	src      All source code.
	tests    All tests.

[1]: https://www.gnu.org/software/bc/
[2]: ./manuals/bc.md
[3]: ./manuals/dc.md
[4]: ./LICENSE.md
[5]: ./manuals/build.md
[6]: https://pkg.musl.cc/bc/
[7]: ./manuals/algorithms.md
[8]: https://git.busybox.net/busybox/tree/miscutils/bc.c
[9]: https://github.com/landley/toybox/blob/master/toys/pending/bc.c
[10]: http://tbaggery.com/2008/04/19/a-note-about-git-commit-messages.html
[11]: http://semver.org/
[12]: https://pubs.opengroup.org/onlinepubs/9699919799/utilities/bc.html
[13]: https://travis-ci.com/gavinhoward/bc.svg?branch=master
[14]: https://travis-ci.com/gavinhoward/bc
[15]: https://codecov.io/gh/gavinhoward/bc/branch/master/graph/badge.svg
[16]: https://codecov.io/gh/gavinhoward/bc
[17]: https://img.shields.io/coverity/scan/16609.svg
[18]: https://scan.coverity.com/projects/gavinhoward-bc
