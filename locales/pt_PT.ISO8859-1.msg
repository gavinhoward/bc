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

1 "Erro de c�lculo:"
2 "Erro de an�lise de sintaxe:"
3 "Erro de execu��o:"
4 "Erro fatal:"
5 "Aviso:"

$ Math errors.
$set 2

1 "n�mero negativo"
2 "n�mero n�o inteiro"
3 "Estouro: n�mero n�o cabe no registro"
4 "dividir por 0"

$ Parse errors.
$set 3

1 "fim do arquivo"
2 "caractere inv�lido '%c'"
3 "N�o foi poss�vel encontrar o final da string"
4 "N�o foi poss�vel encontrar o final do coment�rio"
5 "token inv�lido"
6 "express�o inv�lida"
7 "express�o vazia"
8 "instru��o de grava��o ou de fluxo inv�lida"
9 "defini��o de fun��o inv�lida"
10 "atribui��o inv�lida: a parte esquerda deve ser 'scale', 'ibase', 'obase', 'last', uma vari�vel ou um elemento da matriz"
11 "nenhuma vari�vel autom�tica encontrada"
12 "par�metro de fun��o ou vari�vel autom�tica \"%s%s\" j� existe"
13 "fim do bloco n�o encontrado"
14 "uma fun��o 'void' n�o pode retornar um valor: %s()"
15 "Uma vari�vel n�o pode ser uma refer�ncia: %s"
16 "POSIX n�o permite nomes com mais de 1 caractere: %s"
17 "POSIX n�o permite coment�rios de script '#'"
18 "POSIX n�o permite a seguinte palavra-chave: %s"
19 "POSIX n�o permite um ponto ('.') como um atalho para o �ltimo resultado"
20 "POSIX requer par�nteses em torno de express�es de retorno"
21 "POSIX n�o permite o seguinte operador: %s"
22 "POSIX n�o permite operadores de compara��o fora das express�es 'if' ou loops"
23 "POSIX requer operadores 0 ou 1 de compara��o por condi��o"
24 "POSIX n�o permite uma express�o vazia em um loop 'for'"
25 "POSIX requer uma nova linha entre um ponto-e-v�rgula e uma defini��o de fun��o"
26 "POSIX n�o permite nota��o exponencial"
27 "POSIX n�o permite refer�ncias de matriz como par�metros de fun��o"
28 "POSIX n�o permite fun��es nulas"
29 "POSIX requer que o cabe�alho da fun��o '{' estejam na mesma linha"
30 "POSIX n�o permite a atribui��o de cadeias de caracteres a vari�veis ou matrizes"

$ Runtime errors.
$set 4

1 "ibase inv�lido: deve ser [%lu, %lu]"
2 "obase inv�lido: deve ser [%lu, %lu]"
3 "scale inv�lida: deve ser [%lu, %lu]"
4 "express�o read() inv�lida"
5 "chamada read() recursiva"
6 "tipo errado de vari�vel ou elemento de matriz"
7 "pilha tem poucos elementos"
8 "pilha para registo \"%s\" tem poucos elementos"
9 "n�mero incorreto de par�metros - esperado: %zu, obtido: %zu"
10 "fun��o indefinida: %s()"
11 "um valor 'void' n�o pode ser usado em uma express�o"

$ Fatal errors.
$set 5

1 "falha na aloca��o de mem�ria"
2 "erro de entrada-sa�da"
3 "imposs�vel abrir o arquivo: %s"
4 "arquivo n�o � texto: %s"
5 "caminho � um diret�rio: %s"
6 "op��o de linha de comando inv�lida: \"%s\""
7 "op��o requer um argumento: '%c' (\"%s\")"
8 "a op��o n�o aceita argumentos: '%c' (\"%s\")"
9 "argumento de op��o de linha de comando inv�lido: \"%s\""
