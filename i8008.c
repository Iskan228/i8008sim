#include <stdio.h>
#include "i8008.h"

#define A(s) (s)->regs[0]
#define M(s) (s)->mem[(((s)->regs[5] << 8) + (s)->regs[6]) & 0x3FFF]
#define PC(s) (s)->stack[(s)->sp]
#define INC_PC(s) (PC(s) = (PC(s) + 1) & 0x3FFF)

#define C_flag 1
#define Z_flag 2
#define S_flag 4
#define P_flag 8

typedef void (*i8008_instr)(struct i8008_state *, u8);
#define state struct i8008_state
void HLT(state *st, u8 op) {
  /* IDK, do noting? */
}

void Lrr(state *st, u8 op) {
  const u8 src = op & 7, dst = (op >> 3) & 7;
  st->regs[dst] = st->regs[src];
}
void LrM(state *st, u8 op) {
  const u8 dst = (op >> 3) & 7;
  st->regs[dst] = M(st);
}
void LMr(state *st, u8 op) {
  const u8 src = op & 7;
  M(st) = st->regs[src];
}
void LrI(state *st, u8 op) {
  const u8 dst = op >> 3; /* No need in "& 7" since opcode is 00DDD110 */
  st->regs[dst] = st->mem[INC_PC(st)];
}
void LMI(state *st, u8 op) {
  M(st) = st->mem[INC_PC(st)];
}

void i8008_incorr_instr(state *st, u8 op) {
  fprintf(stderr, "Incorrect instruction with opcode %2x at address %4x\n", op, PC(st));
}
#define err i8008_incorr_instr

u8 popcnt[256] = {0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8};

void set_zsp(state *st, u8 value) {
  if (value == 0) {
    st->flags |= Z_flag;
  } else {
    st->flags &= ~Z_flag;
  }
  if (value & 0x80) {
    st->flags |= S_flag;
  } else {
    st->flags &= ~S_flag;
  }
  if (popcnt[value] % 2 == 0) {
    st->flags |= P_flag;
  } else {
    st->flags &= ~P_flag;
  }
}

void INr(state *st, u8 op) {
  set_zsp(st, ++st->regs[op >> 3]);
}
void DCr(state *st, u8 op) {
  set_zsp(st, --st->regs[op >> 3]);
}

void general_ADD(state *st, u8 value) {
  const u8 oldA = A(st);
  /* printf("A (%u) += %u\n", A(st), value); */
  set_zsp(st, A(st) += value);
  if (A(st) < oldA) {
    st->flags |= C_flag;
  } else {
    st->flags &= ~C_flag;
  }
}
void ADr(state *st, u8 op) {
  const u8 src = op & 7;
  general_ADD(st, st->regs[src]);
}
void ADM(state *st, u8 op) {
  general_ADD(st, M(st));
}
void ADI(state *st, u8 op) {
  const u8 src = st->mem[INC_PC(st)];
  general_ADD(st, src);
}

void general_ADC(state *st, u8 value) {
  const u8 oldA = A(st);
  set_zsp(st, A(st) += value + (st->flags & C_flag));
  if (A(st) < oldA) {
    st->flags |= C_flag;
  } else {
    st->flags &= ~C_flag;
  }
}
void ACr(state *st, u8 op) {
  const u8 src = op & 7;
  general_ADC(st, st->regs[src]);
}
void ACM(state *st, u8 op) {
  general_ADC(st, M(st));
}
void ACI(state *st, u8 op) {
  const u8 src = st->mem[INC_PC(st)];
  general_ADC(st, src);
}

void general_SUB(state *st, u8 value) {
  const u8 oldA = A(st);
  set_zsp(st, A(st) -= value);
  if (A(st) > oldA) {
    st->flags |= C_flag;
  } else {
    st->flags &= ~C_flag;
  }
}
void SUr(state *st, u8 op) {
  const u8 src = op & 7;
  general_SUB(st, st->regs[src]);
}
void SUM(state *st, u8 op) {
  general_SUB(st, M(st));
}
void SUI(state *st, u8 op) {
  const u8 src = st->mem[INC_PC(st)];
  general_SUB(st, src);
}

void general_SBB(state *st, u8 value) {
  const u8 oldA = A(st);
  set_zsp(st, A(st) -= value + (st->flags & C_flag));
  if (A(st) > oldA) {
    st->flags |= C_flag;
  } else {
    st->flags &= ~C_flag;
  }
}
void SBr(state *st, u8 op) {
  const u8 src = op & 7;
  general_SBB(st, st->regs[src]);
}
void SBM(state *st, u8 op) {
  general_SBB(st, M(st));
}
void SBI(state *st, u8 op) {
  const u8 src = st->mem[INC_PC(st)];
  general_SBB(st, src);
}

void general_AND(state *st, u8 value) {
  set_zsp(st, A(st) &= value);
  st->flags &= ~C_flag;
}
void NDr(state *st, u8 op) {
  const u8 src = op & 7;
  general_AND(st, st->regs[src]);
}
void NDM(state *st, u8 op) {
  general_AND(st, M(st));
}
void NDI(state *st, u8 op) {
  const u8 src = st->mem[INC_PC(st)];
  general_AND(st, src);
}

void general_XOR(state *st, u8 value) {
  set_zsp(st, A(st) ^= value);
  st->flags &= ~C_flag;
}
void XRr(state *st, u8 op) {
  const u8 src = op & 7;
  general_XOR(st, st->regs[src]);
}
void XRM(state *st, u8 op) {
  general_XOR(st, M(st));
}
void XRI(state *st, u8 op) {
  const u8 src = st->mem[INC_PC(st)];
  general_XOR(st, src);
}

void general_OR(state *st, u8 value) {
  /* printf("A (%u) |= %u\n", A(st), value); */
  set_zsp(st, A(st) |= value);
  st->flags &= ~C_flag;
}
void ORr(state *st, u8 op) {
  const u8 src = op & 7;
  general_OR(st, st->regs[src]);
}
void ORM(state *st, u8 op) {
  general_OR(st, M(st));
}
void ORI(state *st, u8 op) {
  const u8 src = st->mem[INC_PC(st)];
  general_OR(st, src);
}

void general_CMP(state *st, u8 value) {
  set_zsp(st, A(st) - value);
  if (A(st) < value) {
    st->flags |= C_flag;
  } else {
    st->flags &= ~C_flag;
  }
}
void CPr(state *st, u8 op) {
  const u8 src = op & 7;
  general_CMP(st, st->regs[src]);
}
void CPM(state *st, u8 op) {
  general_CMP(st, M(st));
}
void CPI(state *st, u8 op) {
  const u8 src = st->mem[INC_PC(st)];
  general_CMP(st, src);
}

void RLC(state *st, u8 op) {
  const u8 oldCF = st->flags & C_flag;
  if (A(st) & 0x80) {
    st->flags |= C_flag;
  } else {
    st->flags &= ~C_flag;
  }
  set_zsp(st, A(st) = (A(st) << 1) + oldCF);
}
void RRC(state *st, u8 op) {
  const u8 oldCF = st->flags & C_flag;
  if (A(st) & 1) {
    st->flags |= C_flag;
  } else {
    st->flags &= ~C_flag;
  }
  set_zsp(st, A(st) = (A(st) >> 1) + (oldCF << 7));
}
void RAL(state *st, u8 op) {
  if (A(st) & 0x80) {
    st->flags |= C_flag;
    A(st) = (A(st) << 1) + 1;
  } else {
    st->flags &= ~C_flag;
    A(st) <<= 1;
  }
  set_zsp(st, A(st));
}
void RAR(state *st, u8 op) {
  if (A(st) & 1) {
    st->flags |= C_flag;
    A(st) = (A(st) >> 1) + 0x80;
  } else {
    st->flags &= ~C_flag;
    A(st) >>= 1;
  }
  set_zsp(st, A(st));
}

void JMP(state *st, u8 op) {
  const u8 PCl = st->mem[INC_PC(st)];
  const u8 PCh = st->mem[INC_PC(st)];
  PC(st) = (PCh << 8) + PCl - 1; /* correction with "& 0x3FFF" is useless since it'll be done by INC_PC later */
}
void JFc(state *st, u8 op) {
  if (st->flags & (1 << ((op >> 3) & 3))) {
    PC(st) += 2;
  } else {
    JMP(st, op);
  }
}
void JTc(state *st, u8 op) {
  if (st->flags & (1 << ((op >> 3) & 3))) {
    JMP(st, op);
  } else {
    PC(st) += 2;
  }
}

void CAL(state *st, u8 op) {
  const u8 PCl = st->mem[INC_PC(st)];
  const u8 PCh = st->mem[INC_PC(st)];
  INC_PC(st); /* return address */
  st->sp = (st->sp + 1) & 7;
  PC(st) = (PCh << 8) + PCl - 1;
}
void CFc(state *st, u8 op) {
  if (st->flags & (1 << ((op >> 3) & 3))) {
    PC(st) += 2;
  } else {
    CAL(st, op);
  }
}
void CTc(state *st, u8 op) {
  if (st->flags & (1 << ((op >> 3) & 3))) {
    CAL(st, op);
  } else {
    PC(st) += 2;
  }
}

void RET(state *st, u8 op) {
  st->sp = (st->sp - 1) & 7;
  PC(st)--; /* will be corrected by INC_PC later */
}
void RFc(state *st, u8 op) {
  if (!(st->flags & (1 << ((op >> 3) & 3)))) {
    RET(st, op);
  }
}
void RTc(state *st, u8 op) {
  if (st->flags & (1 << ((op >> 3) & 3))) {
    RET(st, op);
  }
}

void RES(state *st, u8 op) {
  const u8 dst = op >> 3; /* "& 7" is useless since the opcode is 00nnn101 */
  INC_PC(st);
  st->sp = (st->sp + 1) & 7;
  PC(st) = dst << 3; /* actually can be optimized just to "PC(st) = op & 7" */
}

void INP(state *st, u8 op) {
  /* IDK what to do here */
}
void OUT(state *st, u8 op) {
  /* IDK what to do here */
}
#undef state

i8008_instr opcode_map[256] = {
/*x0,  x1,  x2,  x3,  x4,  x5,  x6,  x7,  x8,  x9,  xA,  xB,  xC,  xD,  xE,  xF */
  HLT, HLT, RLC, RFc, err, RES, LrI, RET, INr, DCr, RRC, RFc, err, RES, LrI, RET, /* 0x */
  INr, DCr, RAL, RFc, err, RES, LrI, RET, INr, DCr, RAR, RFc, err, RES, LrI, RET, /* 1x */
  INr, DCr, err, RTc, err, RES, LrI, RET, INr, DCr, err, RTc, err, RES, LrI, RET, /* 2x */
  INr, DCr, err, RTc, err, RES, LrI, RET, err, err, err, RTc, err, RES, LMI, RET, /* 3x */
  JFc, INP, CFc, INP, JMP, INP, CAL, INP, JFc, INP, CFc, INP, JMP, INP, CAL, INP, /* 4x */
  JFc, OUT, CFc, OUT, JMP, OUT, CAL, OUT, JFc, OUT, CFc, OUT, JMP, OUT, CAL, OUT, /* 5x */
  JTc, OUT, CTc, OUT, JMP, OUT, CAL, OUT, JTc, OUT, CTc, OUT, JMP, OUT, CAL, OUT, /* 6x */
  JTc, OUT, CTc, OUT, JMP, OUT, CAL, OUT, JTc, OUT, CTc, OUT, JMP, OUT, CAL, OUT, /* 7x */
  ADr, ADr, ADr, ADr, ADr, ADr, ADr, ADM, ACr, ACr, ACr, ACr, ACr, ACr, ACr, ACM, /* 8x */
  SUr, SUr, SUr, SUr, SUr, SUr, SUr, SUM, SBr, SBr, SBr, SBr, SBr, SBr, SBr, SBM, /* 9x */
  NDr, NDr, NDr, NDr, NDr, NDr, NDr, NDM, XRr, XRr, XRr, XRr, XRr, XRr, XRr, XRM, /* Ax */
  ORr, ORr, ORr, ORr, ORr, ORr, ORr, ORM, CPr, CPr, CPr, CPr, CPr, CPr, CPr, CPM, /* Bx */
  Lrr, Lrr, Lrr, Lrr, Lrr, Lrr, Lrr, LrM, Lrr, Lrr, Lrr, Lrr, Lrr, Lrr, Lrr, LrM, /* Cx */
  Lrr, Lrr, Lrr, Lrr, Lrr, Lrr, Lrr, LrM, Lrr, Lrr, Lrr, Lrr, Lrr, Lrr, Lrr, LrM, /* Dx */
  Lrr, Lrr, Lrr, Lrr, Lrr, Lrr, Lrr, LrM, Lrr, Lrr, Lrr, Lrr, Lrr, Lrr, Lrr, LrM, /* Ex */
  Lrr, Lrr, Lrr, Lrr, Lrr, Lrr, Lrr, LrM, Lrr, LMr, LMr, LMr, LMr, LMr, LMr, HLT  /* Fx */
};
#undef err
void i8008_run_instr(struct i8008_state *st) {
  const u8 op = st->mem[PC(st)];
  /* printf("op = %02x, PC = %04x\n", op, PC(st)); */
  opcode_map[op](st, op);
  INC_PC(st);
}

void i8008_run(struct i8008_state *st) {
  u8 op = st->mem[PC(st)];
  while (op != 0x00 && op != 0x01 && op != 0xFF) { /* while the instruction is not HLT */
    i8008_run_instr(st);
    op = st->mem[PC(st)];
  }
  INC_PC(st);
}
