# bc

This is an implementation of POSIX `bc` that implements
[GNU `bc`](https://www.gnu.org/software/bc/) extensions, as well as the period
(`.`) extension for the BSD bc.

This `bc` is Free and Open Source Software (FOSS). It is licensed under the BSD
0-clause License.

# Build

To build, use the following commands:

```
make release
make install
```

To use a non-default compiler, replace the first command with:

```
CC=<compiler> make release
```

To install into a non-default (the default is `/usr/local`) prefix, change the
second command to:

```
PREFIX=<prefix> make install
```

To make a minimum size release, use `make minrelease` instead of `make release`.
To make a debug release, use `make reldebug`.

## Status

This `bc` is in alpha stage, so it is ready for testing in the wild, but it is
not ready to be officially distributed as an official part of any project.

## Language

This `bc` is written in pure ISO C99.

## Git Workflow

This `bc` uses the git workflow described in
[this post](http://endoflineblog.com/oneflow-a-git-branching-model-and-workflow).
Developers who want to contribute are encouraged to read that post carefully.

For feature branches, it uses `rebase + merge --no-ff` (option 3). It also uses
a `develop`/`master` split. (Main development is on `develop`, and `master` just
points to the latest tagged release to make it easy for users to get the latest
release.)

## Commit Messages

This `bc` uses the commit message guidelines laid out in
[this blog post](http://tbaggery.com/2008/04/19/a-note-about-git-commit-messages.html).

## Semantic Versioning

This `bc` uses [semantic versioning](http://semver.org/).

## Contents Listing

Every folder contains a README file which lists the purposes for the files and
folders in that directory.

## Contents

Files:

	LICENSE.md      A Markdown the BSD 0-clause License.
	NOTICE.md       List of contributors and copyright owners.

Folders:

	docs        Contains all of the documentation (currently empty).
	include     Contains all of the public header files.
	src         All source code.
	tests       Tests.
