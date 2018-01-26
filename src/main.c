#include <string.h>

#include <locale.h>

#include <getopt.h>

#include <bc/bc.h>

static const struct option bc_opts[] = {

  { "help", no_argument, NULL, 'h' },
  { "interactive", no_argument, NULL, 'i' },
  { "mathlib", no_argument, NULL, 'l' },
  { "quiet", no_argument, NULL, 'q' },
  { "standard", no_argument, NULL, 's' },
  { "version", no_argument, NULL, 'v' },
  { "warn", no_argument, NULL, 'w' },
  { 0, 0, 0, 0 },

};

static const char* const bc_short_opts = "hlqsvw";

int main(int argc, char* argv[]) {

  unsigned int flags;
  unsigned int filec;
  const char** filev;

  setlocale(LC_ALL, "");

  flags = 0;

  // Getopt needs this.
  int opt_idx = 0;

  int c = getopt_long(argc, argv, bc_short_opts, bc_opts, &opt_idx);

  while (c != -1) {

    switch (c) {

      case 0:
      {
        // This is the case when a long option is
        // found, so we don't need to do anything.
        break;
      }

      case 'h':
      {
        flags |= BC_FLAG_HELP;
        break;
      }

      case 'i':
      {
        flags |= BC_FLAG_INTERACTIVE;
        break;
      }

      case 'l':
      {
        flags |= BC_FLAG_MATHLIB;
        break;
      }

      case 'q':
      {
        flags |= BC_FLAG_QUIET;
        break;
      }

      case 's':
      {
        flags |= BC_FLAG_STANDARD;
        break;
      }

      case 'v':
      {
        flags |= BC_FLAG_VERSION;
        break;
      }

      case 'w':
      {
        flags |= BC_FLAG_WARN;
        break;
      }

      // Getopt printed an error message, but we should exit.
      case '?':
      default:
      {
        return BC_STATUS_INVALID_OPTION;
        break;
      }
    }

    c = getopt_long(argc, argv, bc_short_opts, bc_opts, &opt_idx);
  }

  if (argv[optind] && strcmp(argv[optind], "--") == 0) {
    ++optind;
  }

  filec = argc - optind;
  filev = (const char**) (argv + optind);

  return bc_exec(flags, filec, filev);
}
