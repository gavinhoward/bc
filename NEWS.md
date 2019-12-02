# News

## 2.4.0

This is a production release primarily aimed at improving `dc`.

* A couple of copy and paste errors in the [`dc` manual][10] were fixed.
* `dc` startup was optimized by making sure it didn't have to set up `bc`-only
  things.
* The `bc` `&&` and `||` were made available to `dc` through the `M` and `m`
  commands, respectively.
* `dc` macros were changed to be tail call-optimized.

The last item, tail call optimization, means that if the last thing in a macro
is a call to another macro, then the old macro is popped before executing the
new macro. This change was made to stop `dc` from consuming more and more memory
as macros are executed in a loop.

The `q` and `Q` commands still respect the "hidden" macros by way of recording
how many macros were removed by tail call optimization.

## 2.3.2

This is a production release meant to fix warnings in the Gentoo `ebuild` by
making it possible to disable binary stripping. Other users do *not* need to
upgrade.

## 2.3.1

This is a production release. It fixes a bug that caused `-1000000000 < -1` to
return `0`. This only happened with negative numbers and only if the value on
the left was more negative by a certain amount.

## 2.3.0

This is a production release with changes to the build system.

## 2.2.0

This release is a production release. It only has new features and performance
improvements.

1.	The performance of `sqrt(x)` was improved.
2.	The new function `root(x, n)` was added to the extended math library to
	calculate `n`th roots.
3.	The new function `cbrt(x)` was added to the extended math library to
	calculate cube roots.

## 2.1.3

This is a non-critical release; it just changes the build system, and in
non-breaking ways:

1.	Linked locale files were changed to link to their sources with a relative
	link.
2.	A bug in `configure.sh` that caused long option parsing to fail under `bash`
	was fixed.

## 2.1.2

This release is not a critical release.

1.	A few codes were added to history.
2.	Multiplication was optimized a bit more.
3.	Addition and subtraction were both optimized a bit more.

## 2.1.1

This release contains a fix for the test suite made for Linux from Scratch: now
the test suite prints `pass` when a test is passed.

Other than that, there is no change in this release, so distros and other users
do not need to upgrade.

## 2.1.0

This release is a production release.

The following bugs were fixed:

1.	A `dc` bug that caused stack mishandling was fixed.
2.	A warning on OpenBSD was fixed.
3.	Bugs in `ctrl+arrow` operations in history were fixed.
4.	The ability to paste multiple lines in history was added.
5.	A `bc` bug, mishandling of array arguments to functions, was fixed.
6.	A crash caused by freeing the wrong pointer was fixed.
7.	A `dc` bug where strings, in a rare case, were mishandled in parsing was
	fixed.

In addition, the following changes were made:

1.	Division was slightly optimized.
2.	An option was added to the build to disable printing of prompts.
3.	The special case of empty arguments is now handled. This is to prevent
	errors in scripts that end up passing empty arguments.
4.	A harmless bug was fixed. This bug was that, with the pop instructions
	(mostly) removed (see below), `bc` would leave extra values on its stack for
	`void` functions and in a few other cases. These extra items would not
	affect anything put on the stack and would not cause any sort of crash or
	even buggy behavior, but they would cause `bc` to take more memory than it
	needed.

On top of the above changes, the following optimizations were added:

1.	The need for pop instructions in `bc` was removed.
2.	Extra tests on every iteration of the interpreter loop were removed.
3.	Updating function and code pointers on every iteration of the interpreter
	loop was changed to only updating them when necessary.
4.	Extra assignments to pointers were removed.

Altogether, these changes sped up the interpreter by around 2x.

***NOTE***: This is the last release with new features because this `bc` is now
considered complete. From now on, only bug fixes and new translations will be
added to this `bc`.

## 2.0.3

This is a production, bug-fix release.

Two bugs were fixed in this release:

1.	A rare and subtle signal handling bug was fixed.
2.	A misbehavior on `0` to a negative power was fixed.

The last bug bears some mentioning.

When I originally wrote power, I did not thoroughly check its error cases;
instead, I had it check if the first number was `0` and then if so, just return
`0`. However, `0` to a negative power means that `1` will be divided by `0`,
which is an error.

I caught this, but only after I stopped being cocky. You see, sometime later, I
had noticed that GNU `bc` returned an error, correctly, but I thought it was
wrong simply because that's not what my `bc` did. I saw it again later and had a
double take. I checked for real, finally, and found out that my `bc` was wrong
all along.

That was bad on me. But the bug was easy to fix, so it is fixed now.

There are two other things in this release:

1.	Subtraction was optimized by [Stefan Eßer][14].
2.	Division was also optimized, also by Stefan Eßer.

## 2.0.2

This release contains a fix for a possible overflow in the signal handling. I
would be surprised if any users ran into it because it would only happen after 2
billion (`2^31-1`) `SIGINT`'s, but I saw it and had to fix it.

## 2.0.1

This release contains very few things that will apply to any users.

1.	A slight bug in `dc`'s interactive mode was fixed.
2.	A bug in the test suite that was only triggered on NetBSD was fixed.
3.	**The `-P`/`--no-prompt` option** was added for users that do not want a
	prompt.
4.	A `make check` target was added as an alias for `make test`.
5.	`dc` got its own read prompt: `?> `.

## 2.0.0

This release is a production release.

This release is also a little different from previous releases. From here on
out, I do not plan on adding any more features to this `bc`; I believe that it
is complete. However, there may be bug fix releases in the future, if I or any
others manage to find bugs.

This release has only a few new features:

1.	`atan2(y, x)` was added to the extended math library as both `a2(y, x)` and
	`atan2(y, x)`.
2.	Locales were fixed.
3.	A **POSIX shell-compatible script was added as an alternative to compiling
	`gen/strgen.c`** on a host machine. More details about making the choice
	between the two can be found by running `./configure.sh --help` or reading
	the [build manual][13].
4.	Multiplication was optimized by using **diagonal multiplication**, rather
	than straight brute force.
5.	The `locale_install.sh` script was fixed.
6.	`dc` was given the ability to **use the environment variable
	`DC_ENV_ARGS`**.
7.	`dc` was also given the ability to **use the `-i` or `--interactive`**
	options.
8.	Printing the prompt was fixed so that it did not print when it shouldn't.
9.	Signal handling was fixed.
10.	**Handling of `SIGTERM` and `SIGQUIT`** was fixed.
11.	The **built-in functions `maxibase()`, `maxobase()`, and `maxscale()`** (the
	commands `T`, `U`, `V` in `dc`, respectively) were added to allow scripts to
	query for the max allowable values of those globals.
12.	Some incompatibilities with POSIX were fixed.

In addition, this release is `2.0.0` for a big reason: the internal format for
numbers changed. They used to be a `char` array. Now, they are an array of
larger integers, packing more decimal digits into each integer. This has
delivered ***HUGE*** performance improvements, especially for multiplication,
division, and power.

This `bc` should now be the fastest `bc` available, but I may be wrong.

## 1.2.8

This release contains a fix for a harmless bug (it is harmless in that it still
works, but it just copies extra data) in the [`locale_install.sh`][12] script.

## 1.2.7

This version contains fixes for the build on Arch Linux.

## 1.2.6

This release removes the use of `local` in shell scripts because it's not POSIX
shell-compatible, and also updates a man page that should have been updated a
long time ago but was missed.

## 1.2.5

This release contains some missing locale `*.msg` files.

## 1.2.4

This release contains a few bug fixes and new French translations.

## 1.2.3

This release contains a fix for a bug: use of uninitialized data. Such data was
only used when outputting an error message, but I am striving for perfection. As
Michelangelo said, "Trifles make perfection, and perfection is no trifle."

## 1.2.2

This release contains fixes for OpenBSD.

## 1.2.1

This release contains bug fixes for some rare bugs.

## 1.2.0

This is a production release.

There have been several changes since `1.1.0`:

1.	The build system had some changes.
2.	Locale support has been added. (Patches welcome for translations.)
3.	**The ability to turn `ibase`, `obase`, and `scale` into stacks** was added
	with the `-g` command-line option. (See the [`bc` manual][9] for more
	details.)
4.	Support for compiling on Mac OSX out of the box was added.
5.	The extended math library got `t(x)`, `ceil(x)`, and some aliases.
6.	The extended math library also got `r2d(x)` (for converting from radians to
	degrees) and `d2r(x)` (for converting from degrees to radians). This is to
	allow using degrees with the standard library.
7.	Both calculators now accept numbers in **scientific notation**. See the
	[`bc` manual][9] and the [`dc` manual][10] for details.
8.	Both calculators can **output in either scientific or engineering
	notation**. See the [`bc` manual][9] and the [`dc` manual][10] for details.
9.	Some inefficiencies were removed.
10.	Some bugs were fixed.
11.	Some bugs in the extended library were fixed.
12.	Some defects from [Coverity Scan][11] were fixed.

## 1.1.4

This release contains a fix to the build system that allows it to build on older
versions of `glibc`.

## 1.1.3

This release contains a fix for a bug in the test suite where `bc` tests and
`dc` tests could not be run in parallel.

## 1.1.2

This release has a fix for a history bug; the down arrow did not work.

## 1.1.1

This release fixes a bug in the `1.1.0` build system. The source is exactly the
same.

The bug that was fixed was a failure to install if no `EXECSUFFIX` was used.

## 1.1.0

This is a production release. However, many new features were added since `1.0`.

1.	**The build system has been changed** to use a custom, POSIX
	shell-compatible configure script ([`configure.sh`][6]) to generate a POSIX
	make-compatible `Makefile`, which means that `bc` and `dc` now build out of
	the box on any POSIX-compatible system.
2.	Out-of-memory and output errors now cause the `bc` to report the error,
	clean up, and die, rather than just reporting and trying to continue.
3.	**Strings and constants are now garbage collected** when possible.
4.	Signal handling and checking has been made more simple and more thorough.
5.	`BcGlobals` was refactored into `BcVm` and `BcVm` was made global. Some
	procedure names were changed to reflect its difference to everything else.
6.	Addition got a speed improvement.
7.	Some common code for addition and multiplication was refactored into its own
	procedure.
8.	A bug was removed where `dc` could have been selected, but the internal
	`#define` that returned `true` for a query about `dc` would not have
	returned `true`.
9.	Useless calls to `bc_num_zero()` were removed.
10.	**History support was added.** The history support is based off of a
	[UTF-8 aware fork][7] of [`linenoise`][8], which has been customized with
	`bc`'s own data structures and signal handling.
11.	Generating C source from the math library now removes tabs from the library,
	shrinking the size of the executable.
12.	The math library was shrunk.
13.	Error handling and reporting was improved.
14.	Reallocations were reduced by giving access to the request size for each
	operation.
15.	**`abs()` (`b` command for `dc`) was added as a builtin.**
16.	Both calculators were tested on FreeBSD.
17.	Many obscure parse bugs were fixed.
18.	Markdown and man page manuals were added, and the man pages are installed by
	`make install`.
19.	Executable size was reduced, though the added features probably made the
	executable end up bigger.
20.	**GNU-style array references were added as a supported feature.**
21.	Allocations were reduced.
22.	**New operators were added**: `$` (`$` for `dc`), `@` (`@` for `dc`), `@=`,
	`<<` (`H` for `dc`), `<<=`, `>>` (`h` for `dc`), and `>>=`. See the
	[`bc` manual][9] and the [`dc` manual][10] for more details.
23.	**An extended math library was added.** This library contains code that
	makes it so I can replace my desktop calculator with this `bc`. See the
	[`bc` manual][3] for more details.
24.	Support for all capital letters as numbers was added.
25.	**Support for GNU-style void functions was added.**
26.	A bug fix for improper handling of function parameters was added.
27.	Precedence for the or (`||`) operator was changed to match GNU `bc`.
28.	`dc` was given an explicit negation command.
29.	`dc` was changed to be able to handle strings in arrays.

## 1.1 Release Candidate 3

This release is the eighth release candidate for 1.1, though it is the third
release candidate meant as a general release candidate. The new code has not
been tested as thoroughly as it should for release.

## 1.1 Release Candidate 2

This release is the seventh release candidate for 1.1, though it is the second
release candidate meant as a general release candidate. The new code has not
been tested as thoroughly as it should for release.

## 1.1 FreeBSD Beta 5

This release is the sixth release candidate for 1.1, though it is the fifth
release candidate meant specifically to test if `bc` works on FreeBSD. The new
code has not been tested as thoroughly as it should for release.

## 1.1 FreeBSD Beta 4

This release is the fifth release candidate for 1.1, though it is the fourth
release candidate meant specifically to test if `bc` works on FreeBSD. The new
code has not been tested as thoroughly as it should for release.

## 1.1 FreeBSD Beta 3

This release is the fourth release candidate for 1.1, though it is the third
release candidate meant specifically to test if `bc` works on FreeBSD. The new
code has not been tested as thoroughly as it should for release.

## 1.1 FreeBSD Beta 2

This release is the third release candidate for 1.1, though it is the second
release candidate meant specifically to test if `bc` works on FreeBSD. The new
code has not been tested as thoroughly as it should for release.

## 1.1 FreeBSD Beta 1

This release is the second release candidate for 1.1, though it is meant
specifically to test if `bc` works on FreeBSD. The new code has not been tested as
thoroughly as it should for release.

## 1.1 Release Candidate 1

This is the first release candidate for 1.1. The new code has not been tested as
thoroughly as it should for release.

## 1.0

This is the first non-beta release. `bc` is ready for production use.

As such, a lot has changed since 0.5.

1.	`dc` has been added. It has been tested even more thoroughly than `bc` was
	for `0.5`. It does not have the `!` command, and for security reasons, it
	never will, so it is complete.
2.	`bc` has been more thoroughly tested. An entire section of the test suite
	(for both programs) has been added to test for errors.
3.	A prompt (`>>> `) has been added for interactive mode, making it easier to
	see inputs and outputs.
4.	Interrupt handling has been improved, including elimination of race
	conditions (as much as possible).
5.	MinGW and [Windows Subsystem for Linux][1] support has been added (see
	[xstatic][2] for binaries).
6.	Memory leaks and errors have been eliminated (as far as ASan and Valgrind
	can tell).
7.	Crashes have been eliminated (as far as [afl][3] can tell).
8.	Karatsuba multiplication was added (and thoroughly) tested, speeding up
	multiplication and power by orders of magnitude.
9.	Performance was further enhanced by using a "divmod" function to reduce
	redundant divisions and by removing superfluous `memset()` calls.
10.	To switch between Karatsuba and `O(n^2)` multiplication, the config variable
	`BC_NUM_KARATSUBA_LEN` was added. It is set to a sane default, but the
	optimal number can be found with [`karatsuba.py`][4] (requires Python 3)
	and then configured through `make`.
11.	The random math test generator script was changed to Python 3 and improved.
	`bc` and `dc` have together been run through 30+ million random tests.
12.	All known math bugs have been fixed, including out of control memory
	allocations in `sine` and `cosine` (that was actually a parse bug), certain
	cases of infinite loop on square root, and slight inaccuracies (as much as
	possible; see the [README][5]) in transcendental functions.
13.	Parsing has been fixed as much as possible.
14.	Test coverage was improved to 94.8%. The only paths not covered are ones
	that happen when `malloc()` or `realloc()` fails.
15.	An extension to get the length of an array was added.
16.	The boolean not (`!`) had its precedence change to match negation.
17.	Data input was hardened.
18.	`bc` was made fully compliant with POSIX when the `-s` flag is used or
	`POSIXLY_CORRECT` is defined.
19.	Error handling was improved.
20.	`bc` now checks that files it is given are not directories.

## 1.0 Release Candidate 7

This is the seventh release candidate for 1.0. It fixes a few bugs in 1.0
Release Candidate 6.

## 1.0 Release Candidate 6

This is the sixth release candidate for 1.0. It fixes a few bugs in 1.0 Release
Candidate 5.

## 1.0 Release Candidate 5

This is the fifth release candidate for 1.0. It fixes a few bugs in 1.0 Release
Candidate 4.

## 1.0 Release Candidate 4

This is the fourth release candidate for 1.0. It fixes a few bugs in 1.0 Release
Candidate 3.

## 1.0 Release Candidate 3

This is the third release candidate for 1.0. It fixes a few bugs in 1.0 Release
Candidate 2.

## 1.0 Release Candidate 2

This is the second release candidate for 1.0. It fixes a few bugs in 1.0 Release
Candidate 1.

## 1.0 Release Candidate 1

This is the first Release Candidate for 1.0. `bc` is complete, with `dc`, but it
is not tested.

## 0.5

This beta release completes more features, but it is still not complete nor
tested as thoroughly as necessary.

## 0.4.1

This beta release fixes a few bugs in 0.4.

## 0.4

This is a beta release. It does not have the complete set of features, and it is
not thoroughly tested.

[1]: https://docs.microsoft.com/en-us/windows/wsl/install-win10
[2]: https://pkg.musl.cc/bc/
[3]: http://lcamtuf.coredump.cx/afl/
[4]: ./karatsuba.py
[5]: ./README.md
[6]: ./configure.sh
[7]: https://github.com/rain-1/linenoise-mob
[8]: https://github.com/antirez/linenoise
[9]: ./manuals/bc.1.ronn
[10]: ./manuals/dc.1.ronn
[11]: https://scan.coverity.com/projects/gavinhoward-bc
[12]: ./locale_install.sh
[13]: ./manuals/build.md
[14]: https://github.com/stesser
