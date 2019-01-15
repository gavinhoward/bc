# `dc`(1)

## Name

dc — arbitrary-precision reverse-Polish notation calculator

## Synopsis

`dc` [`-hvVx`] [`--version`] [`--help`] [`--extended-register`] [`-e` *expr*]
[`--expression=`*expr*...] [`-f` *file*...] [`-file=`*file*...] [*file*...]

## Description

`dc` is an arbitrary-precision calculator. It uses a stack (reverse Polish
notation) to store numbers and results of computations. Arithmetic operations
pop arguments off of the stack and push the results.

If no expressions or files are given on the command line, as in options below,
or for files, as extra command-line arguments, then `dc` reads from `stdin`.
Otherwise, those expressions and files are processed, and `dc` will then exit.

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

Prints a usage message and exits.

##### `-v` `-V` `--version`

Print the version information (copyright header) and exits.

##### `-x` `--extended-register`

Enables extended register mode. See [Registers](#dc-registers) for more
information.

<a name="dc-language"/>

## Language

`ibase` is a global variable determining how to interpret constant numbers. It
is the "input" base, or the number base used for interpreting input numbers.
`ibase` is initially `10`.

`obase` is a global variable determining how to output results. It is the
"output" base, or the number base used for outputting numbers.
`obase` is initially `10`.

The **scale** of an expression is the number of digits in the result of the
expression right of the decimal point, and `scale` is a global variable setting
the precision of any operations (with exceptions). `scale` is initially `0`.

Each item ([numbers](#dc-numbers) or [commands](#dc-commands)) in the input
source code is processed and executed, in order. Input is processed immediately
when entered.

#### Comments

Comments go from `#` until, and not including, the next newline. This is a
non-portable extension.

<a name="dc-numbers"/>

### Numbers

Numbers are strings made up of digits, uppercase letters up to `F`, and at most
`1` period for a radix. Numbers can have up to `BC_NUM_MAX` digits. Uppercase
letters equal `9` + their position in the alphabet (i.e., `A` equals `10`, or
`9 + 1`). If a digit or letter makes no sense with the current value of `ibase`,
they are set to the value of the highest valid digit in `ibase`.

Single-character numbers (i.e., `A`) take the value that they would have if they
were valid digits, regardless of the value of `ibase`. This means that `A`
always equals decimal `10` and `F` always equals decimal `15`.

<a name="dc-commands"/>

### Commands

The valid commands are listed below.

#### Printing

These commands are used for printing.

##### `p`

Prints the value on top of the stack, whether number or string, and prints a
newline after.

This does not alter the stack.

##### `n`

Prints the value on top of the tack, whether number or string, and pops it off
of the stack.

##### `P`

Pops a value off the stack.

If the value is a number, it is truncated and the result's absolute value is
printed as though `obase` is `UCHAR_MAX + 1` and each digit is interpreted as an
ASCII character, making it a byte stream.

If the value is a string, it is printed without a trailing newline.

This is a non-portable extension.

##### `f`

Prints the entire contents of the stack, in order from newest to oldest, without
altering anything.

Users should use this command when they get lost.

#### Arithmetic

These are the commands used for arithmetic.

##### `+`

The top two values are popped off the stack, added, and the result is pushed
onto the stack. The result's **scale** is equal to the max **scale** of both
operands.

##### `-`

The top two values are popped off the stack, subtracted, and the result is
pushed onto the stack. The result's **scale** is equal to the max **scale** of
both operands.

##### `*`

The top two values are popped off the stack, multiplied, and the result is
pushed onto the stack. If `a` is the **scale** of the first expression and `b`
is the **scale** of the second expression, the **scale** of the result is equal
to:

```
min(a + b, max(scale, a, b))
```

where `min` and `max` return the obvious values.

##### `/`

The top two values are popped off the stack, divided, and the result is pushed
onto the stack. The result's **scale** is equal to `scale`.

##### `%`

The top two values are popped off the stack, remaindered, and the result is
pushed onto the stack.

Remaindering is equivalent to the following steps:

1.	Compute `a/b` to current `scale`
2.	Use the result of step 1 to calculate `a-(a/b)*b` to **scale**
	`max(scale + scale(b), scale(a))`.

##### `~`

The top two values are popped off the stack, divided and remaindered, and the
results (divided first, remainder second) are pushed onto the stack. This is
equivalent to:

```
x y / x y %
```

except that `x` and `y` are only evaluated once.

This is a non-portable extension.

##### `^`

The top two values are popped off the stack, the second is raised to the power
of the first, and the result is pushed onto the stack.

The first value popped off the stack must be an integer.

##### `v`

The top value is popped off the stack, its square root is computed, and the
result is pushed onto the stack. The result's **scale** is equal to `scale`.

##### `_`

If this command *immediately* precedes a number (i.e., no spaces or other
commands), then that number is input as a negative number.

Otherwise, the top value on the stack is popped and copied, and the copy is
negated and pushed onto the stack. This behavior without a number is a
non-portable extension.

##### `|`

The top three values are popped off the stack, a modular exponentiation is
computed, and the result is pushed onto the stack.

The first value popped is used as the reduction modulus and must be an integer
and non-zero. The second value popped is used as the exponent and must be an
integer and non-negative. The third value popped is the base and must be an
integer.

This is a non-portable extension.

##### `$`

The top value is popped off the stack, truncated, and the result is pushed onto
the stack.

This is a non-portable extension.

##### `@`

The top two values are popped off the stack, and the second's precision is set
to the value of the first, whether by truncation or extension.

The first value must be an integer and non-negative.

This is a non-portable extension.

##### `H`

The top two values are popped off the stack, and the second is shifted left
(radix shifted right) to the value of the first.

The first value must be an integer and non-negative.

This is a non-portable extension.

##### `h`

The top two values are popped off the stack, and the second is shifted right
(radix shifted left) to the value of the first.

The first value must be an integer and non-negative.

This is a non-portable extension.

##### `G`

The top two values are popped off of the stack, they are compared, and a `1` is
pushed if they are equal, or `0` otherwise.

This is a non-portable extension.

##### `N`

The top value is popped off of the stack, and if it a `0`, a `1` is pushed;
otherwise, a `0` is pushed.

This is a non-portable extension.

##### `(`

The top two values are popped off of the stack, they are compared, and a `1` is
pushed if the first is less than the second, or `0` otherwise.

This is a non-portable extension.

##### `{`

The top two values are popped off of the stack, they are compared, and a `1` is
pushed if the first is less than or equal to the second, or `0` otherwise.

This is a non-portable extension.

##### `)`

The top two values are popped off of the stack, they are compared, and a `1` is
pushed if the first is greater than the second, or `0` otherwise.

This is a non-portable extension.

##### `}`

The top two values are popped off of the stack, they are compared, and a `1` is
pushed if the first is greater than or equal to the second, or `0` otherwise.

This is a non-portable extension.

#### Stack Control

These commands control the stack.

##### `c`

Removes all items from ("clears") the stack.

##### `d`

Copies the item on top of the stack ("duplicates") and pushes the copy onto the
stack.

##### `r`

Swaps ("reverses") the two top items on the stack.

##### `R`

Pops ("removes") the top value from the stack.

#### Register Control

These commands control [registers](#dc-registers).

##### `s`*r*

Pops the value off the top of the stack and stores it into register `r`.

##### `l`*r*

Copies the value in register `r` and pushes it onto the stack.

This does not alter the contents of `r`. Each register is also its own stack, so
the current register value is the top of the register's stack.

##### `S`*r*

Pops the value off the top of the (main) stack and pushes it onto the stack of
register `r`. The previous value of the register becomes inaccessible.

##### `L`*r*

Pops the value off the top of register `r`'s stack and push it onto the main
stack. The previous value in register `r`'s stack, if any, is now accessible via
the `l`*r* command.

#### Parameters

These commands control the values of `ibase`, `obase`, and `scale` (see
[Language](#dc-language)).

##### `i`

Pops the value off of the top of the stack and uses it to set `ibase`, which
must be between `2` and `16`, inclusive.

If the value on top of the stack has any **scale**, it is ignored.

##### `o`

Pops the value off of the top of the stack and uses it to set `obase`, which
must be between `2` and `BC_BASE_MAX`, inclusive (see [`bc`(1)](./bc.md)).

If the value on top of the stack has any **scale**, it is ignored.

##### `k`

Pops the value off of the top of the stack and uses it to set `scale`, which
must be non-negative.

If the value on top of the stack has any **scale**, it is ignored.

##### `I`

Pushes the current value of `ibase` onto the main stack.

##### `O`

Pushes the current value of `obase` onto the main stack.

##### `K`

Pushes the current value of `scale` onto the main stack.

#### Strings

The following commands control strings.

`dc` can work with both numbers and strings, and [registers](#dc-registers) can
hold both strings and numbers. `dc` always knows whether a register's contents
are a string or a number.

While arithmetic operations have to have numbers, and will print an error if
given a string, other commands accept strings.

Strings can also be executed as macros. For example, if the string `[1pR]` is
executed as a macro, then the code `1pR` is executed, meaning that the `1` will
be printed with a newline after and then popped from the stack.

##### `[`*characters*`]`

Makes a string containing `characters` and pushes it onto the stack.

If there are brackets (`[` and `]`) in the string, then they must be balanced.
Unbalanced brackets can be escaped using a backslash (`\`) character.

If there is a backslash character in the string, the character after it (even
another backslash) is put into the string verbatim, but the (first) backslash is
not.

##### `a`

The value on top of the stack is popped.

If it is a number, it is truncated and the result mod `UCHAR_MAX + 1` is
calculated. If that result is `0`, push an empty string; otherwise, push a
one-character string where the character is the result of the mod interpreted as
an ASCII character.

If it is a string, then a new string is made. If the original string is empty,
the new string is empty. If it is not, then the first character of the original
string is used to create the new string as a one-character string. The new
string is then pushed onto the stack.

This is a non-portable extension.

##### `x`

Pops a value off of the top of the stack.

If it is a number, it is pushed onto the stack.

If it is a string, it is executed as a macro.

This behavior is the norm whenever a macro is executed.

##### `>`*r*

Pops two values off of the stack that must be numbers and compares them. If the
first value is greater than the second, then the contents of register `r` are
executed.

Example:

```
0 1>a
1 0>a
```

The first example will execute the contents of register `a`, and the second will
not.

##### `>`*r*`e`*s*

Like the above, but will execute register `s` if the comparison fails.

This is a non-portable extension.

##### `!>`*r*

Pops two values off of the stack that must be numbers and compares them. If the
first value is not greater than the second (less than or equal to), then the
contents of register `r` are executed.

##### `!>`*r*`e`*s*

Like the above, but will execute register `s` if the comparison fails.

This is a non-portable extension.

##### `<`*r*

Pops two values off of the stack that must be numbers and compares them. If the
first value is less than the second, then the contents of register `r` are
executed.

##### `<`*r*`e`*s*

Like the above, but will execute register `s` if the comparison fails.

This is a non-portable extension.

##### `!<`*r*

Pops two values off of the stack that must be numbers and compares them. If the
first value is not less than the second (greater than or equal to), then the
contents of register `r` are executed.

##### `!<`*r*`e`*s*

Like the above, but will execute register `s` if the comparison fails.

This is a non-portable extension.

##### `=`*r*

Pops two values off of the stack that must be numbers and compares them. If the
first value is equal to the second (greater than or equal to), then the contents
of register `r` are executed.

##### `=`*r*`e`*s*

Like the above, but will execute register `s` if the comparison fails.

This is a non-portable extension.

##### `!=`*r*

Pops two values off of the stack that must be numbers and compares them. If the
first value is not equal to the second (greater than or equal to), then the
contents of register `r` are executed.

##### `!=`*r*`e`*s*

Like the above, but will execute register `s` if the comparison fails.

This is a non-portable extension.

##### `?`

Reads a line from the `stdin` and executes it. This is to allow macros to
request input from users.

##### `q`

During execution of a macro, this exits that macro's execution and the execution
of the macro that executed it. If there are no macros, or only one macro
executing, `dc` quits.

##### `Q`

Pops a value from the stack which must be non-negative and is used the number of
macro executions to pop off of the execution stack. If the number of levels to
pop is greater than the number of executing macros, `dc` exits.

#### Status

These commands query status of the stack or its top value.

##### `Z`

Pops a value off of the stack.

If it is a number, calculates the number of significant decimal digits it has
and pushes the result.

If it is a string, pushes the number of characters the string has.

##### `X`

Pops a value off of the stack.

If it is a number, pushes the **scale** of the value onto the stack.

If it is a string, pushes `0`.

##### `z`

Pushes the current stack depth (before execution of this command).

#### Arrays

These commands manipulate arrays.

##### `:`*r*

Pops the top two values off of the stack. The second value will be stored in the
array `r` (see [Registers](#dc-registers)), indexed by the first value.

##### `;`*r*

Pops the value on top of the stack and uses it as an index into the array `r`.
The selected value is then pushed onto the stack.

<a name="dc-registers"/>

### Registers

Registers are names that can store strings, numbers, and arrays. (Number/string
registers do not interfere with array registers.)

In non-extended register mode, a register name is just the single character that
follows any command that needs a register name. The only exception is a newline
(`'\n'`); it is a parse error for a newline to be used as a register name.

#### Extended Register Mode

Unlike most other `dc` implentations, this `dc` provides nearly unlimited
amounts of registers, if extended register mode is enabled.

If extended register mode is enabled (`-x` or `--extended-register` command-line
arguments are given), then normal single character registers are used
***unless*** the character immediately following a command that needs a register
name is a space (according to [`isspace()`][2]) and not a newline (`'\n'`).

In that case, the register name is found according to the regex
`[a-z][a-z0-9_]*` (like [`bc`(1)](./bc.md)), and it is a parse error if the next
non-space characters do not match that regex.

## Signal Handling

If `dc` has been compiled with the
[signal handling](./build.md#build-signal-handling), sending a `SIGINT` to it
will cause it to stop its current execution and reset, asking for more input.

Otherwise, any signals cause `dc` to exit.

## Command Line History

`dc` supports interactive command-line editing, if compiled with the
[history](./build.md#build-history) option enabled. If `stdin` is hooked to a
terminal, it is enabled. Previous lines can be recalled and edited with the
arrow keys.

## See Also

[`bc`(1)](./bc.md)

## Standards

The `dc` utility operators are compliant with the operators in the [`bc`(1)][1]
IEEE Std 1003.1-2017 (“POSIX.1-2017”) specification.

## Authors

This `dc` was made from scratch by [Gavin D. Howard][4].

## Bugs

None are known. Report bugs at [GitHub][5].

[1]: https://pubs.opengroup.org/onlinepubs/9699919799/utilities/bc.html
[2]: https://pubs.opengroup.org/onlinepubs/9699919799/functions/isspace.html
[4]: https://gavinhoward.com/
[5]: https://github.com/gavinhoward/bc
