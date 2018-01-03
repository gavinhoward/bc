#include <stdio.h>
// This is not even close to started, pnly the basic posix defs are completed 

/* As per IEEE Std 1003.1-2008, 2016 Edition */

/* 
obase levels 0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F 
*/

/*
Operator			Associativity

++, -- 				N/A

unary - 			N/A

^ 				Right to left

*, /, % 			Left to right

+, binary - 			Left to right

=, +=, -=, *=, /=, %=, ^= 	Right to left

==, <=, >=, !=, <, > 		None
*/

char *NUMBER[] = {
"0",
"1",
"2",
"3",
"4",
"5",
"6",
"7",
"8",
"9",
"A",
"B",
"C",
"D",
"E",
"F"
};

char *LETTER[] = {
"a",
"b",
"c",
"d",
"e",
"f",
"g",
"h",
"i",
"j",
"k",
"l",
"m",
"n",
"o",
"p",
"q",
"r",
"s",
"t",
"u",
"v",
"w",
"x",
"y",
"z"
};

char *tokens[] = {
"auto",
"break",
"define", 
"ibase",
"if",
"for", 
"length",
"obase",
"quit",
"return",
"scale",
"sqrt",
"while"
};

char *ASSIGN_OP[] = {
"=",
"+=",
"-=",
"*=",
"/=",
"%=",
"^=" 
};

char *MUL_OP[] = {
"*",
"/",
"%" 
};

/* 
Unlike all other operators, the relational operators [...] shall be only 
   valid as the object of an if, while, or inside a for statement
*/
char *REL_OP[] = {
"==",
"<=",
">=",
"!=",
"<",
">"
};

char *INCR_DECR[] = {
"++",
"--"
};

char *SYNTAX[] = {
"\n",
"(",
")",
",",
"+",
"-",
";",
"[",
"]",
"^",
"{",
"}"
};



int main()
{ 
	return 0;
}
