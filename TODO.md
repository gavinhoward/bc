# TODO

* Remove all instances of `BcStatus s`.
* Make all functions `void`.
* Rename `bc_vm_sigjmp()` to `bc_vm_jmp()`.
* Implement buffered I/O.
* Just jmp out on `BC_STATUS_EOF`.
* Properly lock all functions and provide `BC_SETJMP*`'s.
* Figure out what to do about ordering unlocking and jmps.
