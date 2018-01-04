#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "lex.h"

uint32_t ibase = 10;
uint32_t obase = 10;

int main(int argc, char* argv[]) {

	if (argc < 2) {
		return 1;
	}

	// Open the file and check for error.
	FILE* fp = fopen(argv[1], "rb");
	if (fp == NULL) {
		return 1;
	}

	// Get the size of the file.
	fseek(fp, 0, SEEK_END);
	size_t size = ftell(fp);

	// Reset the seek to the beginning.
	fseek(fp, 0, SEEK_SET);

	// Get the allocation size. We need an
	// extra byte if the file isn't binary.
	size_t alloc = size + 1;

	// Allocate the data and check for error.
	char* data = malloc(alloc);
	if (data == NULL) {
		return 1;
	}

	// Read the file and check for error.
	if (fread(data, 1, size, fp) != size) {
		return 1;
	}

	// If the file is not a binary file, we
	// must set a null terminating character.
	data[size] = '\0';

	// Close the file.
	fclose(fp);

	BcLex lex;
	BcLexToken token;

	bc_lex_init(&lex, data);

	bc_lex_next(&lex, &token);

	while (token.type != BC_LEX_EOF) {
		bc_lex_printToken(&token);
		bc_lex_next(&lex, &token);
	}

	bc_lex_printToken(&token);

	free(data);

	return 0;
}
