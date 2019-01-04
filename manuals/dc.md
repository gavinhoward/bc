# `dc`(1)

## Name

dc — arbitrary-precision reverse-Polish notation calculator

## Synopsis

`dc` [`-hvVx`] [`--help`] [`--extended-register`] [`-e` *expr*]
[`--expression=`*expr*...] [`-f` *file*...] [`-file=`*file*...] [*file*...]

## Description

`dc` is an arbitrary-precision calculator. It uses a stack (reverse Polish
notation) to store numbers and results of computations. Arithmetic operations
pop arguments off of the stack and push the results.

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

##### `-v` `-V` `--version`

Print the version information (copyright header) and exit.

##### `-x` `--extended-register`

Enables extended register mode. See Registers for more information.

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
[4]: https://gavinhoward.com/
[5]: https://github.com/gavinhoward/bc
