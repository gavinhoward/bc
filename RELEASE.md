# Release Checklist

This is the checklist for cutting a release.

1.	Update the README.
2.	Update the manuals.
3.	Run and pass the release.sh script on my own machine.
4.	Run and pass the release.sh script (without all tests) on FreeBSD.
5.	Run Coverity Scan and eliminate warnings, if possible (both only).
	* debug
	* release
	* minrelease
6.	Run the randmath.py script an excessive amount and add failing tests to
	test suite.
	* debug
	* release
	* minrelease
7.	Have other testers try to break it.
8.	Fuzz with AFL
	* release
9.	Fix AFL crashes as much as possible.
10.	Repeat steps 3-9 again and repeat until nothing is found.
11.	Change the version (remove "-dev") and commit.
12.	Run "make clean_tests".
13.	Run the release script.
14.	Upload the custom tarball to GitHub.
15.	Add sha's to release notes.
16.	Edit release notes for the changelog.
17.	Run toybox release and submit.
18.	Increment to the next version (with "-dev").
