$ $
$ SPDX-License-Identifier: BSD-2-Clause
$ $
$ Copyright (c) 2018-2025 Gavin D. Howard and contributors.
$ $
$ Redistribution and use in source and binary forms, with or without
$ modification, are permitted provided that the following conditions are met:
$ $
$ * Redistributions of source code must retain the above copyright notice, this
$   list of conditions and the following disclaimer.
$ $
$ * Redistributions in binary form must reproduce the above copyright notice,
$   this list of conditions and the following disclaimer in the documentation
$   and/or other materials provided with the distribution.
$ $
$ THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
$ AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
$ IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
$ ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
$ LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
$ CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
$ SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
$ INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
$ CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
$ ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
$ POSSIBILITY OF SUCH DAMAGE.
$ $

$quote "

$ Error types.
$set 1

1 "Error de matem�tica:"
2 "Error de syntaxis:"
3 "Error de ejecuci�n:"
4 "Error fatal:"
5 "Advertencia:"

$ Math errors.
$set 2

1 "n�mero negativo"
2 "n�mero no es entero"
3 "desbordamiento de enteros: no se puede encajar el el hardware"
4 "divisi�n por cero"

$ Parse errors.
$set 3

1 "fin de archivo"
2 "no v�lido '%c'"
3 "no puede encontrar el fine de la cadena"
4 "no puede encontrar el fine del comentario"
5 "el token no es v�lido"
6 "la expresi�n no es v�lida"
7 "la expresi�n es vac�a"
8 "la expresi�n de print o de stream no es v�lida"
9 "la definici�n de funci�n no es v�lida"
10 "la asignaci�n no es valida: en la izquierda debe ser scale, ibase, obase, last, var, o un elemento de matriz"
11 "no se encontr� ninguna variable autom�tica"
12 "ya hay un par�metro de funci�n o variable automatica que se llama \"%s%s\""
13 "no se puede encontrar el final de del bloque de c�digo"
14 "no puede haber un valor de retorno de una funci�n \"void\": %s()"
15 "var no puede ser una referencia: %s"
16 "POSIX no permite nombres de m�s de 1 car�cter: %s"
17 "POSIX no permite '#' script comentarios"
18 "POSIX no permite este palabra clave %s"
19 "POSIX no permite un punto ('.') como un atajo del resultado previoso"
20 "POSIX requieres par�ntesis en el expresi�n del \"return\""
21 "POSIX no permite este operador: %s"
22 "POSIX no permite operadores de comparaci�n aparte de \"if\" expresi�n o bucles"
23 "POSIX requiere 0 o 1 operadores de comparis�n para cada condici�n"
24 "POSIX requiere todos 3 partes de una bucla que no esta vac�o"
25 "POSIX requiere una nueva l�nea entre un punto y coma y una definici�n de funci�n"
26 "POSIX no permite una notaci�n exponencial"
27 "POSIX no permite una referencia a una matriz como un par�metro de funci�n"
28 "POSIX no permite funciones void"
29 "POSIX requiere el llave de la izquierda que sea en la misma l�nea que los par�metros de la funci�n"
30 "POSIX no permite asignar cadenas a variables o matrices"

$ Runtime errors.
$set 4

1 "\"ibase\" no es v�lido: debe ser [%lu, %lu]"
2 "\"obase\" no es v�lido: debe ser [%lu, %lu]"
3 "\"scale\" no es v�lido: debe ser [%lu, %lu]"
4 "read() expresi�n no es v�lido"
5 "recursion en la invocaci�n de read()"
6 "variable o elemento del matriz de tipo equivocado"
7 "la pila no ha demaciado elementos"
8 "la pila del registro \"%s\" no ha demaciado elementos"
9 "la funci�n no tiene un n�mero de argumentos correcto; necessita %zu, tiene %zu"
10 "la funci�n no esta definida: %s()"
11 "no puede utilizar un valor vac�o en una expresi�n"

$ Fatal errors.
$set 5

1 "error en la asignaci�n de memoria"
2 "error de I/O"
3 "no puede abrir el archivo: %s"
4 "el archivo no es texto: %s"
5 "el ruta es un directorio: %s"
6 "una opci�n de l�nea de comandos no es v�lida: \"%s\""
7 "una opci�n requiere un argumento: '%c' (\"%s\")"
8 "una opci�n no tiene argumento: '%c' (\"%s\")"
9 "uno argumento de opci�n de l�nea de comandos no es v�lido: \"%s\""
