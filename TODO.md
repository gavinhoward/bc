# TODO

* Figure out what to do about ordering unlocking and jmps.
* Update README to POSIX 2008; the standard is good enough there.
* Check all uses of BC_LONGJMP_CONT and change them to BC_LONGJMP_CONT_LOCKED or
  BC_UNSETJMP as necessary.
* Check all uses of BC_LONGJMP_CONT_LOCKED and change them to BC_LONGJMP_CONT or
  BC_UNSETJMP as necessary.
* Change BC_LONGJMP_LOCKED to not exist and use explicit BC_SIG_UNLOCK instead?
* Make sure to restore signal mask when stopping a longjmp() series.
* Make sure to undo signal mask before returning from `main()`.
* Run tests to be sure I can remove the extra code in `bc_num_as()`.
* Be sure to handle interrupts in history.
* Be sure to sig lock all relevant stuff in history.
	* `read()`
	* `write()`
	* termios stuff
* Lock all instances of `free()`.
* Fix all TODO comments
* Run tests on Stefan's proposed change.
