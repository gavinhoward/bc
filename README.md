# `bc`

This is an implementation of POSIX `bc` that implements
[GNU `bc`](https://www.gnu.org/software/bc/) extensions, as well as the period
(`.`) extension for the BSD bc.

This `bc` also includes an implementation of `dc` in the same binary, which
includes all FreeBSD and GNU extensions. However, it does not have the `!`
command; the author does not want to deal with the security implications, and it
is unnecessary in this day of full-featured shells.

This `bc` is Free and Open Source Software (FOSS). It is licensed under the BSD
0-clause License.

## Build

To build, use the following commands:

```
make [bc|dc]
make install
```

This `bc` supports `CC`, `CFLAGS`, `CPPFLAGS`, `LDFLAGS`, `LDLIBS`, `PREFIX`,
and `DESTDIR` `make` variables. Users can also create a file named `config.mak`
in the root directory to control `make`.

There is also a `make help` command to list all targets and options.

## Status

This `bc` is in beta stage. It has been well-tested and fuzzed, but has not yet
been proven in the wild.

## Language

This `bc` is written in pure ISO C99.

## Commit Messages

This `bc` uses the commit message guidelines laid out in
[this blog post](http://tbaggery.com/2008/04/19/a-note-about-git-commit-messages.html).

## Semantic Versioning

This `bc` uses [semantic versioning](http://semver.org/).

## Contents

Files:

	install.sh       Install script.
	karatsuba.py     Script for distro and package maintainers to find the optimal Karatsuba number.
	LICENSE.md       A Markdown form of the BSD 0-clause License.
	link.sh          A script to link dc to bc.
	Makefile         The Makefile.
	NOTICE.md        List of contributors and copyright owners.
	RELEASE.md       A checklist for making a release.
	safe-install.sh  Safe install script from musl libc.

Folders:

	dist     Files to cut a release for toybox/busybox. (Maintainer use only.)
	gen      The bc math library, help texts, and code to generate C source.
	include  All header files.
	src      All source code.
	tests    All tests.
