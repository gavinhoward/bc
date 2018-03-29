# Release Checklist

This is the checklist for cutting a release.

1.	Eliminate all compiler warnings
	* debug
	* release
	* minrelease
2.	Run scan-build and eliminate warnings
	* debug
	* release
	* minrelease
3.	Run and pass the test suite
	* debug (with AddressSanitizer and UndefinedBehaviorSanitizer)
	* release
	* minrelease
4.	Run valgrind on the test suite
	* debug
5.	Have other testers try to break it.
6.	Fuzz with AFL
	* release
7.	Fix AFL crashes as much as possible
8.	Repeat steps 1-4 again.
9.	Run the release script.
10.	Upload the custom tarball to GitHub.
11.	Add sha's to release notes.
12.	Edit release notes for the changelog.
