#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "i8008.h"

int main(int argc, char *argv[]) {
  struct i8008_state st;
  FILE *f;

  if (argc != 2) {
    fprintf(stderr, "usage: %s <memory file>\n", argv[0]);
    return 0;
  }
  st.mem = malloc(sizeof(u8) * 0x4000);
  if (st.mem == NULL) {
    fputs("Failed to allocate 14 KB of memory\n", stderr);
    return 1;
  }
  f = fopen(argv[1], "rb");
  if (f == NULL) {
    free(st.mem);
    fprintf(stderr, "Failed to open file \"%s\"\n", argv[1]);
  } else {
    fread(st.mem, sizeof(u8), 0x4000, f);
    fclose(f);
  }
  memset(st.regs, 0, sizeof(st.regs));
  memset(st.stack, 0, sizeof(st.stack));
  st.sp = st.flags = 0;
  while (1) {
    char input[80];
    fgets(input, sizeof(input), stdin);
    switch (input[0]) {
    case 'R':
      i8008_run(&st);
      break;
    case '=': {
        const char r = input[1];
        if (r >= 'A' && r <= 'E') {
          st.regs[r - 'A'] = atoi(input + 2);
        } else if (r == 'H') {
          st.regs[5] = atoi(input + 2);
        } else if (r == 'L') {
          st.regs[6] = atoi(input + 2);
        } else if (r == 'M') {
          st.mem[((st.regs[5] << 8) + st.regs[6]) & 0x3FFF] = atoi(input + 2);
        } else if (r == 'S') {
          st.sp = ((u8)atoi(input + 2)) & 7;
        } else if (r == 'P') {
          st.stack[st.sp] = ((u16)atoi(input + 2)) & 0x3FFF;
        } else if (r == 'F') {
          st.flags = atoi(input + 2) & 0xF;
        } else {
          puts("Incorrect register name");
        }
      }
      break;
    case '?': {
        const char r = input[1];
        if (r >= 'A' && r <= 'E') {
          printf("%c = %u\n", r, st.regs[r - 'A']);
        } else if (r == 'H') {
          printf("H = %u\n", st.regs[5]);
        } else if (r == 'L') {
          printf("L = %u\n", st.regs[6]);
        } else if (r == 'M') {
          printf("M = %u\n", st.mem[((st.regs[5] << 8) + st.regs[6]) & 0x3FFF]);
        } else if (r == 'S') {
          printf("SP = %u\n", st.sp);
        } else if (r == 'P') {
          printf("PC = 0x%04x\n", st.stack[st.sp]);
        } else if (r == 'F') {
          printf("flags = 0x%02x\n", st.flags);
        } else {
          puts("Incorret register name");
        }
      }
      break;
    }
  }
  return 0;
}
