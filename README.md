# bc

This is an implementation of POSIX `bc` that implements
[GNU `bc`](https://www.gnu.org/software/bc/) extensions.

This `bc` is Free and Open Source Software (FOSS). It is licensed under the BSD
3-clause License, with a special exemption for the Toybox project to use it
under the BSD 0-clause License.

Because this `bc` makes use of [`arbprec`](https://github.com/cmgraff/arbsh) by
[cmgraff](https://github.com/cmgraff) and [hexingb](https://github.com/hexingb)
et al, this `bc` is a collaborative effort. All `arbprec` contributors are
considered `bc` contributors.

## Status

This `bc` is not even in alpha stage yet, so it is not ready for use. However,
at this time, it can do basic math operations (`+`, `-`, `*`, `/`, `%`) on
constants.

## Cloning

After cloning, make sure to run `git submodule update --init --recursive` to
clone the `arbprec` submodule.

## Language

This `bc` is written in pure ISO C11.

## Git Workflow

This `bc` uses the git workflow described in
[this post](http://endoflineblog.com/oneflow-a-git-branching-model-and-workflow).
Developers who want to contribute are encouraged to read that post carefully.

For feature branches, it uses `rebase + merge --no--ff` (option 3). It also uses
a `master`/`release` split. (Main development is on `master`, and `release` just
points to the latest tagged release to make it easy for users to get the latest
release.)

This `bc` includes scripts and a `.gitconfig` that helps manage the workflow. New
contributors should familiarize themselves with them.

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

	LICENSE.md      A Markdown the BSD 3-clause License.
	NOTICE.md       List of contributors and copyright owners.

Folders:

	docs        Contains all of the documentation (currently empty).
	include     Contains all of the public header files.
	src         All source code.
	tests       Tests.
