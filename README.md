# `bc`

This is an implementation of POSIX `bc` that implements [GNU `bc`][1]
extensions, as well as the period (`.`) extension for the BSD flavor of `bc`.

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

Please submit bug reports if this `bc` does not build out of the box on other
systems.

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
make install
```

### Debug

For debug builds, use the following commands in the root directory:

```
./configure.sh -g
make
make install
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

	configure.sh     The configure script.
	install.sh       Install script.
	karatsuba.py     Script for package maintainers to find the optimal Karatsuba number.
	LICENSE.md       A Markdown form of the BSD 2-clause License.
	link.sh          A script to link dc to bc.
	Makefile.in      The Makefile template.
	NOTICE.md        List of contributors and copyright owners.
	RELEASE.md       A checklist for making a release (maintainer use only).
	release.sh       A script to run during the release process (maintainer use only).
	safe-install.sh  Safe install script from musl libc.

Folders:

	gen      The bc math library, help texts, and code to generate C source.
	include  All header files.
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
