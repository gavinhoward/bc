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
3.	Run scan-build with gcc and eliminate warnings (both, bc only, and dc only).
	* debug
	* release
	* minrelease
4.	Run Coverity Scan and eliminate warnings, if possible (both, bc only, and dc
	only).
	* debug
	* release
	* minrelease
5.	Run and pass the test suite (both, bc only, and dc only).
	* debug (with and without AddressSanitizer and UndefinedBehaviorSanitizer)
	* release
	* minrelease
6.	Run timeconst.sh.
	* debug
	* release
	* minrelease
7.	Run the Karatsuba script with tests to ensure multiply is correct.
	* debug
	* release
	* minrelease
8.	Run the randmath.sh script an excessive amount and add failing tests to
	test suite.
	* debug
	* release
	* minrelease
9.	Run valgrind on the test suite.
	* debug
	* release
	* minrelease
10.	Have other testers try to break it.
11.	Fuzz with AFL
	* release
12.	Fix AFL crashes as much as possible.
13.	Repeat steps 1-11 again and repeat until nothing is found.
14.	Run "make clean_tests".
15.	Run toybox release and submit.
16.	Run busybox release and submit.
17.	Run the release script.
18.	Upload the custom tarball to GitHub.
19.	Add sha's to release notes.
20.	Edit release notes for the changelog.
