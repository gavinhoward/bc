# Release Checklist

This is the checklist for cutting a release.

1.	Run and pass the release.sh script.
2.	Run Coverity Scan and eliminate warnings, if possible (both only).
	* debug
	* release
	* minrelease
3.	Run the randmath.py script an excessive amount and add failing tests to
	test suite.
	* debug
	* release
	* minrelease
4.	Have other testers try to break it.
5.	Fuzz with AFL
	* release
6.	Fix AFL crashes as much as possible.
7.	Repeat steps 1-8 again and repeat until nothing is found.
8.	Run "make clean_tests".
9.	Run the release script.
10.	Upload the custom tarball to GitHub.
11.	Add sha's to release notes.
12.	Edit release notes for the changelog.
13.	Run toybox release and submit.
