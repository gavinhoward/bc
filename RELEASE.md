# Release Checklist

This is the checklist for cutting a release.

1.	Update the README.
2.	Update the manuals.
3.	Run and pass the `release.sh` script, without tests, on my own machine.
4.	Run and pass the `release.sh` script on my own machine.
5.	Run and pass the `release.sh` script, without generated tests, on FreeBSD.
6.	Run Coverity Scan and eliminate warnings, if possible (both only).
	* debug
	* release
	* minrelease
7.	Run the randmath.py script an excessive amount and add failing tests to
	test suite.
	* debug
	* release
	* minrelease
8.	Have other testers try to break it.
9.	Fuzz with AFL
	* reldebug
10.	Fix AFL crashes as much as possible.
11.	Repeat steps 3-10 again and repeat until nothing is found.
12.	Change the version (remove "-dev") and commit.
13.	Run "make clean_tests".
14.	Run the release script.
15.	Upload the custom tarball to GitHub.
16.	Add sha's to release notes.
17.	Edit release notes for the changelog.
18.	Increment to the next version (with "-dev").
