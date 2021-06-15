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

## TODO

* The purpose of every file.
* How locale works.
	* How locales are installed.
	* How the locales are used.
* Why generated manpages (including markdown) are checked into git.
* How all manpage versions are generated.

[1]: https://en.wikipedia.org/wiki/Bus_factor
[2]: https://pubs.opengroup.org/onlinepubs/9699919799/utilities/bc.html#top
[3]: https://en.wikipedia.org/wiki/Dc_(Unix)
[4]: https://en.wikipedia.org/wiki/Reverse_Polish_notation
