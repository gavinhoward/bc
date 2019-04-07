# Build

This `bc` attempts to be as portable as possible. It can be built on any
POSIX-compliant system.

To accomplish that, a POSIX-compatible, custom `configure.sh` script is used to
select build options, compiler, and compiler flags and generate a `Makefile`.

The general form of configuring, building, and installing this `bc` is as
follows:

```
[ENVIRONMENT_VARIABLE=<value>...] ./configure.sh [build_options...]
make
make install
```

To get all of the options, including any useful environment variables, use
either one of the following commands:

```
./configure.sh -h
./configure.sh --help
```

To learn the available `make` targets run the following command after running
the `configure.sh` script:

```
make help
```

See [Build Environment Variables][4] for a more detailed description of all
accepted environment variables and [Build Options][5] for more detail about all
accepted build options.

<a name="cross-compiling"/>

## Cross Compiling

To cross-compile this `bc`, an appropriate compiler must be present and assigned
to the environment variable `HOSTCC` or `HOST_CC` (the two are equivalent,
though `HOSTCC` is prioritized). This is in order to bootstrap core file(s), if
the architectures are not compatible (i.e., unlike i686 on x86_64). Thus, the
approach is:

```
HOSTCC="/path/to/native/compiler" ./configure.sh
make
make install
```

`HOST_CC` will work in exactly the same way.

`HOSTCFLAGS` and `HOST_CFLAGS` can be used to set compiler flags for `HOSTCC`.
(The two are equivalent, as `HOSTCC` and `HOST_CC` are.) `HOSTCFLAGS` is
prioritized over `HOST_CFLAGS`. If neither are present, `HOSTCC` (or `HOST_CC`)
uses `CFLAGS` (see [Build Environment Variables][4] for more details).

It is expected that `CC` produces code for the target system and `HOSTCC`
produces code for the host system. See [Build Environment Variables][4] for more
details.

If an emulator is necessary to run the bootstrap binaries, it can be set with
the environment variable `GEN_EMU`.

<a name="build-environment-variables"/>

## Build Environment Variables

This `bc` supports `CC`, `HOSTCC`, `HOST_CC`, `CFLAGS`, `HOSTCFLAGS`,
`HOST_CFLAGS`, `CPPFLAGS`, `LDFLAGS`, `LDLIBS`, `PREFIX`, `DESTDIR`, `BINDIR`,
`DATAROOTDIR`, `DATADIR`, `MANDIR`, `MAN1DIR`, `LOCALEDIR` `EXECSUFFIX`,
`EXECPREFIX` and `GEN_EMU` environment variables in `configure.sh`. Any values
of those variables given to `configure.sh` will be put into the generated
Makefile.

More detail on what those environment variables do can be found in the following
sections.

### `CC`

C compiler for the target system. `CC` must be compatible with POSIX `c99`
behavior and options.

Defaults to `c99`.

### `HOSTCC` or `HOST_CC`

C compiler for the host system, used only in [cross compiling][6].

Defaults to `$CC`.

### `CFLAGS`

Command-line flags that will be passed verbatim to `CC`.

Defaults to empty.

### `HOSTCFLAGS` or `HOST_CFLAGS`

Command-line flags that will be passed verbatim to `HOSTCC` or `HOST_CC`.

Defaults to `$CFLAGS`.

### `CPPFLAGS`

Command-line flags for the C preprocessor. These are also passed verbatim to
both compilers (`CC` and `HOSTCC`); they are supported just for legacy reasons.

Defaults to empty.

### `LDFLAGS`

Command-line flags for the linker. These are also passed verbatim to both
compilers (`CC` and `HOSTCC`); they are supported just for legacy reasons.

Defaults to empty.

### `LDLIBS`

Libraries to link to. These are also passed verbatim to both compilers (`CC` and
`HOSTCC`); they are supported just for legacy reasons and for cross compiling
with different C standard libraries (like [musl][3]).

Defaults to empty.

### `PREFIX`

The prefix to install to.

Can be overridden by passing the `--prefix` option to `configure.sh`.

Defaults to `/usr/local`.

### `DESTDIR`

Path to prepend onto `PREFIX`. This is mostly for distro and package
maintainers.

This can be passed either to `configure.sh` or `make install`. If it is passed
to both, the one given to `configure.sh` takes precedence.

Defaults to empty.

### `BINDIR`

The directory to install binaries in.

Can be overridden by passing the `--bindir` option to `configure.sh`.

Defaults to `$PREFIX/bin`.

### `DATAROOTDIR`

The root directory to install data files in.

Can be overridden by passing the `--datarootdir` option to `configure.sh`.

Defaults to `$PREFIX/share`.

### `DATADIR`

The directory to install data files in.

Can be overridden by passing the `--datadir` option to `configure.sh`.

Defaults to `$DATAROOTDIR`.

### `MANDIR`

The directory to install manpages in.

Can be overridden by passing the `--mandir` option to `configure.sh`.

Defaults to `$DATADIR/man`

### `MAN1DIR`

The directory to install Section 1 manpages in. Because both `bc` and `dc` are
Section 1 commands, this is the only relevant section directory.

Can be overridden by passing the `--man1dir` option to `configure.sh`.

Defaults to `$MANDIR/man1`.

### `LOCALEDIR`

The directory to install locales in.

Can be overridden by passing the `--localedir` option to `configure.sh`.

Defaults to `$DATAROOTDIR/locale`.

### `EXECSUFFIX`

The suffix to append onto the executable names *when installing*. This is for
packagers and distro maintainers who want this `bc` as an option, but do not
want to replace the default `bc`.

Defaults to empty.

### `EXECPREFIX`

The prefix to append onto the executable names *when building and installing*.
This is for packagers and distro maintainers who want this `bc` as an option,
but do not want to replace the default `bc`.

Defaults to empty.

### `GEN_EMU`

The emulator to run bootstrap binaries under. This is only if the binaries
produced by `HOSTCC` (or `HOST_CC`) need to be run under an emulator to work.

Defaults to empty.

<a name="build-options"/>

## Build Options

This `bc` comes with several build options, all of which are enabled by default.

All options can be used with each other, with a few exceptions that will be
noted below.

**NOTE**: All long options with mandatory argumenst accept either one of the
following forms:

```
--option arg
--option=arg
```

### `bc` Only

To build `bc` only (no `dc`), use any one of the following commands for the
configure step:

```
./configure.sh -b
./configure.sh --bc-only
./configure.sh -D
./configure.sh --disable-dc
```

Those commands are all equivalent.

***Warning***: It is an error to use those options if `bc` has also been
disabled (see below).

### `dc` Only

To build `dc` only (no `bc`), use either one of the following commands for the
configure step:

```
./configure.sh -d
./configure.sh --dc-only
./configure.sh -B
./configure.sh --disable-bc
```

Those commands are all equivalent.

***Warning***: It is an error to use those options if `dc` has also been
disabled (see above).

<a name="build-signal-handling"/>

### Signal Handling

To disable signal handling, pass either the `-S` flag or the
`--disable-signal-handling` option to `configure.sh`, as follows:

```
./configure.sh -S
./configure.sh --disable-signal-handling
```

Both commands are equivalent.

<a name="build-history"/>

### History

To disable signal handling, pass either the `-H` flag or the `--disable-history`
option to `configure.sh`, as follows:

```
./configure.sh -H
./configure.sh --disable-history
```

Both commands are equivalent.

History is automatically disabled when building for Windows or on another
platform that does not support the terminal handling that is required.

***WARNING***: Of all of the code in the `bc`, this is the only code that is not
completely portable. If the `bc` does not work on your platform, your first step
should be to retry with history disabled.

### NLS (Locale Support)

To disable locale support (use only English), pass either the `-N` flag or the
`--disable-nls` option to `configure.sh`, as follows:

```
./configure.sh -N
./configure.sh --disable-nls
```

Both commands are equivalent.

NLS (locale support) is automatically disabled when building for Windows or on
another platform that does not support the POSIX locale API or utilities.

### Extra Math

This `bc` has 7 extra operators:

* `$` (truncation to integer)
* `@` (set precision)
* `@=` (set precision and assign)
* `<<` (shift number left, shifts radix right)
* `<<=` (shift number left and assign)
* `>>` (shift number right, shifts radix left)
* `>>=` (shift number right and assign)

There is no assignment version of `$` because it is a unary operator.

The assignment versions of the above operators are not available in `dc`, but
the others are, as the operators `$`, `@`, `H`, and `h`, respectively.

Extra operators can be disabled by passing either the `-E` flag or the
`--disable-extra-math` option to `configure.sh`, as follows:

```
./configure.sh -E
./configure.sh --disable-extra-math
```

Both commands are equivalent.

This `bc` also has a larger library that is only enabled if extra operators are.
More information about the functions can be found in the
[Extended Library](./bc.md#extended-library) section of the
[full manual](./bc.md).

### Manpages

To disable installing manpages, pass either the `-M` flag or the
`--disable-man-pages` option to `configure.sh` as follows:

```
./configure.sh -M
./configure.sh --disable-man-pages
```

Both commands are equivalent.

### Karatsuba Length

The Karatsuba length is the point at which `bc` and `dc` switch from Karatsuba
multiplication to brute force, `O(n^2)` multiplication. It can be set by passing
the `-k` flag or the `--karatsuba-len` option to `configure.sh` as follows:

```
./configure.sh -k64
./configure.sh --karatsuba-len 64
```

Both commands are equivalent.

***WARNING***: The Karatsuba Length must be a **integer** greater than or equal
to `16`. If it is not, `configure.sh` will give an error.

### Install Options

The relevant `autotools`-style install options are supported in `configure.sh`:

* `--prefix`
* `--bindir`
* `--datarootdir`
* `--datadir`
* `--mandir`
* `--man1dir`
* `--localedir`

An example is:

```
./configure.sh --prefix=/usr --localedir /usr/share/nls
make
make install
```

They correspond to the environment variables `$PREFIX`, `$BINDIR`,
`$DATAROOTDIR`, `$DATADIR`, `$MANDIR`, `$MAN1DIR`, and `$LOCALEDIR`,
respectively.

***WARNING***: If the option is given, the value of the corresponding
environment variable is overridden.

## Optimization

The `configure.sh` script will accept an optimization level to pass to the
compiler. Because `bc` is orders of magnitude faster with optimization, I
***highly*** recommend package and distro maintainers pass the highest
optimization level available in `CC` to `configure.sh` with the `-O` flag or
`--opt` option, as follows:

```
./configure.sh -O3
./configure.sh --opt 3
```

Both commands are equivalent.

The build and install can then be run as normal:

```
make
make install
```

As usual, `configure.sh` will also accept additional `CFLAGS` on the command
line, so for SSE4 architectures, the following can add a bit more speed:

```
CFLAGS="-march=native -msse4" ./configure.sh -O3
make
make install
```

Building with link-time optimization (`-flto` in clang) can further increase the
performance.

Manual stripping is not necessary; non-debug builds are automatically stripped
in the link stage.

## Debug Builds

Debug builds (which also disable optimization if no optimization level is given
and if no extra `CFLAGS` are given) can be enabled with either the `-g` flag or
the `--debug` option, as follows:

```
./configure.sh -g
./configure.sh --debug
```

Both commands are equivalent.

The build and install can then be run as normal:

```
make
make install
```

## Binary Size

When built with both calculators, all available features, and `-Os` using clang,
the executable is 101 kb (100,912 bytes) on x86_64. That isn't much for what is
contained in the binary, but if necessary, it can be reduced.

The single largest user of space is the `bc` calculator. If just `dc` is needed,
the size can be reduced to to 72 kb (72,223 bytes).

The next largest user of space is history support. If that is not needed, size
can be reduced (for a build with both calculators) to 84 kb (84,448 bytes).

There are several reasons that history is a bigger user of space than `dc`
itself:

* `dc`'s lexer and parser are *tiny* compared to `bc`'s.
* `dc` does not have much extra code in the interpreter.
* History has a lot of const data for supporting `UTF-8` terminals.

The next biggest user is `dc`, so if just `bc` is needed, the size can be
reduced to 89 kb (88,624 bytes) with history and 76 kb (76,256 bytes) without
history.

The next biggest user is signal handling. Without it, the size (with both
calculators) is reduced to 97 kb (97,800 bytes) with history and 84 kb (84,416
bytes) without history.

The next largest user is extra math support. If this is not needed, the size
(with both calculators) can be reduced to 97 kb (96,808 bytes) with history and
signal handling, 80 kb (80,344 bytes) without history, 93 kb (92,696 bytes)
without signal handling, and 76 kb (76,216 bytes) without both.

## Testing

The default test suite can be run with the following command:

```
make test
```

To test `bc` only, run the following command:

```
make test_bc
```

To test `dc` only, run the following command:

```
make test_dc
```

This `bc`, if built, assumes a working, GNU-compatible `bc`, installed on the
system and in the `PATH`, to generate some tests, unless the `-G` flag or
`--disable-generated-tests` option is given to `configure.sh`, as follows:

```
./configure.sh -G
./configure.sh --disable-generated-tests
```

After running `configure.sh`, build and run tests as follows:

```
make
make test
```

This `dc` also assumes a working, GNU-compatible `dc`, installed on the system
and in the `PATH`, to generate some tests, unless on of the above options is
given to `configure.sh`.

To generate test coverage, pass the `-c` flag or the `--coverage` option to
`configure.sh` as follows:

```
./configure.sh -c
./configure.sh --coverage
```

Both commands are equivalent.

***WARNING***: Both `bc` and `dc` must be built for test coverage. Otherwise,
`configure.sh` will give an error.

[1]: https://pubs.opengroup.org/onlinepubs/9699919799/utilities/bc.html
[2]: https://www.gnu.org/software/bc/
[3]: https://www.musl-libc.org/
[4]: #build-environment-variables
[5]: #build-options
[6]: #cross-compiling
