# TODO

* Run tests to be sure I can remove the extra code in `bc_num_as()`.
* Be sure to handle interrupts in history.
* Be sure to sig lock all relevant stuff in history.
	* `read()`
	* `write()`
	* termios stuff
* Lock all instances of `free()`.
* Fix all TODO comments
* Run tests on all of Stefan's proposed changes.
* Rerun benchmarks.
* Redo executable size (with static binaries).
* Stop passing BcProgram and BcHistory around?
