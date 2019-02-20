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
8.	Run the randmath.py script an excessive amount and add failing tests to
	test suite.
	* debug
	* release
	* minrelease
9.	Have other testers try to break it.
10.	Fuzz with AFL.
	* reldebug
11.	Fix AFL crashes.
12.	Repeat steps 3-11 again and repeat until nothing is found.
13.	Change the version (remove "-dev") and commit.
14.	Run "make clean_tests".
15.	Run the release script.
16.	Upload the custom tarball to GitHub.
17.	Add sha's to release notes.
18.	Edit release notes for the changelog.
19.	Increment to the next version (with "-dev").
