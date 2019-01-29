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

To get all of the options, including any useful environment variables, use the
following command:

```
./configure.sh -h
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

It is expected that `CC` produces code for the target system. See
[Build Environment Variables][4] for more details.

If an emulator is necessary to run the bootstrap binaries, it can be set with
the environment variable `GEN_EMU`.

<a name="build-environment-variables"/>

## Build Environment Variables

This `bc` supports `CC`, `HOSTCC`, `HOST_CC`, `CFLAGS`, `HOSTCFLAGS`,
`HOST_CFLAGS`, `CPPFLAGS`, `LDFLAGS`, `LDLIBS`, `PREFIX`, `DESTDIR`, `BINDIR`,
`DATAROOTDIR`, `DATADIR`, `MANDIR`, `MAN1DIR`, `EXECSUFFIX` and `GEN_EMU`
environment variables in `configure.sh`. Any values of those variables given to
`configure.sh` will be put into the generated Makefile.

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

Defaults to `/usr/local`.

### `DESTDIR`

Path to prepend onto `PREFIX`. This is mostly for distro and package
maintainers.

Defaults to empty.

### `BINDIR`

The directory to install binaries in.

Defaults to `$PREFIX/bin`.

### `DATAROOTDIR`

The root directory to install data files in.

Defaults to `$PREFIX/share`.

### `DATADIR`

The directory to install data files in.

Defaults to `$DATAROOTDIR`.

### `MANDIR`

The directory to install manpages in.

Defaults to `$DATADIR/man`

### `MAN1DIR`

The directory to install Section 1 manpages in. Because both `bc` and `dc` are
Section 1 commands, this is the only relevant section directory.

Defaults to `$MANDIR/man1`.

### `EXECSUFFIX`

The suffix to append onto the executable names *when installing*. This is for
packagers and distro maintainers who want this `bc` as an option, but do not
want to replace the default `bc`.

Defaults to empty.

### `GEN_EMU`

The emulator to run bootstrap binaries under. This is only if the binaries
produced by `HOSTCC` (or `HOST_CC`) need to be run under an emulator to work.

Defaults to empty.

<a name="build-options"/>

## Build Options

This `bc` comes with several build options, all of which are enabled by default.

All options can be used with each other, with a few exceptions that will be
noted below. Also, array references are turned off automatically when building
only `dc`.

### `bc` Only

To build `bc` only (no `dc`), use either one of the following commands for the
configure step:

```
./configure.sh -b
./configure.sh -D
```

Those two commands are equivalent.

***Warning***: It is an error to use those options if `bc` has also been
disabled (see below).

### `dc` Only

To build `dc` only (no `bc`), use either one of the following commands for the
configure step:

```
./configure.sh -d
./configure.sh -B
```

Those two commands are equivalent.

***Warning***: It is an error to use those options if `dc` has also been
disabled (see above).

<a name="build-signal-handling"/>

### Signal Handling

To disable signal handling, use the `-S` flag in the configure step:

```
./configure.sh -S
```

<a name="build-history"/>

### History

To disable signal handling, use the `-H` flag in the configure step:

```
./configure.sh -H
```

History is automatically disabled when building for Windows or on another
platform that does not support the terminal handling that is required.

***WARNING***: Of all of the code in the `bc`, this is the only code that is not
completely portable. If the `bc` does not work on your platform, your first step
should be to retry with history disabled.

<a name="build-array-references"/>

### Array References

Array references are an extension to the [standard][1] first implemented by the
[GNU `bc`][2]. They can be disabled by using the `-R` flag in the configure
step:

```
./configure.sh -R
```

<a name="build-extra-math"/>

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

Extra operators can be disabled using the `-E` flag in the configure step:

```
./configure.sh -E
```

This `bc` also has a larger library that is only enabled if extra operators are.
More information about the functions can be found in the
[Extended Library](./bc.md#extended-library) section of the
[full manual](./bc.md).

## Optimization

The `configure.sh` script will accept an optimization level to pass to the
compiler. Because `bc` is orders of magnitude faster with optimization, I
***highly*** recommend package and distro maintainers pass the highest
optimization level available in `CC` to `configure.sh`, as follows:

```
./configure.sh -O3
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
and if no extra `CFLAGS` are given) can be enabled with:

```
./configure.sh -g
make
make install
```

## Testing

The default test suite can be run with the following command:

```
make test
```

All available tests can be run by running the following command:

```
make test_all
```

This `bc`, if built, assumes a working, GNU-compatible `bc` in the `PATH` to
generate some tests, unless the `-G` option is given to `configure.sh`, as
follows:

```
./configure.sh -G
make
make test
```

This `dc` also assumes a working, GNU-compatible `dc` in the `PATH` to generate
some tests, unless the above option is given to `configure.sh`.

[1]: https://pubs.opengroup.org/onlinepubs/9699919799/utilities/bc.html
[2]: https://www.gnu.org/software/bc/
[3]: https://www.musl-libc.org/
[4]: #build-environment-variables
[5]: #build-options
[6]: #cross-compiling
