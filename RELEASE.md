# Release Checklist

This is the checklist for cutting a release.

1.	Eliminate all compiler warnings, clang and gcc (both, bc only, and dc only).
	* debug
	* release
	* minrelease
2.	Run scan-build and eliminate warnings (both, bc only, and dc only).
	* debug
	* release
	* minrelease
3.	Run Coverity Scan and eliminate warnings, if possible (both only).
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
6.	Run the Karatsuba script with tests to ensure multiply is correct.
	* debug
	* release
	* minrelease
7.	Run the randmath.sh script an excessive amount and add failing tests to
	test suite.
	* debug
	* release
	* minrelease
8.	Run valgrind on the test suite.
	* debug
	* release
	* minrelease
9.	Have other testers try to break it.
10.	Fuzz with AFL
	* release
11.	Fix AFL crashes as much as possible.
12.	Repeat steps 1-11 again and repeat until nothing is found.
13.	Run "make clean_tests".
14.	Run toybox release and submit.
15.	Run busybox release and submit.
16.	Run the release script.
17.	Upload the custom tarball to GitHub.
18.	Add sha's to release notes.
19.	Edit release notes for the changelog.
