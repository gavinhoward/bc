# `bc`(1)

## Name

bc — arbitrary-precision arithmetic language and calculator

## Synopsis

`bc` [`-hilqsvVw`] [`--help`] [`--interactive`] [`--mathlib`] [`--quiet`]
[`--standard`] [`--warn`] [`--version`] [`-e` *expr*] [`--expression=`*expr*...]
[`-f` *file*...] [`-file=`*file*...] [*file*...]

## Description

`bc` is an interactive processor for a language first standardized in 1991 by
POSIX. (The current standard is [here][1].) The language provides unlimited
precision decimal arithmetic and is somewhat C-like, but there are differences.
Such differences will be noted in this document.

After parsing and handling options, this `bc` reads any files given on the
command line and executes them before reading from `stdin`.

With all build options enabled (except for extra math), this `bc` is a drop-in
replacement for ***any*** `bc`, including (and especially) the [GNU `bc`][2].

### Options

##### `-e` *expr* `--expression=`*expr*

Evaluates `expr`. If multiple expressions are given, they are evaluated in
order. If files are given as well (see below), the expressions and files are
evaluated in the order given. This means that if a file is given before an
expression, the file is read in and evaluated first.

##### `-f` *file* `--file=`*file*

Reads in `file` and evaluates it. If expressions are also given (see above), the
expressions are evaluated in the order given.

##### `-h` `--help`

Prints a usage message and quits.

##### `-i` `--interactive`

Forces interactive mode.

Per the [standard][1], `bc` has an interactive mode and a non-interactive mode.
The interactive mode is turned on automatically when both `stdin` and `stdout`
are hooked to a terminal, but this flag can turn it on in other cases. In
interactive mode, `bc` attempts to recover from errors and flushes `stdout` as
soon as execution is done for the current input.

##### `-l` `--mathlib`

Sets `scale` (see the Scale section) to `20` and loads the included math library
before running any code, including any expressions or files specified on the
command line.

To learn what is in the library, see the [Library](#library) section.

##### `-q` `--quiet`

Do not print copyright header. `bc` will also suppress the header in
non-interactive mode.

This is mostly for compatibility with the [GNU `bc`][2].

##### `-s` `--standard`

Process exactly the language defined by the [standard][1] and error if any
extensions are used.

##### `-v` `-V` `--version`

Print the version information (copyright header) and exit.

##### `-w` `--warn`

Like `-s` and `--standard`, except that warnings (and not errors) are given for
non-standard extensions.

## Build

See the [build manual](./build.md).

## Language

### Syntax

The syntax for `bc` programs is mostly C-like, with some differences. This `bc`
follows the [POSIX standard][1], which is a much more thorough resource for the
language this `bc` accepts. This section is meant to be a summary and a listing
of all the extensions to the [standard][1].

In the sections below, `E` means expression, `S` means statement, and `I` means
identifier. Identifiers start with a lowercase letter and can be followed by
any number (up to `BC_NAME_MAX - 1`) of lowercase letters (`a-z`), digits
(`0-9`), and underscores (`_`). Identifiers with more than one character
(letter) are an extension.

#### Comments

There are two kinds of comments:

1.	Block comments are enclosed in `/*` and `*/`.
2.	Line comments go from `#` until, and not including, the next newline. This
	is a non-portable extension.

#### Names

1.	Variables: `I`
2.	Array Elements: `I[E]`
3.	`ibase`
4.	`obase`
5.	`scale`
6.	`last` or a single dot (`.`)

#### Operands

1.	Numbers with at most `BC_NUM_MAX` digits and an optional decimal point.
	Digits can include `[0-9A-Z]`, where the uppercase letters equal 9 + their
	position in the alphabet (i.e., `A` equals `10`, or `9+1`).
2.	`(E)`: The value of `E`.
3.	`sqrt(E)`: The square root of `E`.
4.	`length(E)`: The number of significant decimal digits in `E`.
5.	`length(I[])`: The number of elements in the array `I`. This is a
	non-portable extension.
6.	`scale(E)`: Number of digits right of the decimal point in `E`.

#### Operators

The following arithmetic and logical operators can be used. They are listed in
order of decreasing precedence. Operators in the same group have the same
precedence.

| Operators   | Type               | Associativity | Description               |
|-------------|--------------------|---------------|---------------------------|
| `++` `--`   | Prefix and Postfix | None          | increment, decrement      |
| `-` `!`     | Prefix             | None          | negation, boolean not     |
| `$`         | Postfix            | None          | truncation                |
| `@`         | Binary             | Right         | set precision             |
| `^`         | Binary             | Right         | power                     |
| `*` `/` `%` | Binary             | Left          | multiply, divide, modulus |
| `+` `-`     | Binary             | Left          | plus, minus               |
| `<<` `>>`   | Binary             | Left          | shift left, shift right   |
| `=` `<<=` `>>=` `+=` `-=` `*=` `/=` `%=` `^=` `@=` | Binary | Right | assignment |
| `==` `<=` `>=` `!=` `<` `>` | Binary | Left      | relational                |
| `&&`        | Binary             | Left          | boolean and               |
| \|\|        | Binary             | Left          | boolean or                |

There are differences between how these operators work in C (if they exist) and
how they work in `bc`:

1.	`$` is truncation, which is to take the given expression, copy it, and
	return a new value that is equivalent to that expression except with all of
	its fractional part (any digits after a decimal point) removed.
2.	`@` is set precision, which takes two expressions and returns a copy of the
	first with the same amount of decimal places as the value of the second
	expression. The second expression must be an integer (no fractional part).
3.	`^` is power, not exclusive or.

<a name="library"/>

### Library

All of the functions below, including the functions in the
[extended library](#extended-library) if `bc` has been compiled with the
[extra math](./build.md#extra-math) option enabled, are available when the `-l`
or `--mathlib` command-line flags are given.

<a name="standard-library"/>

#### Standard Library

The [standard][1] defines the following functions for the math library:

##### `s(x)`

Returns the sine of `x`, which is assumed to be in radians.

##### `c(x)`

Returns the cosine of `x`, which is assumed to be in radians.

##### `a(x)`

Returns the arctangent of `x`, in radians.

##### `l(x)`

Returns the natural logarithm of `x`.

##### `e(x)`

Returns the mathematical constant `e` raised to the power of `x`.

##### `j(x, n)`

Returns the bessel integer order `n` of `x`.

<a name="extended-library"/>

#### Extended Library

In addition to the [standard library](#standard-library), if `bc` has been built
with the [extra math](./build.md#extra-math) option, the following functions are
available when either the `-l` or `--mathlib` options are given.

However, the extended library is ***not*** loaded when the `-s`/`--standard` or
`-w`/`--warn` options are given since they are not part of the library defined
by the [standard][1].

##### `abs(x)`

Returns the absolute value of `x`.

##### `r(x, p)`

Rounds `x` to `p` decimal places according to the rounding mode
[round half away from `0`][3].

##### `f(x)`

Returns the factorial of the truncated absolute value of `x`.

##### `perm(n, k)`

Returns the permutation of the truncated absolute value of `n` of the truncated
absolute value of `k`, if `k <= n`. If not, it returns `0`.

##### `comb(n, k)`

Returns the combination of the truncated absolute value of `n` of the truncated
absolute value of `k`, if `k <= n`. If not, it returns `0`.

##### `l2(x)`

Returns the logarithm base `2` of `x`.

##### `l10(x)`

Returns the logarithm base `10` of `x`.

##### `log(x, b)`

Returns the logarithm base `b` of `x`.

##### `pi(p)`

Returns `pi` to p decimal places.

##### `ubytes(x)`

Returns the numbers of unsigned integer bytes required to hold the truncated
absolute value of `x`.

##### `sbytes(x)`

Returns the numbers of signed, two's-complement integer bytes required to hold
the truncated value of `x`.

##### `hex(x)`

Outputs the hexadecimal (base `16`) representation of `x`.

This is a [void](#void-functions) function.

##### `binary(x)`

Outputs the binary (base `2`) representation of `x`.

This is a [void](#void-functions) function.

##### `output(x, b)`

Outputs the base `b` representation of `x`.

This is a [void](#void-functions) function.

##### `uint(x)`

Outputs the representation, in binary and hexadecimal, of `x` as an unsigned
integer in as few power of two bytes as possible. Both outputs are split into
bytes separated by spaces.

If `x` is not an integer or is negative, an error message is printed instead.

This is a [void](#void-functions) function.

##### `int(x)`

Outputs the representation, in binary and hexadecimal, of `x` as a signed,
two's-complement integer in as few power of two bytes as possible. Both outputs
are split into bytes separated by spaces.

If `x` is not an integer, an error message is printed instead.

This is a [void](#void-functions) function.

##### `uintn(x, n)`

Outputs the representation, in binary and hexadecimal, of `x` as an unsigned
integer in `n` bytes. Both outputs are split into bytes separated by spaces.

If `x` is not an integer, is negative, or cannot fit into `n` bytes, an error
message is printed instead.

##### `intn(x, n)`

Outputs the representation, in binary and hexadecimal, of `x` as an signed,
two's-complement integer in `n` bytes. Both outputs are split into bytes
separated by spaces.

If `x` is not an integer or cannot fit into `n` bytes, an error message is
printed instead.

##### `uint8(x)`

Outputs the representation, in binary and hexadecimal, of `x` as an unsigned
integer in `1` byte. Both outputs are split into bytes separated by spaces.

If `x` is not an integer, is negative, or cannot fit into `1` byte, an error
message is printed instead.

##### `int8(x)`

Outputs the representation, in binary and hexadecimal, of `x` as an signed,
two's-complement integer in `1` byte. Both outputs are split into bytes
separated by spaces.

If `x` is not an integer or cannot fit into `1` byte, an error message is
printed instead.

##### `uint16(x)`

Outputs the representation, in binary and hexadecimal, of `x` as an unsigned
integer in `2` bytes. Both outputs are split into bytes separated by spaces.

If `x` is not an integer, is negative, or cannot fit into `2` bytes, an error
message is printed instead.

##### `int16(x)`

Outputs the representation, in binary and hexadecimal, of `x` as an signed,
two's-complement integer in `2` bytes. Both outputs are split into bytes
separated by spaces.

If `x` is not an integer or cannot fit into `2` bytes, an error message is
printed instead.

##### `uint32(x)`

Outputs the representation, in binary and hexadecimal, of `x` as an unsigned
integer in `4` bytes. Both outputs are split into bytes separated by spaces.

If `x` is not an integer, is negative, or cannot fit into `4` bytes, an error
message is printed instead.

##### `int32(x)`

Outputs the representation, in binary and hexadecimal, of `x` as an signed,
two's-complement integer in `4` bytes. Both outputs are split into bytes
separated by spaces.

If `x` is not an integer or cannot fit into `4` bytes, an error message is
printed instead.

##### `uint64(x)`

Outputs the representation, in binary and hexadecimal, of `x` as an unsigned
integer in `8` bytes. Both outputs are split into bytes separated by spaces.

If `x` is not an integer, is negative, or cannot fit into `8` bytes, an error
message is printed instead.

##### `int64(x)`

Outputs the representation, in binary and hexadecimal, of `x` as an signed,
two's-complement integer in `8` bytes. Both outputs are split into bytes
separated by spaces.

If `x` is not an integer or cannot fit into `8` bytes, an error message is
printed instead.

##### `hex_uint(x, n)`

Outputs the representation of the truncated absolute value of `x` as a unsigned
integer in hexadecimal using `n` bytes. Not all of the value will be output if
`n` is too small.

This is a [void](#void-functions) function.

##### `binary_uint(x, n)`

Outputs the representation of the truncated absolute value of `x` as a unsigned
integer in binary using `n` bytes. Not all of the value will be output if `n` is
too small.

This is a [void](#void-functions) function.

##### `output_uint(x, n)`

Outputs the representation of the truncated absolute value of `x` as a unsigned
integer in the current [`obase`](#obase) using `n` bytes. Not all of the value
will be output if `n` is too small.

This is a [void](#void-functions) function.

##### `output_byte(x, i)`

Outputs byte `i` of the truncated absolute value of `x`, where `0` is the least
significant byte and `number_of_bytes - 1` is the most significant byte.

This is a [void](#void-functions) function.

## Command Line History

`bc` supports interactive command-line editing, if compiled with the
[history](./build.md#build-history) option enabled. If `stdin` is hooked to a
terminal, it is enabled. Previous lines can be recalled and edited with the
arrow keys.

## See Also

[`dc`(1)](./dc.md)

## Standards

The `bc` utility is compliant with the IEEE Std 1003.1-2017 (“POSIX.1-2017”)
specification. The flags `-efhiqsvVw`, all long options, and the extensions
noted above are extensions to that specification.

## Authors

This `bc` was made from scratch by [Gavin D. Howard][4].

## Bugs

None are known. Report bugs at [GitHub][5].

[1]: https://pubs.opengroup.org/onlinepubs/9699919799/utilities/bc.html
[2]: https://www.gnu.org/software/bc/
[3]: https://en.wikipedia.org/wiki/Rounding#Round_half_away_from_zero
[4]: https://gavinhoward.com/
[5]: https://github.com/gavinhoward/bc
