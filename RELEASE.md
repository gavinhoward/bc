# Release Checklist

This is the checklist for cutting a release.

1.	Comment out everything in config.mak
2.	Update Busybox sizes and turn both applets on.
3.	Run and pass the release.sh script.
4.	Run Coverity Scan and eliminate warnings, if possible (both only).
	* debug
	* release
	* minrelease
5.	Run the randmath.py script an excessive amount and add failing tests to
	test suite.
	* debug
	* release
	* minrelease
6.	Have other testers try to break it.
7.	Fuzz with AFL
	* release
8.	Fix AFL crashes as much as possible.
9.	Repeat steps 1-6 again and repeat until nothing is found.
10.	Run "make clean_tests".
11.	Run toybox release and submit.
12.	Run busybox release and submit.
13.	Run the release script.
14.	Upload the custom tarball to GitHub.
15.	Add sha's to release notes.
16.	Edit release notes for the changelog.
