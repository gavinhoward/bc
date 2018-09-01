# Release Checklist

This is the checklist for cutting a release.

1.	Eliminate all compiler warnings.
	* debug
	* release
	* minrelease
2.	Run scan-build and eliminate warnings.
	* debug
	* release
	* minrelease
3.	Run and pass the test suite.
	* debug (with AddressSanitizer and UndefinedBehaviorSanitizer)
	* release
	* minrelease
4.	Run the randmath.sh script an excessive amount.
	* debug
	* release
	* minrelease
5.	Run valgrind on the test suite.
	* debug
6.	Have other testers try to break it.
7.	Fuzz with AFL
	* release
8.	Fix AFL crashes as much as possible.
9.	Repeat steps 1-4 again.
10.	Run the release script.
11.	Upload the custom tarball to GitHub.
12.	Add sha's to release notes.
13.	Edit release notes for the changelog.
