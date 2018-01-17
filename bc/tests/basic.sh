#!/bin/sh

EXE="../bc -l"
#EXE="bc -l"

cat basic_input.txt | $EXE
