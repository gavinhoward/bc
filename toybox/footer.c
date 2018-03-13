
void bc_main(void) {
  toys.exitval = (char) bc_exec(toys.optflags, toys.optc, toys.optargs);
}
