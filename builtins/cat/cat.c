#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

int8_t arg_mask = 0;

// arguments are tracked by masking 8 bits to the 8 args in order described by man cat [-belnstuv]

void set_mask(int n) {
  arg_mask |= (1 << n);
}

int get_mask(int n) {
  return arg_mask & (1 << n);
}

void init_mask(int argc, char **argv) {
  if (argc < 3) return;
  char ch;

  while ((ch = getopt(argc, argv, "belnstuv")) != -1) {
    switch (ch) {
      case 'b':
        set_mask(7); set_mask(4); // -b implies -n
        break;
      case 'e':
        set_mask(6); set_mask(0); // -e implies -v
        break;
      case 'l':
        set_mask(5);
        break;
      case 'n':
        set_mask(4);
        break;
      case 's':
        set_mask(3);
        break;
      case 't':
        set_mask(2); set_mask(0); // -t implies -v
        break;
      case 'u':
        set_mask(1);
        break;
      case 'v':
        set_mask(0);
        break;
      default:
        fprintf(stderr, "invalid argument '%c'\n", ch);
        break;
    }
  }
}

int main(int argc, char **argv) {
  init_mask(argc, argv);

  // handle buffering
  if (get_mask(1)) {
    setvbuf(stdout, NULL, _IONBF, 0);
  } else if (get_mask(5)) {
    setvbuf(stdout, NULL, _IOLBF, 0);
  }

  for (int i = optind; i < argc; i++) {
    FILE *fptr = fopen(argv[i], "r");
    if (!fptr) continue;

    int ch;
    int prev = '\n';
    int consecutive_nl = 0; 
    uint64_t line_number = 1;
    int start_of_line = 1;

    while ((ch = fgetc(fptr)) != EOF) {
      // handle squeeze
      if (get_mask(3)) { 
        if (ch == '\n' && prev == '\n') {
          consecutive_nl++;
          if (consecutive_nl > 1) {
            prev = ch;
            continue;
          }
        } else {
          consecutive_nl = 0;
        }
      }

      // handle line numbers
      if (start_of_line) {
        if ((get_mask(7) && ch != '\n') || (get_mask(4) && !get_mask(7))) {
          printf("%6llu  ", line_number++);
        } else if (get_mask(4) && !get_mask(7) && ch == '\n') {
          printf("%6llu  ", line_number++);
        }
        start_of_line = 0;
      }

      if (get_mask(6) && ch == '\n') {
        putchar('$');
      }
      
      // handle current char
      if (get_mask(0)) {
        if (ch >= 128) {
          printf("M-");
          ch -= 128;
        }

        if (ch < 32) {
          if ((ch == '\t' && !get_mask(2)) || (ch == '\n')) {
            putchar(ch);
          } else {
            printf("^%c", ch + 64);
          }
        } else if (ch == 127) {
          printf("^?");
        } else {
          putchar(ch);
        }
      } else {
        putchar(ch);
      }

      if (ch == '\n') {
        start_of_line = 1;
      }
      prev = ch;
    }
    fclose(fptr);
  }

  return 0;
}
