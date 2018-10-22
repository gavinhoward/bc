# `bc`

This is an implementation of POSIX `bc` that implements
[GNU `bc`](https://www.gnu.org/software/bc/) extensions, as well as the period
(`.`) extension for the BSD flavor of `bc`.

This `bc` also includes an implementation of `dc` in the same binary, accessible
via a symbolic link, which implements all FreeBSD and GNU extensions. If a
single `dc` binary is desired, `bc` can be copied and renamed to `dc`. The `!`
command is omitted; the author believes this is poses security concerns and
further that such functionality is unnecessary.

This `bc` is Free and Open Source Software (FOSS). It is offered under the BSD
0-clause License. Full license text may be found in the `LICENSE.md` file.

## Build

To build, use the following commands:

```
make [bc|dc]
make install
```

This `bc` supports `CC`, `CFLAGS`, `CPPFLAGS`, `LDFLAGS`, `LDLIBS`, `PREFIX`,
and `DESTDIR` `make` variables. Note that to cross-compile this `bc`, an
appropriate compiler must be present in order to bootstrap core file(s), if the
architectures are not compatible (i.e., unlike i686 on x86_64). The approach is:

```
HOSTCC="/path/to/native/compiler" make [bc|dc]
make install
```

It is expected that `CC` produces code for the target system.

Users can also create a file named `config.mak` in the top-level directory to
control `make`. This is not normally necessary.

Executing `make help` lists all `make` targets and options.

## Status

This `bc` is robust.

It is well-tested, fuzzed, and fully standards-compliant (though not yet
certified) with POSIX `bc`. It can be used as a drop-in replacement for any
existing `bc`. To build `bc` from source, an environment which accepts GNU
Makefiles is required.

The community is free to contribute patches for POSIX Makefile builds. This `bc`
is also compatible with MinGW toolchains.

It is also possible to download pre-compiled binaries for a wide list of
platforms, including Linux- and Windows-based systems:

    https://xstatic.musl.cc/bc/

This link always points to the latest release of `bc`.

### Performance

This `bc` has similar performance to GNU `bc`. It is slightly slower on certain
operations and slightly faster on others. Full benchmark data are not yet
available.

#### Algorithms

This `bc` uses the math algorithms below:

##### Addition

This `bc` uses brute force addition, which is linear (`O(n)`) in the number of
digits.

##### Subtraction

This `bc` uses brute force subtraction, which is linear (`O(n)`) in the number
of digits.

##### Multiplication

This `bc` uses two algorithms:
[Karatsuba](https://en.wikipedia.org/wiki/Karatsuba_algorithm) and brute force.

Karatsuba is used for "large" numbers. ("Large" numbers are defined as any
number with `BC_NUM_KARATSUBA_LEN` digits or larger. `BC_NUM_KARATSUBA_LEN` has
a sane default, but may be configured by the user). Karatsuba as implemented in
this `bc` is superlinear but subpolynomial (bound by `O(n^log_2(3))`).

Brute force multiplication is polynomial (`O(n^2)`), but since Karatsuba
requires both more intermediate values (which translate to memory allocations)
and a few more additions, there is a "break even" point in the number of digits
where brute force multiplication is faster than Karatsuba. There is a script
(`$ROOT/karatsuba.py`) that will find the break even point on a particular
platform.

***WARNING: The Karatsuba script requires Python 3 and Numpy.***

##### Division

This `bc` uses Algorithm D
([long division](https://en.wikipedia.org/wiki/Long_division)). Long division is
polynomial (`O(n^2)`), but unlike Karatsuba, any division "divide and conquer"
algorithm reaches its "break even" point with significantly larger numbers.
"Fast" algorithms become less attractive with division as this operation
typically reduces the problem size.

While the implementation of long division may appear to use the subtractive
chunking method, it only uses subtraction to find a quotient digit. It avoids
unnecessary work by aligning digits prior to performing subtraction.

Subtraction was used instead of multiplication for two reasons:

1.	Division and subtraction can share code (one of the goals of this `bc` is
	small code).
2.	It minimizes code complexity.

Multiplication would suffer from a worse worst-case complexity.

##### Power

This `bc` implements
[Exponentiation by Squaring](https://en.wikipedia.org/wiki/Exponentiation_by_squaring),
and (via Karatsuba) has a complexity of `O((n*log(n))^log_2(3))` which is
favorable to the `O((n*log(n))^2)` without Karatsuba.

##### Square Root

This `bc` implements the fast algorithm
[Newton's Method](https://en.wikipedia.org/wiki/Newton%27s_method#Square_root_of_a_number)
(also known as the Newton-Raphson Method, or the
[Babylonian Method](https://en.wikipedia.org/wiki/Methods_of_computing_square_roots#Babylonian_method))
to perform the square root operation. Its complexity is `O(log(n)*n^2)` as it
requires one division per iteration.

##### Sine and Cosine

This `bc` uses the series

```
x - x^3/3! + x^5/5! - x^7/7! + ...
```

to calculate `sin(x)` and `cos(x)`. It also uses the relation

```
cos(x) = sin(x + pi/2)
```

to calculate `cos(x)`. It has a complexity of `O(n^3)`.

##### Exponentiation (Power of `e`)

This `bc` uses the series

```
1 + x + x^2/2! + x^3/3! + ...
```

to calculate `e^x`. Since this only works when `x` is small, it uses

```
e^x = (e^(x/2))^2
```

to reduce `x`. It has a complexity of `O(n^3)`.

##### Natural Log

This `bc` uses the series

```
a + a^3/3 + a^5/5 + ...
```

(where `a` is equal to `(x - 1)/(x + 1)`) to calculate `ln(x)` when `x` is small
and uses the relation

```
ln(x^2) = 2 * ln(x)
```

to sufficiently reduce `x`. It has a complexity of `O(n^3)`.

##### Arctangent

This `bc` uses the series

```
x - x^3/3 + x^5/5 - x^7/7 + ...
```

to calculate `atan(x)` for small `x` and the relation

```
atan(x) = atan(c) + atan((x - c)/(1 + x*c))
```

to reduce `x` to small enough. It has a complexity of `O(n^3)`.

##### Bessel

This `bc` uses the series

```
x^n/(2^n*n!) * (1 - x^2*2*1!*(n + 1)) + x^4/(2^4*2!*(n + 1)*(n + 2)) - ...
```

to calculate the bessel function (integer order only).

It also uses the relation

```
j(-n,x) = (-1)^n*j(n,x)
```

to calculate the bessel when `x < 0`, It has a complexity of `O(n^3)`.

##### Modular Exponentiation (`dc` Only)

This `dc` uses the
[Memory-efficient method](https://en.wikipedia.org/wiki/Modular_exponentiation#Memory-efficient_method)
to compute modular exponentiation. The complexity is `O(e*n^2)`, which may
initially seem inefficient, but `n` is kept small by maintaining small numbers.
In practice, it is extremely fast.

## Language

This `bc` is written in pure ISO C99.

## Commit Messages

This `bc` uses the commit message guidelines laid out in
[this blog post](http://tbaggery.com/2008/04/19/a-note-about-git-commit-messages.html).

## Semantic Versioning

This `bc` uses [semantic versioning](http://semver.org/).

## Contents

Files:

	install.sh       Install script.
	karatsuba.py     Script for package maintainers to find the optimal Karatsuba number.
	LICENSE.md       A Markdown form of the BSD 0-clause License.
	link.sh          A script to link dc to bc.
	Makefile         The Makefile.
	NOTICE.md        List of contributors and copyright owners.
	RELEASE.md       A checklist for making a release.
	safe-install.sh  Safe install script from musl libc.

Folders:

	dist     Files to cut toybox/busybox releases (maintainer use only).
	gen      The `bc` math library, help texts, and code to generate C source.
	include  All header files.
	src      All source code.
	tests    All tests.

