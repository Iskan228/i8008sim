#include <stdio.h>
#include <ctype.h>

int hex2dec(int c) {
  if (c >= '0' && c <= '9') {
    return c - '0';
  } else if (c >= 'A' && c <= 'F') {
    return c - 'A' + 10;
  } else if (c >= 'a' && c <= 'f') {
    return c - 'a' + 10;
  } else {
    return -1;
  }
}

int main() {
  int c;
  FILE *f = fopen("out.hex", "wb");
  if (f == NULL) {
    puts("Error 1");
    return 0;
  }
  while ((c = getchar()) != EOF && c != 'Q') {
    int n;
    if (!isxdigit(c)) continue;
    n = hex2dec(c);
    c = getchar();
    if (isxdigit(c)) {
      n = (n << 4) + hex2dec(c);
    }
    putc(n, f);
    if (c == EOF || c == 'Q') break;
  }
  fclose(f);
  return 0;
}
