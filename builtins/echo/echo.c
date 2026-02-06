#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) {
  int print_newline = 1;
  int start = 1;

  if (argc > 1 && strcmp(argv[1], "-n") == 0) {
    print_newline = 0;
    start = 2;
  }
  
  for (int i = start; i < argc; i++) {
    printf("%s", argv[i]);
    if (i + 1 < argc) printf(" ");
  }
  
  if (print_newline) printf("\n");
  
  return 0;
}