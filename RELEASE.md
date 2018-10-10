# Release Checklist

This is the checklist for cutting a release.

1.	Eliminate all compiler warnings (both, bc only, and dc only).
	* debug
	* release
	* minrelease
2.	Run scan-build and eliminate warnings (both, bc only, and dc only).
	* debug
	* release
	* minrelease
3.	Run Coverity Scan and eliminate warnings (both, bc only, and dc only).
	* debug
	* release
	* minrelease
4.	Run and pass the test suite (both, bc only, and dc only).
	* debug (with and without AddressSanitizer and UndefinedBehaviorSanitizer)
	* release
	* minrelease
5.	Run timeconst.sh.
	* debug
	* release
	* minrelease
6.	Run the randmath.sh script an excessive amount and add failing tests to
	test suite.
	* debug
	* release
	* minrelease
7.	Run valgrind on the test suite.
	* debug
	* release
	* minrelease
8.	Have other testers try to break it.
9.	Fuzz with AFL
	* release
10.	Fix AFL crashes as much as possible.
11.	Repeat steps 1-9 again and repeat until nothing is found.
12.	Run the release script.
13.	Upload the custom tarball to GitHub.
14.	Add sha's to release notes.
15.	Edit release notes for the changelog.
