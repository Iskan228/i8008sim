#ifndef _I8008_H_
#define _I8008_H_

typedef unsigned char u8;
typedef unsigned short u16;

struct i8008_state {
  u8 regs[7]; /* A, B, C, D, E, H, L */
  u16 stack[8]; /* 14-bit PC0..PC7 */
  u8 sp; /* 0..7 */
  u8 flags; /* 0000PSZC */
  u8 *mem; /* 14 KB of memory */
};

void i8008_run_instr(struct i8008_state *);
void i8008_run(struct i8008_state *);

#endif /* _I8008_H_ */
