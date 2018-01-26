#include <locale.h>

#include <bc/bc.h>

int main(int argc, char* argv[]) {
  setlocale(LC_ALL, "");
  return bc_main(argc, argv);
}
