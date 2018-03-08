
void bc_main(void) {

  unsigned int flags;

  flags = (unsigned int) toys.optflags;

  toys.exitval = (char) bc_exec(flags, toys.optc, toys.optargs);
}
