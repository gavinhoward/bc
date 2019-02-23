$ $
$ Copyright 2018 Gavin D. Howard
$ $
$ Permission to use, copy, modify, and/or distribute this software for any
$ purpose with or without fee is hereby granted.
$ $
$ THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
$ REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
$ AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
$ INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
$ LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
$ OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
$ PERFORMANCE OF THIS SOFTWARE.
$ $

$quote "

$ Headers for printing errors/warnings.
$set 1

1 "\n%s error: "
2 "\n%s warning: "

$ Error types.
$set 2

1 "Math"
2 "Parse"
3 "Runtime"
4 "Fatal"

$ Math errors.
$set 3

1 "negative number"
2 "non-integer number"
3 "overflow; number does not fit into a hardware type"
4 "divide by 0"

$ Parse errors.
$set 4

1 "end of file"
2 "invalid character (%c)"
3 "string end could not be found"
4 "comment end could not be found"
5 "invalid token"
6 "invalid expression"
7 "empty expression"
8 "invalid print statement"
9 "invalid function definition"
10 "invalid assignment: left side must be scale, ibase, obase, last, var, or array element"
11 "no auto variable found"
12 "function parameter or auto \"%s%s\" already exists"
13 "block end could not be found"
14 "cannot return a value from void function: %s()"
15 "var cannot be a reference: %s"
16 "POSIX does not allow names longer than 1 character: %s"
17 "POSIX does not allow '#' script comments"
18 "POSIX does not allow the following keyword: %s"
19 "POSIX does not allow a period ('.') as a shortcut for the last result"
20 "POSIX requires parentheses around return expressions"
21 "POSIX does not allow the following operator: %s"
22 "POSIX does not allow comparison operators outside if statements or loops"
23 "POSIX requires 0 or 1 comparison operators per condition"
24 "POSIX does not allow an empty init expression in a for loop"
25 "POSIX does not allow an empty condition expression in a for loop"
26 "POSIX does not allow an empty update expression in a for loop"
27 "POSIX does not allow exponential notation"
28 "POSIX does not allow array references as function parameters"
29 "POSIX requires the left brace be on the same line as the function header"

$ Runtime errors.
$set 5

1 "invalid ibase; must be [%lu, %lu]"
2 "invalid obase; must be [%lu, %lu]"
3 "invalid scale; must be [%lu, %lu]"
4 "invalid read() expression"
5 "recursive read() call"
6 "variable or array element is the wrong type"
7 "stack has too few elements"
8 "wrong number of parameters; need %zu, have %zu"
9 "undefined function: %s()"
10 "cannot use a void value in an expression"

$ Fatal errors.
$set 6

1 "memory allocation failed"
2 "I/O error"
3 "could not open file: %s"
4 "file is not ASCII: %s"
5 "path is a directory: %s"
6 "invalid command-line option: '%c' (\"%s\")"
