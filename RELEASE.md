# Release Checklist

This is the checklist for cutting a release.

1.	Update the README.
2.	Update the manuals.
3.	Test history manually.
4.	Run and pass the `release.sh` script, without tests, on my own machine.
5.	Run and pass the `release.sh` script on my own machine.
6.	Run and pass the `release.sh` script, without generated tests and
	sanitizers, on FreeBSD.
7.	Run Coverity Scan and eliminate warnings, if possible (both only).
	* debug
8.	Build with xstatic.
9.	Run the randmath.py script an excessive amount and add failing tests to
	test suite.
	* debug
	* release
	* minrelease
10.	Fuzz with AFL.
	* reldebug
11.	Fix AFL crashes.
12.	Find ASan crashes on AFL test cases.
13.	Fix ASan crashes.
14.	Repeat steps 3-13 again and repeat until nothing is found.
15.	Change the version (remove "-dev") and commit.
16.	Run "make clean_tests".
17.	Run the release script.
18.	Upload the custom tarball to GitHub.
19.	Add sha's to release notes.
20.	Edit release notes for the changelog.
21.	Increment to the next version (with "-dev").
22.	Notify the following:
	* FreeBSD
	* Adelie Linux
	* Sabotage
	* xstatic
23.	Submit new packages for the following:
	* OpenBSD
	* Alpine Linux
	* Void Linux
	* Gentoo Linux
	* Linux from Scratch
	* Arch Linux
