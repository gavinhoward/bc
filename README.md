# bc

This is an implementation of POSIX `bc` that implements
[GNU `bc`](https://www.gnu.org/software/bc/) extensions.

Because this `bc` makes use of [`arbprec`](https://github.com/cmgraff/arbsh) by
[cmgraff](https://github.com/cmgraff) and [hexingb](https://github.com/hexingb)
et al, this `bc` is a collaborative effort. All `arbprec` contributors are
considered `bc` contributors.

## Status

This `bc` is not even in alpha stage yet, so it is not ready for use. However,
at this time, it can do basic math operations (`+`, `-`, `*`, `/`, `%`) on
constants.

## Cloning

After cloning, make sure to run `git submodule update --init --recursive` to
clone the `arbprec` submodule.
