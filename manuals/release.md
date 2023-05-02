# Release Checklist

This is the checklist for cutting a release.

For a lot of these steps, they are only needed if the code that would be
affected was changed. For example, I don't need to run the `scripts/randmath.py`
test if I did not change any of the math code.

1.	Run `./scripts/format.sh`.
2.	Update the README.
3.	Update the manuals.
4.	Test with POSIX test suite.
5.	Run the `scripts/randmath.py` script an excessive amount and add failing
	tests to test suite.
	* debug
	* release
	* minrelease
6.	Fuzz with AFL.
	* reldebug
7.	Fix AFL crashes.
8.	Find ASan crashes on AFL test cases.
9.	Fix ASan crashes.
10.	Build on Windows, no errors or warnings.
	* Debug/`x64`.
	* Debug/`x86`.
	* Release{MD,MT}/`x64`.
	* Release{MD,MT}/`x86`.
11.	Run and pass the `scripts/release.sh` script on my own machine.
12.	Run and pass the `scripts/release.sh` script, without generated tests and
	sanitizers, on FreeBSD.
13.	Run and pass the `scripts/release.sh` script, without generated tests,
	sanitizers, and 64-bit, on an ARM server.
14.	Run and pass the release script, with no generated tests, no clang, no
	sanitizers, and no valgrind, on NetBSD.
15.	Run and pass the release script, with no generated tests, no sanitizers, and
	no valgrind, on OpenBSD.
16.	Run `scan-build make`.
17.	Run `./scripts/lint.sh`.
18.	Repeat steps 3-16 again and repeat until nothing is found.
19.	Update the benchmarks.
20.	Update the version and `NEWS.md` and commit.
21. Boot into Windows.
22. Build all release versions of everything.
	* Release/`x64` for `bc`.
	* Release/`x64` for `dc`.
	* Release{MD,MT}/`x64` for `bcl`.
	* Release/`x86` for `bc`.
	* Release/`x86` for `dc`.
	* Release{MD,MT}/`x86` for `bcl`.
23.	Put the builds where Linux can access them.
24. Boot back into Linux.
25.	Run `make clean_tests`.
26.	Run the `scripts/package.sh` script.
27.	Upload the custom tarball and Windows builds to the Gavin Howard Gitea.
28.	Add output from `scripts/package.sh` to Gavin Howard Gitea release notes.
29.	Edit Gavin Howard Gitea release notes for the changelog.
30.	Upload the custom tarball to GitHub.
31.	Add output from `scripts/package.sh` to GitHub release notes.
32.	Edit GitHub release notes for the changelog.
33.	Notify the following:
	* FreeBSD
	* Adelie Linux
	* Ataraxia Linux
	* Sabotage
	* xstatic
	* OpenBSD
	* NetBSD
34.	Submit new packages for the following:
	* Gentoo Linux
	* Termux
	* Linux from Scratch
	* Alpine Linux
	* Void Linux
	* Arch Linux
