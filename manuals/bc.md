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

`ibase` is a global variable determining how to interpret constant numbers. It
is the "input" base, or the number base used for interpreting input numbers.
`ibase` is initially `10`.

`obase` is a global variable determining how to output results. It is the
"output" base, or the number base used for outputting numbers.
`obase` is initially `10`.

The **scale** of an expression is the number of digits in the result of the
expression right of the decimal point, and `scale` is a global variable setting
the precision of any operations, with exceptions. `scale` is initially `0`.

`bc` has both **global** variables and **local** variables. All **local**
variables are local to the function; they are parameters or are introduced in a
function's `auto` list (see [Functions](#bc-functions)). If a variable is
accessed which is not a parameter or in the `auto` list, it is assumed to be
**global**. If a parent function has a **local** variable version of a
**global** variable that is accessed by a function that it calls, the value of
that **global** variable in the child function is the value of the variable in
the parent function, not the value of the actual **global** variable.

All of the above applies to arrays as well.

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

<a name="named-expressions"/>

#### Named Expressions

1.	Variables: `I`
2.	Array Elements: `I[E]`
3.	`ibase`
4.	`obase`
5.	`scale`
6.	`last` or a single dot (`.`)

Variables and arrays do not interfere; users can have arrays named the same as
variables. This also applies to [functions](#bc-functions), so a user can have
a variable, array, and function that all have the same name.

#### Operands

1.	Numbers (see [Numbers](#bc-numbers) below).
2.	`(E)`: The value of `E` (used to change precedence).
3.	`sqrt(E)`: The square root of `E`.
4.	`length(E)`: The number of significant decimal digits in `E`.
5.	`length(I[])`: The number of elements in the array `I`. This is a
	non-portable extension.
6.	`scale(E)`: `E`'s **scale**.

<a name="bc-numbers"/>

#### Numbers

Numbers are strings made up of digits, uppercase letters, and at most `1` period
for a radix. Numbers can have up to `BC_NUM_MAX` digits. Uppercase letters
equal `9` + their position in the alphabet (i.e., `A` equals `10`, or `9 + 1`).
If a digit or letter makes no sense with the current value of `ibase`, they are
set to the value of the highest valid digit in `ibase`.

Single-character numbers (i.e., `A`) take the value that they would have if they
were valid digits, regardless of the value of `ibase`. This means that `A`
always equals decimal `10` and `Z` always equals decimal `35`.

#### Operators

The following arithmetic and logical operators can be used. They are listed in
order of decreasing precedence. Operators in the same group have the same
precedence.

| Operators   | Type               | Associativity | Description                     |
|-------------|--------------------|---------------|---------------------------------|
| `++` `--`   | Prefix and Postfix | None          | `increment`, `decrement`        |
| `-` `!`     | Prefix             | None          | `negation`, `boolean not`       |
| `$`         | Postfix            | None          | `truncation`                    |
| `@`         | Binary             | Right         | `set precision`                 |
| `^`         | Binary             | Right         | `power`                         |
| `*` `/` `%` | Binary             | Left          | `multiply`, `divide`, `modulus` |
| `+` `-`     | Binary             | Left          | `plus`, `subtract`              |
| `<<` `>>`   | Binary             | Left          | `shift left`, `shift right`     |
| `=` `<<=` `>>=` `+=` `-=` `*=` `/=` `%=` `^=` `@=` | Binary | Right | `assignment` |
| `==` `<=` `>=` `!=` `<` `>` | Binary | Left      | `relational`                    |
| `&&`        | Binary             | Left          | `boolean and`                   |
| <code>&#124;&#124;</code>   | Binary | Left      | `boolean or`                    |

They will be descrbed in more detail below.

##### `++` `--`

These are the prefix and postfix `increment` and `decrement` operators. They
behave exactly like they would in C.

##### `-`

This is the `negation` operator. If a user attempts to negate any expression
with the value `0`, an exact copy of the expression is returned. Otherwise, a
copy of the expression with its sign flipped is returned.

##### `!`

This is the `boolean not` operator. It returns `1` if the expression is `0`, or
`0` otherwise.

This is a non-portable extension.

##### `$`

This is the `truncation` operator. It returns a copy of the given expression
with all of its **scale** removed.

This is a non-portable extension.

This is only available if `bc` has been compiled with the
[extra math](./build.md#build-extra-math) option enabled.

##### `@`

This is the `set precision` operator. It takes two expressions and returns a
copy of the first with its **scale** equal to the value of the second
expression. That could either mean that the number is returned without change
(if the first expression's **scale** matches the value of the second
expression), extended (if it is less), or truncated (if it is more).

The second expression must be an integer (no **scale**) and non-negative.

This is a non-portable extension.

This is only available if `bc` has been compiled with the
[extra math](./build.md#build-extra-math) option enabled.

##### `^`

This is the `power` operator, not the `exclusive or` operator. It takes two
expressions and raises the first to the power of the value of the second.

The second expression must be an integer (no **scale**).

##### `*`

This is the `multiply` operator. It takes two expressions, multiplies them, and
returns the product. If `a` is the **scale** of the first expression and `b` is
the **scale** of the second expression, the scale of the result is equal to:

```
min(a + b, max(scale, a, b))
```

where `min` and `max` return the obvious values.

##### `/`

This is the `divide` operator. It takes two expressions, divides them, and
returns the quotient. The scale of the result shall be the value of `scale`.

##### `%`

This is the `modulus` operator. It takes two expressions, `a` and `b`, and
evaluates them according to the following steps:

1.	Compute `a/b` to current `scale`
2.	Use the result of step 1 to calculate `a-(a/b)*b` to scale
	`max(scale + scale(b), scale(a))`.

##### `+`

This is the `add` operator. It takes two expressions, `a` and `b`, and returns
the sum, with a scale equal to:

```
max(scale(a), scale(b))
```

where `max` returns the obvious value.

##### `-`

This is the `subtract` operator. It takes two expressions, `a` and `b`, and
returns the difference, with a scale equal to:

```
max(scale(a), scale(b))
```

where `max` returns the obvious value.

##### `<<`

This is the `left shift` operator. It takes two expressions, `a` and `b`, and
returns the value of `a` with its decimal point moved `b` places to the right.

The second expression must be an integer (no **scale**) and non-negative.

This is a non-portable extension.

This is only available if `bc` has been compiled with the
[extra math](./build.md#build-extra-math) option enabled.

##### `>>`

This is the `right shift` operator. It takes two expressions, `a` and `b`, and
returns the value of `a` with its decimal point moved `b` places to the left.

The second expression must be an integer (no **scale**) and non-negative.

This is a non-portable extension.

This is only available if `bc` has been compiled with the
[extra math](./build.md#build-extra-math) option enabled.

##### `=` `<<=` `>>=` `+=` `-=` `*=` `/=` `%=` `^=` `@=`

These are the `assignment` operators. They take two expressions, `a` and `b`
where `a` is a [named expression](#named-expressions).

For `=`, `b` is copied and the result is assigned to `a`. For all others, `a`
and `b` are applied as operands to the corresponding arithmetic operators and
the result is assigned to `a`.

The `assignment` operators that correspond to operators that are extensions are
themselves extensions.

Also, those `assignment` operators that are extensions are only available if
`bc` has been compiled with the [extra math](./build.md#build-extra-math) option
enabled.

##### `==` `<=` `>=` `!=` `<` `>`

These are the `relational` operators. They compare two expressions, `a` and `b`,
and if the relation holds, according to C language semantics, the result is `1`.
Otherwise, it is `0`.

Note that these operators have a lower precedence than the `assignment`
operators, which means that `a=b>c` is interpreted as `(a=b)>c`.

Also, unlike the [standard][1] requires, these operators can appear anywhere any
other expressions can be used. This allowance is an extension.

##### `&&`

This is the `boolean and` operator. It takes two expressions and returns `1` if
both expressions are non-zero, `1` otherwise.

This is ***not*** a short-circuit operator.

This is a non-portable extension.

##### `||`

This is the `boolean or` operator. It takes two expressions and returns `1` if
one of the expressions is non-zero, `1` otherwise.

This is ***not*** a short-circuit operator.

This is a non-portable extension.

#### Statements

The following items are statements:

1.	`E`
2.	`{` `S` `;` ... `;` `S` `}`
3.	`if` `(` `E` `)` `S`
4.	`if` `(` `E` `)` `S` `else` `S`
5.	`while` `(` `E` `)` `S`
6.	`for` `(` `E` `;` `E` `;` `E` `)` `S`
7.	An empty statement
8.	`break`
9.	`continue`
10.	`quit`
11.	`halt`
12.	`limits`
13.	A string of characters, enclosed in double quotes
14.	`print` `E` `,` ... `,` `E`

Numbers 4, 9, 11, 12, and 14 are extensions.

Also, as an extension, any or all of the expressions in the header of a for loop
may be omitted. If the condition (second expression) is omitted, it is assumed
to be a constant `1`.

The `continue` statement causes a loop iteration to stop early and returns to
the start of the loop, including testing the loop condition.

The `if` `else` statement does the same thing as in C.

The `quit` statement causes `bc` to quit, even if it is on a branch that will
not be executed (it is a compile-time command).

The `halt` statement causes `bc` to quit, if it is executed. (Unlike `quit` if
it is on a branch of an `if` statement that is not executed, `bc` does not
quit.)

The `limits` statement prints the limits that this `bc` is subject to. This is
like the `quit` statement in that it is a compile-time command.

##### Print Statement

The "expressions" in a `print` statement may also be strings. If they are, there
are backslash escape sequences that are interpreted specially. What those
sequences are, and what they cause to be printed, are shown below:

| Sequence | Prints          |
|----------|-----------------|
| `\a`     | alert           |
| `\b`     | backspace       |
| `\\`     | `\`             |
| `\e`     | `\`             |
| `\f`     | formfeed        |
| `\n`     | newline         |
| `\q`     | `"`             |
| `\r`     | carriage return |
| `\t`     | tab             |

Any other character following a backslash causes the backslash and character to
be printed as-is.

Any non-string expression in a print statement shall be assigned to `last`, like
any other expression that is printed.

<a name="bc-functions"/>

#### Functions

Function definitions follow what is required by the bc spec:

```
define I(I,...,I){
	auto I,...,I
	S;...;S
	return(E)
}
```

Any `I` in the parameter list or `auto` list may be replaced with `I[]` to make
a parameter or `auto` var an array.

As a non-portable extension, the opening brace of a `define` statement may
appear on the next line.

The return statement may also be in the following forms:

1.	`return`
2.	`return` `(` `)`
3.	`return` `E`

The first two, or not specifying a `return` statement, is equivalent to
`return (0)`.

<a name="void-functions"/>

##### Void Functions

Functions can also be void functions, defined as follows:

```
define void I(I,...,I){
	auto I,...,I
	S;...;S
	return(E)
}
```

They can only be used as standalone expressions, where such an expression would
be printed alone, except in a print statement.

Void functions can only use the first two `return` statements listed above.

The word `void` is not treated as a keyword; it is still possible to have
variables, arrays, and functions named `void`.

This is a non-portable extension.

##### Array References

For any array in the parameter list, if the array is declared in the form

```
*I[]
```

it is a **reference**. Any changes to the array in the function are reflected
when the function returns to the array that was passed in.

Other than this, all function arguments are passed by value.

This is a non-portable extension.

This is only available if `bc` has been compiled with the
[array references](./build.md#array-references) option enabled.

<a name="library"/>

### Library

All of the functions below, including the functions in the
[extended library](#extended-library) if `bc` has been compiled with the
[extra math](./build.md#build-extra-math) option enabled, are available when the `-l`
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

Returns the bessel integer order `n` (truncated) of `x`.

<a name="extended-library"/>

#### Extended Library

In addition to the [standard library](#standard-library), if `bc` has been built
with the [extra math](./build.md#build-extra-math) option, the following
functions are available when either the `-l` or `--mathlib` options are given.

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

Returns `pi` to `p` decimal places.

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

## Signal Handling

If `bc` has been compiled with the
[signal handling](./build.md#build-signal-handling), sending a `SIGINT` to it
will cause it to stop its current execution and reset, asking for more input.

Otherwise, any signals cause `bc` to exit.

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
