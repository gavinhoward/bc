# Release Checklist

This is the checklist for cutting a release.

1.	Update the README.
2.	Update the manuals.
3.	Test history manually.
4.	Run the randmath.py script an excessive amount and add failing tests to
	test suite.
	* debug
	* release
	* minrelease
5.	Fuzz with AFL.
	* reldebug
6.	Fix AFL crashes.
7.	Find ASan crashes on AFL test cases.
8.	Fix ASan crashes.
9.	Build with xstatic.
10.	Run and pass the `release.sh` script, without tests, on my own machine.
11.	Run and pass the `release.sh` script on my own machine.
12.	Run and pass the `release.sh` script, without generated tests and
	sanitizers, on FreeBSD.
13.	Run Coverity Scan and eliminate warnings, if possible (both only).
	* debug
14.	Run `scan-build make`.
15.	Repeat steps 3-14 again and repeat until nothing is found.
16.	Change the version (remove "-dev") and commit.
17.	Run "make clean_tests".
18.	Run the release script.
19.	Upload the custom tarball to GitHub.
20.	Add sha's to release notes.
21.	Edit release notes for the changelog.
22.	Increment to the next version (with "-dev").
23.	Notify the following:
	* FreeBSD
	* Adelie Linux
	* Sabotage
	* xstatic
24.	Submit new packages for the following:
	* OpenBSD
	* NetBSD
	* Alpine Linux
	* Void Linux
	* Gentoo Linux
	* Linux from Scratch
	* Arch Linux
