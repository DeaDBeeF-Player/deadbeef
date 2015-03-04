static void BTST_reg(int b, int reg0)
{
  u32 a = reg68.d[reg0];
  b &= 31;
  BTST(a,b);
  ADDCYCLE(2);
}

static void BTST_mem(int b, int mode, int reg0)
{
  u32 addr = get_eab[mode](reg0);
  u32 a = read_B(addr);
  b &= 7;
  BTST(a,b);
  ADDCYCLE(0);
}

static void BCHG_reg(int b, int reg0)
{
  u32 a = reg68.d[reg0];
  b &= 31;
  BCHG(a,b);
  reg68.d[reg0] = a;
  ADDCYCLE(4);
}

static void BCHG_mem(int b, int mode, int reg0)
{
  u32 addr = get_eab[mode](reg0);
  u32 a = read_B(addr);
  b &= 7;
  BCHG(a,b);
  write_B(addr,a);
  ADDCYCLE(4);
}

static void BCLR_reg(int b, int reg0)
{
  u32 a = reg68.d[reg0];
  b &= 31;
  BCLR(a,b);
  reg68.d[reg0] = a;
  ADDCYCLE(6);
}

static void BCLR_mem(int b, int mode, int reg0)
{
  u32 addr = get_eab[mode](reg0);
  u32 a = read_B(addr);
  b &= 7;
  BCLR(a,b);
  write_B(addr,a);
  ADDCYCLE(4);
}

static void BSET_reg(int b, int reg0)
{
  u32 a = reg68.d[reg0];
  b &= 31;
  BSET(a,b);
  reg68.d[reg0] = a;
  ADDCYCLE(4);
}

static void BSET_mem(int b, int mode, int reg0)
{
  u32 addr = get_eab[mode](reg0);
  u32 a = read_B(addr);
  b &= 7;
  BSET(a,b);
  write_B(addr,a);
  ADDCYCLE(4);
}

static void l0_ill(u32 reg0)
{
  reg0=reg0; ILLEGAL;
}

static void l0_ORb0(u32 reg0)
{
  /* OR.B #imm,Dy */
  s32 a,b,s;
  a = get_nextw()<<24;
  b = reg68.d[reg0]<<24;
  ORB(s,a,b);
  reg68.d[reg0] = (reg68.d[reg0]&0xffffff00) + ((u32)s>>24);
}

static void l0_ANDb0(u32 reg0)
{
  /* AND.B #imm,Dy */
  s32 a,b,s;
  a = get_nextw()<<24;
  b = reg68.d[reg0]<<24;
  ANDB(s,a,b);
  reg68.d[reg0] = (reg68.d[reg0]&0xffffff00) + ((u32)s>>24);
}

static void l0_EORb0(u32 reg0)
{
  /* EOR.B #imm,Dy */
  s32 a,b,s;
  a = get_nextw()<<24;
  b = reg68.d[reg0]<<24;
  EORB(s,a,b);
  reg68.d[reg0] = (reg68.d[reg0]&0xffffff00) + ((u32)s>>24);
}

static void l0_ADDb0(u32 reg0)
{
  /* ADD.B #imm,Dy */
  s32 a,b,s;
  a = get_nextw()<<24;
  b = reg68.d[reg0]<<24;
  ADDB(s,a,b);
  reg68.d[reg0] = (reg68.d[reg0]&0xffffff00) + ((u32)s>>24);
}

static void l0_SUBb0(u32 reg0)
{
  /* SUB.B #imm,Dy */
  s32 a,b,s;
  a = get_nextw()<<24;
  b = reg68.d[reg0]<<24;
  SUBB(s,a,b);
  reg68.d[reg0] = (reg68.d[reg0]&0xffffff00) + ((u32)s>>24);
}

static void l0_CMPb0(u32 reg0)
{
  /* CMP.B #imm,Dy */
  s32 a,b;
  a = get_nextw()<<24;
  b = reg68.d[reg0]<<24;
  CMPB(a,b);
}

static void l0_ORw0(u32 reg0)
{
  /* OR.W #imm,Dy */
  s32 a,b,s;
  a = get_nextw()<<16;
  b = reg68.d[reg0]<<16;
  ORW(s,a,b);
  reg68.d[reg0] = (reg68.d[reg0]&0xffff0000) + ((u32)s>>16);
}

static void l0_ANDw0(u32 reg0)
{
  /* AND.W #imm,Dy */
  s32 a,b,s;
  a = get_nextw()<<16;
  b = reg68.d[reg0]<<16;
  ANDW(s,a,b);
  reg68.d[reg0] = (reg68.d[reg0]&0xffff0000) + ((u32)s>>16);
}

static void l0_EORw0(u32 reg0)
{
  /* EOR.W #imm,Dy */
  s32 a,b,s;
  a = get_nextw()<<16;
  b = reg68.d[reg0]<<16;
  EORW(s,a,b);
  reg68.d[reg0] = (reg68.d[reg0]&0xffff0000) + ((u32)s>>16);
}

static void l0_ADDw0(u32 reg0)
{
  /* ADD.W #imm,Dy */
  s32 a,b,s;
  a = get_nextw()<<16;
  b = reg68.d[reg0]<<16;
  ADDW(s,a,b);
  reg68.d[reg0] = (reg68.d[reg0]&0xffff0000) + ((u32)s>>16);
}

static void l0_SUBw0(u32 reg0)
{
  /* SUB.W #imm,Dy */
  s32 a,b,s;
  a = get_nextw()<<16;
  b = reg68.d[reg0]<<16;
  SUBW(s,a,b);
  reg68.d[reg0] = (reg68.d[reg0]&0xffff0000) + ((u32)s>>16);
}

static void l0_CMPw0(u32 reg0)
{
  /* CMP.W #imm,Dy */
  s32 a,b;
  a = get_nextw()<<16;
  b = reg68.d[reg0]<<16;
  CMPW(a,b);
}

static void l0_ORl0(u32 reg0)
{
  /* OR.L #imm,Dy */
  s32 a,b,s;
  a = get_nextl()<<0;
  b = reg68.d[reg0]<<0;
  ORL(s,a,b);
  reg68.d[reg0] = (reg68.d[reg0]&0) + ((u32)s>>0);
}

static void l0_ANDl0(u32 reg0)
{
  /* AND.L #imm,Dy */
  s32 a,b,s;
  a = get_nextl()<<0;
  b = reg68.d[reg0]<<0;
  ANDL(s,a,b);
  reg68.d[reg0] = (reg68.d[reg0]&0) + ((u32)s>>0);
}

static void l0_EORl0(u32 reg0)
{
  /* EOR.L #imm,Dy */
  s32 a,b,s;
  a = get_nextl()<<0;
  b = reg68.d[reg0]<<0;
  EORL(s,a,b);
  reg68.d[reg0] = (reg68.d[reg0]&0) + ((u32)s>>0);
}

static void l0_ADDl0(u32 reg0)
{
  /* ADD.L #imm,Dy */
  s32 a,b,s;
  a = get_nextl()<<0;
  b = reg68.d[reg0]<<0;
  ADDL(s,a,b);
  reg68.d[reg0] = (reg68.d[reg0]&0) + ((u32)s>>0);
}

static void l0_SUBl0(u32 reg0)
{
  /* SUB.L #imm,Dy */
  s32 a,b,s;
  a = get_nextl()<<0;
  b = reg68.d[reg0]<<0;
  SUBL(s,a,b);
  reg68.d[reg0] = (reg68.d[reg0]&0) + ((u32)s>>0);
}

static void l0_CMPl0(u32 reg0)
{
  /* CMP.L #imm,Dy */
  s32 a,b;
  a = get_nextl()<<0;
  b = reg68.d[reg0]<<0;
  CMPL(a,b);
}

static void l0_ORb2(u32 reg0)
{
  /* OR.B #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = get_nextw()<<24;
  addr = get_eab[2](reg0);
  b = read_B(addr)<<24;
  ORB(s,a,b);
  write_B(addr,(u32)s>>24);
}

static void l0_ANDb2(u32 reg0)
{
  /* AND.B #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = get_nextw()<<24;
  addr = get_eab[2](reg0);
  b = read_B(addr)<<24;
  ANDB(s,a,b);
  write_B(addr,(u32)s>>24);
}

static void l0_EORb2(u32 reg0)
{
  /* EOR.B #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = get_nextw()<<24;
  addr = get_eab[2](reg0);
  b = read_B(addr)<<24;
  EORB(s,a,b);
  write_B(addr,(u32)s>>24);
}

static void l0_ADDb2(u32 reg0)
{
  /* ADD.B #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = get_nextw()<<24;
  addr = get_eab[2](reg0);
  b = read_B(addr)<<24;
  ADDB(s,a,b);
  write_B(addr,(u32)s>>24);
}

static void l0_SUBb2(u32 reg0)
{
  /* SUB.B #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = get_nextw()<<24;
  addr = get_eab[2](reg0);
  b = read_B(addr)<<24;
  SUBB(s,a,b);
  write_B(addr,(u32)s>>24);
}

static void l0_CMPb2(u32 reg0)
{
  /* CMP.B #imm,<Ae> */
  s32 a,b;
  u32 addr;
  a = get_nextw()<<24;
  addr = get_eab[2](reg0);
  b = read_B(addr)<<24;
  CMPB(a,b);
}

static void l0_ORw2(u32 reg0)
{
  /* OR.W #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = get_nextw()<<16;
  addr = get_eaw[2](reg0);
  b = read_W(addr)<<16;
  ORW(s,a,b);
  write_W(addr,(u32)s>>16);
}

static void l0_ANDw2(u32 reg0)
{
  /* AND.W #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = get_nextw()<<16;
  addr = get_eaw[2](reg0);
  b = read_W(addr)<<16;
  ANDW(s,a,b);
  write_W(addr,(u32)s>>16);
}

static void l0_EORw2(u32 reg0)
{
  /* EOR.W #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = get_nextw()<<16;
  addr = get_eaw[2](reg0);
  b = read_W(addr)<<16;
  EORW(s,a,b);
  write_W(addr,(u32)s>>16);
}

static void l0_ADDw2(u32 reg0)
{
  /* ADD.W #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = get_nextw()<<16;
  addr = get_eaw[2](reg0);
  b = read_W(addr)<<16;
  ADDW(s,a,b);
  write_W(addr,(u32)s>>16);
}

static void l0_SUBw2(u32 reg0)
{
  /* SUB.W #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = get_nextw()<<16;
  addr = get_eaw[2](reg0);
  b = read_W(addr)<<16;
  SUBW(s,a,b);
  write_W(addr,(u32)s>>16);
}

static void l0_CMPw2(u32 reg0)
{
  /* CMP.W #imm,<Ae> */
  s32 a,b;
  u32 addr;
  a = get_nextw()<<16;
  addr = get_eaw[2](reg0);
  b = read_W(addr)<<16;
  CMPW(a,b);
}

static void l0_ORl2(u32 reg0)
{
  /* OR.L #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = get_nextl()<<0;
  addr = get_eal[2](reg0);
  b = read_L(addr)<<0;
  ORL(s,a,b);
  write_L(addr,(u32)s>>0);
}

static void l0_ANDl2(u32 reg0)
{
  /* AND.L #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = get_nextl()<<0;
  addr = get_eal[2](reg0);
  b = read_L(addr)<<0;
  ANDL(s,a,b);
  write_L(addr,(u32)s>>0);
}

static void l0_EORl2(u32 reg0)
{
  /* EOR.L #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = get_nextl()<<0;
  addr = get_eal[2](reg0);
  b = read_L(addr)<<0;
  EORL(s,a,b);
  write_L(addr,(u32)s>>0);
}

static void l0_ADDl2(u32 reg0)
{
  /* ADD.L #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = get_nextl()<<0;
  addr = get_eal[2](reg0);
  b = read_L(addr)<<0;
  ADDL(s,a,b);
  write_L(addr,(u32)s>>0);
}

static void l0_SUBl2(u32 reg0)
{
  /* SUB.L #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = get_nextl()<<0;
  addr = get_eal[2](reg0);
  b = read_L(addr)<<0;
  SUBL(s,a,b);
  write_L(addr,(u32)s>>0);
}

static void l0_CMPl2(u32 reg0)
{
  /* CMP.L #imm,<Ae> */
  s32 a,b;
  u32 addr;
  a = get_nextl()<<0;
  addr = get_eal[2](reg0);
  b = read_L(addr)<<0;
  CMPL(a,b);
}

static void l0_ORb3(u32 reg0)
{
  /* OR.B #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = get_nextw()<<24;
  addr = get_eab[3](reg0);
  b = read_B(addr)<<24;
  ORB(s,a,b);
  write_B(addr,(u32)s>>24);
}

static void l0_ANDb3(u32 reg0)
{
  /* AND.B #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = get_nextw()<<24;
  addr = get_eab[3](reg0);
  b = read_B(addr)<<24;
  ANDB(s,a,b);
  write_B(addr,(u32)s>>24);
}

static void l0_EORb3(u32 reg0)
{
  /* EOR.B #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = get_nextw()<<24;
  addr = get_eab[3](reg0);
  b = read_B(addr)<<24;
  EORB(s,a,b);
  write_B(addr,(u32)s>>24);
}

static void l0_ADDb3(u32 reg0)
{
  /* ADD.B #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = get_nextw()<<24;
  addr = get_eab[3](reg0);
  b = read_B(addr)<<24;
  ADDB(s,a,b);
  write_B(addr,(u32)s>>24);
}

static void l0_SUBb3(u32 reg0)
{
  /* SUB.B #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = get_nextw()<<24;
  addr = get_eab[3](reg0);
  b = read_B(addr)<<24;
  SUBB(s,a,b);
  write_B(addr,(u32)s>>24);
}

static void l0_CMPb3(u32 reg0)
{
  /* CMP.B #imm,<Ae> */
  s32 a,b;
  u32 addr;
  a = get_nextw()<<24;
  addr = get_eab[3](reg0);
  b = read_B(addr)<<24;
  CMPB(a,b);
}

static void l0_ORw3(u32 reg0)
{
  /* OR.W #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = get_nextw()<<16;
  addr = get_eaw[3](reg0);
  b = read_W(addr)<<16;
  ORW(s,a,b);
  write_W(addr,(u32)s>>16);
}

static void l0_ANDw3(u32 reg0)
{
  /* AND.W #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = get_nextw()<<16;
  addr = get_eaw[3](reg0);
  b = read_W(addr)<<16;
  ANDW(s,a,b);
  write_W(addr,(u32)s>>16);
}

static void l0_EORw3(u32 reg0)
{
  /* EOR.W #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = get_nextw()<<16;
  addr = get_eaw[3](reg0);
  b = read_W(addr)<<16;
  EORW(s,a,b);
  write_W(addr,(u32)s>>16);
}

static void l0_ADDw3(u32 reg0)
{
  /* ADD.W #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = get_nextw()<<16;
  addr = get_eaw[3](reg0);
  b = read_W(addr)<<16;
  ADDW(s,a,b);
  write_W(addr,(u32)s>>16);
}

static void l0_SUBw3(u32 reg0)
{
  /* SUB.W #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = get_nextw()<<16;
  addr = get_eaw[3](reg0);
  b = read_W(addr)<<16;
  SUBW(s,a,b);
  write_W(addr,(u32)s>>16);
}

static void l0_CMPw3(u32 reg0)
{
  /* CMP.W #imm,<Ae> */
  s32 a,b;
  u32 addr;
  a = get_nextw()<<16;
  addr = get_eaw[3](reg0);
  b = read_W(addr)<<16;
  CMPW(a,b);
}

static void l0_ORl3(u32 reg0)
{
  /* OR.L #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = get_nextl()<<0;
  addr = get_eal[3](reg0);
  b = read_L(addr)<<0;
  ORL(s,a,b);
  write_L(addr,(u32)s>>0);
}

static void l0_ANDl3(u32 reg0)
{
  /* AND.L #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = get_nextl()<<0;
  addr = get_eal[3](reg0);
  b = read_L(addr)<<0;
  ANDL(s,a,b);
  write_L(addr,(u32)s>>0);
}

static void l0_EORl3(u32 reg0)
{
  /* EOR.L #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = get_nextl()<<0;
  addr = get_eal[3](reg0);
  b = read_L(addr)<<0;
  EORL(s,a,b);
  write_L(addr,(u32)s>>0);
}

static void l0_ADDl3(u32 reg0)
{
  /* ADD.L #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = get_nextl()<<0;
  addr = get_eal[3](reg0);
  b = read_L(addr)<<0;
  ADDL(s,a,b);
  write_L(addr,(u32)s>>0);
}

static void l0_SUBl3(u32 reg0)
{
  /* SUB.L #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = get_nextl()<<0;
  addr = get_eal[3](reg0);
  b = read_L(addr)<<0;
  SUBL(s,a,b);
  write_L(addr,(u32)s>>0);
}

static void l0_CMPl3(u32 reg0)
{
  /* CMP.L #imm,<Ae> */
  s32 a,b;
  u32 addr;
  a = get_nextl()<<0;
  addr = get_eal[3](reg0);
  b = read_L(addr)<<0;
  CMPL(a,b);
}

static void l0_ORb4(u32 reg0)
{
  /* OR.B #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = get_nextw()<<24;
  addr = get_eab[4](reg0);
  b = read_B(addr)<<24;
  ORB(s,a,b);
  write_B(addr,(u32)s>>24);
}

static void l0_ANDb4(u32 reg0)
{
  /* AND.B #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = get_nextw()<<24;
  addr = get_eab[4](reg0);
  b = read_B(addr)<<24;
  ANDB(s,a,b);
  write_B(addr,(u32)s>>24);
}

static void l0_EORb4(u32 reg0)
{
  /* EOR.B #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = get_nextw()<<24;
  addr = get_eab[4](reg0);
  b = read_B(addr)<<24;
  EORB(s,a,b);
  write_B(addr,(u32)s>>24);
}

static void l0_ADDb4(u32 reg0)
{
  /* ADD.B #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = get_nextw()<<24;
  addr = get_eab[4](reg0);
  b = read_B(addr)<<24;
  ADDB(s,a,b);
  write_B(addr,(u32)s>>24);
}

static void l0_SUBb4(u32 reg0)
{
  /* SUB.B #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = get_nextw()<<24;
  addr = get_eab[4](reg0);
  b = read_B(addr)<<24;
  SUBB(s,a,b);
  write_B(addr,(u32)s>>24);
}

static void l0_CMPb4(u32 reg0)
{
  /* CMP.B #imm,<Ae> */
  s32 a,b;
  u32 addr;
  a = get_nextw()<<24;
  addr = get_eab[4](reg0);
  b = read_B(addr)<<24;
  CMPB(a,b);
}

static void l0_ORw4(u32 reg0)
{
  /* OR.W #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = get_nextw()<<16;
  addr = get_eaw[4](reg0);
  b = read_W(addr)<<16;
  ORW(s,a,b);
  write_W(addr,(u32)s>>16);
}

static void l0_ANDw4(u32 reg0)
{
  /* AND.W #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = get_nextw()<<16;
  addr = get_eaw[4](reg0);
  b = read_W(addr)<<16;
  ANDW(s,a,b);
  write_W(addr,(u32)s>>16);
}

static void l0_EORw4(u32 reg0)
{
  /* EOR.W #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = get_nextw()<<16;
  addr = get_eaw[4](reg0);
  b = read_W(addr)<<16;
  EORW(s,a,b);
  write_W(addr,(u32)s>>16);
}

static void l0_ADDw4(u32 reg0)
{
  /* ADD.W #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = get_nextw()<<16;
  addr = get_eaw[4](reg0);
  b = read_W(addr)<<16;
  ADDW(s,a,b);
  write_W(addr,(u32)s>>16);
}

static void l0_SUBw4(u32 reg0)
{
  /* SUB.W #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = get_nextw()<<16;
  addr = get_eaw[4](reg0);
  b = read_W(addr)<<16;
  SUBW(s,a,b);
  write_W(addr,(u32)s>>16);
}

static void l0_CMPw4(u32 reg0)
{
  /* CMP.W #imm,<Ae> */
  s32 a,b;
  u32 addr;
  a = get_nextw()<<16;
  addr = get_eaw[4](reg0);
  b = read_W(addr)<<16;
  CMPW(a,b);
}

static void l0_ORl4(u32 reg0)
{
  /* OR.L #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = get_nextl()<<0;
  addr = get_eal[4](reg0);
  b = read_L(addr)<<0;
  ORL(s,a,b);
  write_L(addr,(u32)s>>0);
}

static void l0_ANDl4(u32 reg0)
{
  /* AND.L #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = get_nextl()<<0;
  addr = get_eal[4](reg0);
  b = read_L(addr)<<0;
  ANDL(s,a,b);
  write_L(addr,(u32)s>>0);
}

static void l0_EORl4(u32 reg0)
{
  /* EOR.L #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = get_nextl()<<0;
  addr = get_eal[4](reg0);
  b = read_L(addr)<<0;
  EORL(s,a,b);
  write_L(addr,(u32)s>>0);
}

static void l0_ADDl4(u32 reg0)
{
  /* ADD.L #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = get_nextl()<<0;
  addr = get_eal[4](reg0);
  b = read_L(addr)<<0;
  ADDL(s,a,b);
  write_L(addr,(u32)s>>0);
}

static void l0_SUBl4(u32 reg0)
{
  /* SUB.L #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = get_nextl()<<0;
  addr = get_eal[4](reg0);
  b = read_L(addr)<<0;
  SUBL(s,a,b);
  write_L(addr,(u32)s>>0);
}

static void l0_CMPl4(u32 reg0)
{
  /* CMP.L #imm,<Ae> */
  s32 a,b;
  u32 addr;
  a = get_nextl()<<0;
  addr = get_eal[4](reg0);
  b = read_L(addr)<<0;
  CMPL(a,b);
}

static void l0_ORb5(u32 reg0)
{
  /* OR.B #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = get_nextw()<<24;
  addr = get_eab[5](reg0);
  b = read_B(addr)<<24;
  ORB(s,a,b);
  write_B(addr,(u32)s>>24);
}

static void l0_ANDb5(u32 reg0)
{
  /* AND.B #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = get_nextw()<<24;
  addr = get_eab[5](reg0);
  b = read_B(addr)<<24;
  ANDB(s,a,b);
  write_B(addr,(u32)s>>24);
}

static void l0_EORb5(u32 reg0)
{
  /* EOR.B #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = get_nextw()<<24;
  addr = get_eab[5](reg0);
  b = read_B(addr)<<24;
  EORB(s,a,b);
  write_B(addr,(u32)s>>24);
}

static void l0_ADDb5(u32 reg0)
{
  /* ADD.B #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = get_nextw()<<24;
  addr = get_eab[5](reg0);
  b = read_B(addr)<<24;
  ADDB(s,a,b);
  write_B(addr,(u32)s>>24);
}

static void l0_SUBb5(u32 reg0)
{
  /* SUB.B #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = get_nextw()<<24;
  addr = get_eab[5](reg0);
  b = read_B(addr)<<24;
  SUBB(s,a,b);
  write_B(addr,(u32)s>>24);
}

static void l0_CMPb5(u32 reg0)
{
  /* CMP.B #imm,<Ae> */
  s32 a,b;
  u32 addr;
  a = get_nextw()<<24;
  addr = get_eab[5](reg0);
  b = read_B(addr)<<24;
  CMPB(a,b);
}

static void l0_ORw5(u32 reg0)
{
  /* OR.W #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = get_nextw()<<16;
  addr = get_eaw[5](reg0);
  b = read_W(addr)<<16;
  ORW(s,a,b);
  write_W(addr,(u32)s>>16);
}

static void l0_ANDw5(u32 reg0)
{
  /* AND.W #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = get_nextw()<<16;
  addr = get_eaw[5](reg0);
  b = read_W(addr)<<16;
  ANDW(s,a,b);
  write_W(addr,(u32)s>>16);
}

static void l0_EORw5(u32 reg0)
{
  /* EOR.W #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = get_nextw()<<16;
  addr = get_eaw[5](reg0);
  b = read_W(addr)<<16;
  EORW(s,a,b);
  write_W(addr,(u32)s>>16);
}

static void l0_ADDw5(u32 reg0)
{
  /* ADD.W #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = get_nextw()<<16;
  addr = get_eaw[5](reg0);
  b = read_W(addr)<<16;
  ADDW(s,a,b);
  write_W(addr,(u32)s>>16);
}

static void l0_SUBw5(u32 reg0)
{
  /* SUB.W #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = get_nextw()<<16;
  addr = get_eaw[5](reg0);
  b = read_W(addr)<<16;
  SUBW(s,a,b);
  write_W(addr,(u32)s>>16);
}

static void l0_CMPw5(u32 reg0)
{
  /* CMP.W #imm,<Ae> */
  s32 a,b;
  u32 addr;
  a = get_nextw()<<16;
  addr = get_eaw[5](reg0);
  b = read_W(addr)<<16;
  CMPW(a,b);
}

static void l0_ORl5(u32 reg0)
{
  /* OR.L #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = get_nextl()<<0;
  addr = get_eal[5](reg0);
  b = read_L(addr)<<0;
  ORL(s,a,b);
  write_L(addr,(u32)s>>0);
}

static void l0_ANDl5(u32 reg0)
{
  /* AND.L #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = get_nextl()<<0;
  addr = get_eal[5](reg0);
  b = read_L(addr)<<0;
  ANDL(s,a,b);
  write_L(addr,(u32)s>>0);
}

static void l0_EORl5(u32 reg0)
{
  /* EOR.L #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = get_nextl()<<0;
  addr = get_eal[5](reg0);
  b = read_L(addr)<<0;
  EORL(s,a,b);
  write_L(addr,(u32)s>>0);
}

static void l0_ADDl5(u32 reg0)
{
  /* ADD.L #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = get_nextl()<<0;
  addr = get_eal[5](reg0);
  b = read_L(addr)<<0;
  ADDL(s,a,b);
  write_L(addr,(u32)s>>0);
}

static void l0_SUBl5(u32 reg0)
{
  /* SUB.L #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = get_nextl()<<0;
  addr = get_eal[5](reg0);
  b = read_L(addr)<<0;
  SUBL(s,a,b);
  write_L(addr,(u32)s>>0);
}

static void l0_CMPl5(u32 reg0)
{
  /* CMP.L #imm,<Ae> */
  s32 a,b;
  u32 addr;
  a = get_nextl()<<0;
  addr = get_eal[5](reg0);
  b = read_L(addr)<<0;
  CMPL(a,b);
}

static void l0_ORb6(u32 reg0)
{
  /* OR.B #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = get_nextw()<<24;
  addr = get_eab[6](reg0);
  b = read_B(addr)<<24;
  ORB(s,a,b);
  write_B(addr,(u32)s>>24);
}

static void l0_ANDb6(u32 reg0)
{
  /* AND.B #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = get_nextw()<<24;
  addr = get_eab[6](reg0);
  b = read_B(addr)<<24;
  ANDB(s,a,b);
  write_B(addr,(u32)s>>24);
}

static void l0_EORb6(u32 reg0)
{
  /* EOR.B #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = get_nextw()<<24;
  addr = get_eab[6](reg0);
  b = read_B(addr)<<24;
  EORB(s,a,b);
  write_B(addr,(u32)s>>24);
}

static void l0_ADDb6(u32 reg0)
{
  /* ADD.B #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = get_nextw()<<24;
  addr = get_eab[6](reg0);
  b = read_B(addr)<<24;
  ADDB(s,a,b);
  write_B(addr,(u32)s>>24);
}

static void l0_SUBb6(u32 reg0)
{
  /* SUB.B #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = get_nextw()<<24;
  addr = get_eab[6](reg0);
  b = read_B(addr)<<24;
  SUBB(s,a,b);
  write_B(addr,(u32)s>>24);
}

static void l0_CMPb6(u32 reg0)
{
  /* CMP.B #imm,<Ae> */
  s32 a,b;
  u32 addr;
  a = get_nextw()<<24;
  addr = get_eab[6](reg0);
  b = read_B(addr)<<24;
  CMPB(a,b);
}

static void l0_ORw6(u32 reg0)
{
  /* OR.W #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = get_nextw()<<16;
  addr = get_eaw[6](reg0);
  b = read_W(addr)<<16;
  ORW(s,a,b);
  write_W(addr,(u32)s>>16);
}

static void l0_ANDw6(u32 reg0)
{
  /* AND.W #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = get_nextw()<<16;
  addr = get_eaw[6](reg0);
  b = read_W(addr)<<16;
  ANDW(s,a,b);
  write_W(addr,(u32)s>>16);
}

static void l0_EORw6(u32 reg0)
{
  /* EOR.W #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = get_nextw()<<16;
  addr = get_eaw[6](reg0);
  b = read_W(addr)<<16;
  EORW(s,a,b);
  write_W(addr,(u32)s>>16);
}

static void l0_ADDw6(u32 reg0)
{
  /* ADD.W #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = get_nextw()<<16;
  addr = get_eaw[6](reg0);
  b = read_W(addr)<<16;
  ADDW(s,a,b);
  write_W(addr,(u32)s>>16);
}

static void l0_SUBw6(u32 reg0)
{
  /* SUB.W #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = get_nextw()<<16;
  addr = get_eaw[6](reg0);
  b = read_W(addr)<<16;
  SUBW(s,a,b);
  write_W(addr,(u32)s>>16);
}

static void l0_CMPw6(u32 reg0)
{
  /* CMP.W #imm,<Ae> */
  s32 a,b;
  u32 addr;
  a = get_nextw()<<16;
  addr = get_eaw[6](reg0);
  b = read_W(addr)<<16;
  CMPW(a,b);
}

static void l0_ORl6(u32 reg0)
{
  /* OR.L #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = get_nextl()<<0;
  addr = get_eal[6](reg0);
  b = read_L(addr)<<0;
  ORL(s,a,b);
  write_L(addr,(u32)s>>0);
}

static void l0_ANDl6(u32 reg0)
{
  /* AND.L #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = get_nextl()<<0;
  addr = get_eal[6](reg0);
  b = read_L(addr)<<0;
  ANDL(s,a,b);
  write_L(addr,(u32)s>>0);
}

static void l0_EORl6(u32 reg0)
{
  /* EOR.L #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = get_nextl()<<0;
  addr = get_eal[6](reg0);
  b = read_L(addr)<<0;
  EORL(s,a,b);
  write_L(addr,(u32)s>>0);
}

static void l0_ADDl6(u32 reg0)
{
  /* ADD.L #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = get_nextl()<<0;
  addr = get_eal[6](reg0);
  b = read_L(addr)<<0;
  ADDL(s,a,b);
  write_L(addr,(u32)s>>0);
}

static void l0_SUBl6(u32 reg0)
{
  /* SUB.L #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = get_nextl()<<0;
  addr = get_eal[6](reg0);
  b = read_L(addr)<<0;
  SUBL(s,a,b);
  write_L(addr,(u32)s>>0);
}

static void l0_CMPl6(u32 reg0)
{
  /* CMP.L #imm,<Ae> */
  s32 a,b;
  u32 addr;
  a = get_nextl()<<0;
  addr = get_eal[6](reg0);
  b = read_L(addr)<<0;
  CMPL(a,b);
}

static void l0_ORb7(u32 reg0)
{
  if(reg0==4)
  {
    u32 a;
    a = get_nextw()&255;
    reg68.sr |= a;
  }
  else
  {
  /* OR.B #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = get_nextw()<<24;
  addr = get_eab[7](reg0);
  b = read_B(addr)<<24;
  ORB(s,a,b);
  write_B(addr,(u32)s>>24);
  }
}

static void l0_ANDb7(u32 reg0)
{
  if(reg0==4)
  {
    u32 a;
    a = get_nextw()|0xFF00;
    reg68.sr &= a;
  }
  else
  {
  /* AND.B #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = get_nextw()<<24;
  addr = get_eab[7](reg0);
  b = read_B(addr)<<24;
  ANDB(s,a,b);
  write_B(addr,(u32)s>>24);
  }
}

static void l0_EORb7(u32 reg0)
{
  if(reg0==4)
  {
    u32 a;
    a = get_nextw()&255;
    reg68.sr ^= a;
  }
  else
  {
  /* EOR.B #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = get_nextw()<<24;
  addr = get_eab[7](reg0);
  b = read_B(addr)<<24;
  EORB(s,a,b);
  write_B(addr,(u32)s>>24);
  }
}

static void l0_ADDb7(u32 reg0)
{
  /* ADD.B #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = get_nextw()<<24;
  addr = get_eab[7](reg0);
  b = read_B(addr)<<24;
  ADDB(s,a,b);
  write_B(addr,(u32)s>>24);
}

static void l0_SUBb7(u32 reg0)
{
  /* SUB.B #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = get_nextw()<<24;
  addr = get_eab[7](reg0);
  b = read_B(addr)<<24;
  SUBB(s,a,b);
  write_B(addr,(u32)s>>24);
}

static void l0_CMPb7(u32 reg0)
{
  /* CMP.B #imm,<Ae> */
  s32 a,b;
  u32 addr;
  a = get_nextw()<<24;
  addr = get_eab[7](reg0);
  b = read_B(addr)<<24;
  CMPB(a,b);
}

static void l0_ORw7(u32 reg0)
{
  if(reg0==4)
  {
    u32 a;
    a = get_nextw();
    reg68.sr |= a;
  }
  else
  {
  /* OR.W #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = get_nextw()<<16;
  addr = get_eaw[7](reg0);
  b = read_W(addr)<<16;
  ORW(s,a,b);
  write_W(addr,(u32)s>>16);
  }
}

static void l0_ANDw7(u32 reg0)
{
  if(reg0==4)
  {
    u32 a;
    a = get_nextw();
    reg68.sr &= a;
  }
  else
  {
  /* AND.W #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = get_nextw()<<16;
  addr = get_eaw[7](reg0);
  b = read_W(addr)<<16;
  ANDW(s,a,b);
  write_W(addr,(u32)s>>16);
  }
}

static void l0_EORw7(u32 reg0)
{
  if(reg0==4)
  {
    u32 a;
    a = get_nextw();
    reg68.sr ^= a;
  }
  else
  {
  /* EOR.W #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = get_nextw()<<16;
  addr = get_eaw[7](reg0);
  b = read_W(addr)<<16;
  EORW(s,a,b);
  write_W(addr,(u32)s>>16);
  }
}

static void l0_ADDw7(u32 reg0)
{
  /* ADD.W #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = get_nextw()<<16;
  addr = get_eaw[7](reg0);
  b = read_W(addr)<<16;
  ADDW(s,a,b);
  write_W(addr,(u32)s>>16);
}

static void l0_SUBw7(u32 reg0)
{
  /* SUB.W #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = get_nextw()<<16;
  addr = get_eaw[7](reg0);
  b = read_W(addr)<<16;
  SUBW(s,a,b);
  write_W(addr,(u32)s>>16);
}

static void l0_CMPw7(u32 reg0)
{
  /* CMP.W #imm,<Ae> */
  s32 a,b;
  u32 addr;
  a = get_nextw()<<16;
  addr = get_eaw[7](reg0);
  b = read_W(addr)<<16;
  CMPW(a,b);
}

static void l0_ORl7(u32 reg0)
{
  /* OR.L #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = get_nextl()<<0;
  addr = get_eal[7](reg0);
  b = read_L(addr)<<0;
  ORL(s,a,b);
  write_L(addr,(u32)s>>0);
}

static void l0_ANDl7(u32 reg0)
{
  /* AND.L #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = get_nextl()<<0;
  addr = get_eal[7](reg0);
  b = read_L(addr)<<0;
  ANDL(s,a,b);
  write_L(addr,(u32)s>>0);
}

static void l0_EORl7(u32 reg0)
{
  /* EOR.L #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = get_nextl()<<0;
  addr = get_eal[7](reg0);
  b = read_L(addr)<<0;
  EORL(s,a,b);
  write_L(addr,(u32)s>>0);
}

static void l0_ADDl7(u32 reg0)
{
  /* ADD.L #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = get_nextl()<<0;
  addr = get_eal[7](reg0);
  b = read_L(addr)<<0;
  ADDL(s,a,b);
  write_L(addr,(u32)s>>0);
}

static void l0_SUBl7(u32 reg0)
{
  /* SUB.L #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = get_nextl()<<0;
  addr = get_eal[7](reg0);
  b = read_L(addr)<<0;
  SUBL(s,a,b);
  write_L(addr,(u32)s>>0);
}

static void l0_CMPl7(u32 reg0)
{
  /* CMP.L #imm,<Ae> */
  s32 a,b;
  u32 addr;
  a = get_nextl()<<0;
  addr = get_eal[7](reg0);
  b = read_L(addr)<<0;
  CMPL(a,b);
}

static void (*line0_imm[8][32])(u32) =
{
/* OR */
  {
  l0_ORb0,l0_ill,l0_ORb2,l0_ORb3,l0_ORb4,l0_ORb5,l0_ORb6,l0_ORb7,
  l0_ORw0,l0_ill,l0_ORw2,l0_ORw3,l0_ORw4,l0_ORw5,l0_ORw6,l0_ORw7,
  l0_ORl0,l0_ill,l0_ORl2,l0_ORl3,l0_ORl4,l0_ORl5,l0_ORl6,l0_ORl7,
  l0_ill,l0_ill,l0_ill,l0_ill,l0_ill,l0_ill,l0_ill,l0_ill,

  },
/* AND */
  {
  l0_ANDb0,l0_ill,l0_ANDb2,l0_ANDb3,l0_ANDb4,l0_ANDb5,l0_ANDb6,l0_ANDb7,
  l0_ANDw0,l0_ill,l0_ANDw2,l0_ANDw3,l0_ANDw4,l0_ANDw5,l0_ANDw6,l0_ANDw7,
  l0_ANDl0,l0_ill,l0_ANDl2,l0_ANDl3,l0_ANDl4,l0_ANDl5,l0_ANDl6,l0_ANDl7,
  l0_ill,l0_ill,l0_ill,l0_ill,l0_ill,l0_ill,l0_ill,l0_ill,

  },
/* SUB */
  {
  l0_SUBb0,l0_ill,l0_SUBb2,l0_SUBb3,l0_SUBb4,l0_SUBb5,l0_SUBb6,l0_SUBb7,
  l0_SUBw0,l0_ill,l0_SUBw2,l0_SUBw3,l0_SUBw4,l0_SUBw5,l0_SUBw6,l0_SUBw7,
  l0_SUBl0,l0_ill,l0_SUBl2,l0_SUBl3,l0_SUBl4,l0_SUBl5,l0_SUBl6,l0_SUBl7,
  l0_ill,l0_ill,l0_ill,l0_ill,l0_ill,l0_ill,l0_ill,l0_ill,

  },
/* ADD */
  {
  l0_ADDb0,l0_ill,l0_ADDb2,l0_ADDb3,l0_ADDb4,l0_ADDb5,l0_ADDb6,l0_ADDb7,
  l0_ADDw0,l0_ill,l0_ADDw2,l0_ADDw3,l0_ADDw4,l0_ADDw5,l0_ADDw6,l0_ADDw7,
  l0_ADDl0,l0_ill,l0_ADDl2,l0_ADDl3,l0_ADDl4,l0_ADDl5,l0_ADDl6,l0_ADDl7,
  l0_ill,l0_ill,l0_ill,l0_ill,l0_ill,l0_ill,l0_ill,l0_ill,

  },
/* ? */
  {
  l0_ill,l0_ill,l0_ill,l0_ill,l0_ill,l0_ill,l0_ill,l0_ill,
  l0_ill,l0_ill,l0_ill,l0_ill,l0_ill,l0_ill,l0_ill,l0_ill,
  l0_ill,l0_ill,l0_ill,l0_ill,l0_ill,l0_ill,l0_ill,l0_ill,
  l0_ill,l0_ill,l0_ill,l0_ill,l0_ill,l0_ill,l0_ill,l0_ill,

  },
/* EOR */
  {
  l0_EORb0,l0_ill,l0_EORb2,l0_EORb3,l0_EORb4,l0_EORb5,l0_EORb6,l0_EORb7,
  l0_EORw0,l0_ill,l0_EORw2,l0_EORw3,l0_EORw4,l0_EORw5,l0_EORw6,l0_EORw7,
  l0_EORl0,l0_ill,l0_EORl2,l0_EORl3,l0_EORl4,l0_EORl5,l0_EORl6,l0_EORl7,
  l0_ill,l0_ill,l0_ill,l0_ill,l0_ill,l0_ill,l0_ill,l0_ill,

  },
/* CMP */
  {
  l0_CMPb0,l0_ill,l0_CMPb2,l0_CMPb3,l0_CMPb4,l0_CMPb5,l0_CMPb6,l0_CMPb7,
  l0_CMPw0,l0_ill,l0_CMPw2,l0_CMPw3,l0_CMPw4,l0_CMPw5,l0_CMPw6,l0_CMPw7,
  l0_CMPl0,l0_ill,l0_CMPl2,l0_CMPl3,l0_CMPl4,l0_CMPl5,l0_CMPl6,l0_CMPl7,
  l0_ill,l0_ill,l0_ill,l0_ill,l0_ill,l0_ill,l0_ill,l0_ill,

  },
/* ? */
  {
  l0_ill,l0_ill,l0_ill,l0_ill,l0_ill,l0_ill,l0_ill,l0_ill,
  l0_ill,l0_ill,l0_ill,l0_ill,l0_ill,l0_ill,l0_ill,l0_ill,
  l0_ill,l0_ill,l0_ill,l0_ill,l0_ill,l0_ill,l0_ill,l0_ill,
  l0_ill,l0_ill,l0_ill,l0_ill,l0_ill,l0_ill,l0_ill,l0_ill,

  },
};

void line000(u32 reg9, u32 reg0)
{
  if(reg9==4) BTST_reg(get_nextw(),reg0);
  else (line0_imm[reg9][0])(reg0);
}

void line001(u32 reg9, u32 reg0)
{
  if(reg9==4) BTST_mem(get_nextw(),1,reg0);
  else (line0_imm[reg9][1])(reg0);
}

void line002(u32 reg9, u32 reg0)
{
  if(reg9==4) BTST_mem(get_nextw(),2,reg0);
  else (line0_imm[reg9][2])(reg0);
}

void line003(u32 reg9, u32 reg0)
{
  if(reg9==4) BTST_mem(get_nextw(),3,reg0);
  else (line0_imm[reg9][3])(reg0);
}

void line004(u32 reg9, u32 reg0)
{
  if(reg9==4) BTST_mem(get_nextw(),4,reg0);
  else (line0_imm[reg9][4])(reg0);
}

void line005(u32 reg9, u32 reg0)
{
  if(reg9==4) BTST_mem(get_nextw(),5,reg0);
  else (line0_imm[reg9][5])(reg0);
}

void line006(u32 reg9, u32 reg0)
{
  if(reg9==4) BTST_mem(get_nextw(),6,reg0);
  else (line0_imm[reg9][6])(reg0);
}

void line007(u32 reg9, u32 reg0)
{
  if(reg9==4) BTST_mem(get_nextw(),7,reg0);
  else (line0_imm[reg9][7])(reg0);
}

void line008(u32 reg9, u32 reg0)
{
  if(reg9==4) BCHG_reg(get_nextw(),reg0);
  else (line0_imm[reg9][8])(reg0);
}

void line009(u32 reg9, u32 reg0)
{
  if(reg9==4) BCHG_mem(get_nextw(),1,reg0);
  else (line0_imm[reg9][9])(reg0);
}

void line00A(u32 reg9, u32 reg0)
{
  if(reg9==4) BCHG_mem(get_nextw(),2,reg0);
  else (line0_imm[reg9][10])(reg0);
}

void line00B(u32 reg9, u32 reg0)
{
  if(reg9==4) BCHG_mem(get_nextw(),3,reg0);
  else (line0_imm[reg9][11])(reg0);
}

void line00C(u32 reg9, u32 reg0)
{
  if(reg9==4) BCHG_mem(get_nextw(),4,reg0);
  else (line0_imm[reg9][12])(reg0);
}

void line00D(u32 reg9, u32 reg0)
{
  if(reg9==4) BCHG_mem(get_nextw(),5,reg0);
  else (line0_imm[reg9][13])(reg0);
}

void line00E(u32 reg9, u32 reg0)
{
  if(reg9==4) BCHG_mem(get_nextw(),6,reg0);
  else (line0_imm[reg9][14])(reg0);
}

void line00F(u32 reg9, u32 reg0)
{
  if(reg9==4) BCHG_mem(get_nextw(),7,reg0);
  else (line0_imm[reg9][15])(reg0);
}

void line010(u32 reg9, u32 reg0)
{
  if(reg9==4) BCLR_reg(get_nextw(),reg0);
  else (line0_imm[reg9][16])(reg0);
}

void line011(u32 reg9, u32 reg0)
{
  if(reg9==4) BCLR_mem(get_nextw(),1,reg0);
  else (line0_imm[reg9][17])(reg0);
}

void line012(u32 reg9, u32 reg0)
{
  if(reg9==4) BCLR_mem(get_nextw(),2,reg0);
  else (line0_imm[reg9][18])(reg0);
}

void line013(u32 reg9, u32 reg0)
{
  if(reg9==4) BCLR_mem(get_nextw(),3,reg0);
  else (line0_imm[reg9][19])(reg0);
}

void line014(u32 reg9, u32 reg0)
{
  if(reg9==4) BCLR_mem(get_nextw(),4,reg0);
  else (line0_imm[reg9][20])(reg0);
}

void line015(u32 reg9, u32 reg0)
{
  if(reg9==4) BCLR_mem(get_nextw(),5,reg0);
  else (line0_imm[reg9][21])(reg0);
}

void line016(u32 reg9, u32 reg0)
{
  if(reg9==4) BCLR_mem(get_nextw(),6,reg0);
  else (line0_imm[reg9][22])(reg0);
}

void line017(u32 reg9, u32 reg0)
{
  if(reg9==4) BCLR_mem(get_nextw(),7,reg0);
  else (line0_imm[reg9][23])(reg0);
}

void line018(u32 reg9, u32 reg0)
{
  if(reg9==4) BSET_reg(get_nextw(),reg0);
  else (line0_imm[reg9][24])(reg0);
}

void line019(u32 reg9, u32 reg0)
{
  if(reg9==4) BSET_mem(get_nextw(),1,reg0);
  else (line0_imm[reg9][25])(reg0);
}

void line01A(u32 reg9, u32 reg0)
{
  if(reg9==4) BSET_mem(get_nextw(),2,reg0);
  else (line0_imm[reg9][26])(reg0);
}

void line01B(u32 reg9, u32 reg0)
{
  if(reg9==4) BSET_mem(get_nextw(),3,reg0);
  else (line0_imm[reg9][27])(reg0);
}

void line01C(u32 reg9, u32 reg0)
{
  if(reg9==4) BSET_mem(get_nextw(),4,reg0);
  else (line0_imm[reg9][28])(reg0);
}

void line01D(u32 reg9, u32 reg0)
{
  if(reg9==4) BSET_mem(get_nextw(),5,reg0);
  else (line0_imm[reg9][29])(reg0);
}

void line01E(u32 reg9, u32 reg0)
{
  if(reg9==4) BSET_mem(get_nextw(),6,reg0);
  else (line0_imm[reg9][30])(reg0);
}

void line01F(u32 reg9, u32 reg0)
{
  if(reg9==4) BSET_mem(get_nextw(),7,reg0);
  else (line0_imm[reg9][31])(reg0);
}

void line020(u32 reg9, u32 reg0)
{
  /* BTST Dx,Dy */
  u32 a,b;
  a = reg68.d[reg0];
  b = reg68.d[reg9]&31;
  BTST(a,b);
  ADDCYCLE(2);
}

void line021(u32 reg9, u32 reg0)
{
  /* MOVEP.W d(An),Dn */
  u32 addr = reg68.a[reg0] + get_nextw();
  u32 a;
  a  =(u8)read_B(addr)<<8;
  a +=(u8)read_B(addr+=2)<<0;
  reg68.d[reg9] = (reg68.d[reg9]&0xFFFF0000) | a;
  ADDCYCLE(8);
}

void line022(u32 reg9, u32 reg0)
{
  /* BTST Dn,<Ae> */
  u32 a,b;
  u32 addr;
  addr = get_eab[2](reg0);
  a = read_B(addr);
  b = reg68.d[reg9]&7;
  BTST(a,b);
  ADDCYCLE(0);
}

void line023(u32 reg9, u32 reg0)
{
  /* BTST Dn,<Ae> */
  u32 a,b;
  u32 addr;
  addr = get_eab[3](reg0);
  a = read_B(addr);
  b = reg68.d[reg9]&7;
  BTST(a,b);
  ADDCYCLE(0);
}

void line024(u32 reg9, u32 reg0)
{
  /* BTST Dn,<Ae> */
  u32 a,b;
  u32 addr;
  addr = get_eab[4](reg0);
  a = read_B(addr);
  b = reg68.d[reg9]&7;
  BTST(a,b);
  ADDCYCLE(0);
}

void line025(u32 reg9, u32 reg0)
{
  /* BTST Dn,<Ae> */
  u32 a,b;
  u32 addr;
  addr = get_eab[5](reg0);
  a = read_B(addr);
  b = reg68.d[reg9]&7;
  BTST(a,b);
  ADDCYCLE(0);
}

void line026(u32 reg9, u32 reg0)
{
  /* BTST Dn,<Ae> */
  u32 a,b;
  u32 addr;
  addr = get_eab[6](reg0);
  a = read_B(addr);
  b = reg68.d[reg9]&7;
  BTST(a,b);
  ADDCYCLE(0);
}

void line027(u32 reg9, u32 reg0)
{
  /* BTST Dn,<Ae> */
  u32 a,b;
  u32 addr;
  addr = get_eab[7](reg0);
  a = read_B(addr);
  b = reg68.d[reg9]&7;
  BTST(a,b);
  ADDCYCLE(0);
}

void line028(u32 reg9, u32 reg0)
{
  /* BCHG Dx,Dy */
  u32 a,b;
  a = reg68.d[reg0];
  b = reg68.d[reg9]&31;
  BCHG(a,b);
  reg68.d[reg0] = a;
  ADDCYCLE(4);
}

void line029(u32 reg9, u32 reg0)
{
  /* MOVEP.L d(An),Dn */
  u32 addr = reg68.a[reg0] + get_nextw();
  u32 a;
  a  =(u8)read_B(addr)<<24;
  a +=(u8)read_B(addr+=2)<<16;
  a +=(u8)read_B(addr+=2)<<8;
  a +=(u8)read_B(addr+=2)<<0;
  reg68.d[reg9] = a;
  ADDCYCLE(16);
}

void line02A(u32 reg9, u32 reg0)
{
  /* BCHG Dn,<Ae> */
  u32 a,b;
  u32 addr;
  addr = get_eab[2](reg0);
  a = read_B(addr);
  b = reg68.d[reg9]&7;
  BCHG(a,b);
  write_B(addr,a);
  ADDCYCLE(4);
}

void line02B(u32 reg9, u32 reg0)
{
  /* BCHG Dn,<Ae> */
  u32 a,b;
  u32 addr;
  addr = get_eab[3](reg0);
  a = read_B(addr);
  b = reg68.d[reg9]&7;
  BCHG(a,b);
  write_B(addr,a);
  ADDCYCLE(4);
}

void line02C(u32 reg9, u32 reg0)
{
  /* BCHG Dn,<Ae> */
  u32 a,b;
  u32 addr;
  addr = get_eab[4](reg0);
  a = read_B(addr);
  b = reg68.d[reg9]&7;
  BCHG(a,b);
  write_B(addr,a);
  ADDCYCLE(4);
}

void line02D(u32 reg9, u32 reg0)
{
  /* BCHG Dn,<Ae> */
  u32 a,b;
  u32 addr;
  addr = get_eab[5](reg0);
  a = read_B(addr);
  b = reg68.d[reg9]&7;
  BCHG(a,b);
  write_B(addr,a);
  ADDCYCLE(4);
}

void line02E(u32 reg9, u32 reg0)
{
  /* BCHG Dn,<Ae> */
  u32 a,b;
  u32 addr;
  addr = get_eab[6](reg0);
  a = read_B(addr);
  b = reg68.d[reg9]&7;
  BCHG(a,b);
  write_B(addr,a);
  ADDCYCLE(4);
}

void line02F(u32 reg9, u32 reg0)
{
  /* BCHG Dn,<Ae> */
  u32 a,b;
  u32 addr;
  addr = get_eab[7](reg0);
  a = read_B(addr);
  b = reg68.d[reg9]&7;
  BCHG(a,b);
  write_B(addr,a);
  ADDCYCLE(4);
}

void line030(u32 reg9, u32 reg0)
{
  /* BCLR Dx,Dy */
  u32 a,b;
  a = reg68.d[reg0];
  b = reg68.d[reg9]&31;
  BCLR(a,b);
  reg68.d[reg0] = a;
  ADDCYCLE(6);
}

void line031(u32 reg9, u32 reg0)
{
  /* MOVEP.W Dn,d(An) */
  u32 addr = reg68.a[reg0] + get_nextw();
  u32 a;
  a = reg68.d[reg9];
  write_B(addr,a>>8);
  write_B(addr+=2,a>>0);
  ADDCYCLE(8);
}

void line032(u32 reg9, u32 reg0)
{
  /* BCLR Dn,<Ae> */
  u32 a,b;
  u32 addr;
  addr = get_eab[2](reg0);
  a = read_B(addr);
  b = reg68.d[reg9]&7;
  BCLR(a,b);
  write_B(addr,a);
  ADDCYCLE(4);
}

void line033(u32 reg9, u32 reg0)
{
  /* BCLR Dn,<Ae> */
  u32 a,b;
  u32 addr;
  addr = get_eab[3](reg0);
  a = read_B(addr);
  b = reg68.d[reg9]&7;
  BCLR(a,b);
  write_B(addr,a);
  ADDCYCLE(4);
}

void line034(u32 reg9, u32 reg0)
{
  /* BCLR Dn,<Ae> */
  u32 a,b;
  u32 addr;
  addr = get_eab[4](reg0);
  a = read_B(addr);
  b = reg68.d[reg9]&7;
  BCLR(a,b);
  write_B(addr,a);
  ADDCYCLE(4);
}

void line035(u32 reg9, u32 reg0)
{
  /* BCLR Dn,<Ae> */
  u32 a,b;
  u32 addr;
  addr = get_eab[5](reg0);
  a = read_B(addr);
  b = reg68.d[reg9]&7;
  BCLR(a,b);
  write_B(addr,a);
  ADDCYCLE(4);
}

void line036(u32 reg9, u32 reg0)
{
  /* BCLR Dn,<Ae> */
  u32 a,b;
  u32 addr;
  addr = get_eab[6](reg0);
  a = read_B(addr);
  b = reg68.d[reg9]&7;
  BCLR(a,b);
  write_B(addr,a);
  ADDCYCLE(4);
}

void line037(u32 reg9, u32 reg0)
{
  /* BCLR Dn,<Ae> */
  u32 a,b;
  u32 addr;
  addr = get_eab[7](reg0);
  a = read_B(addr);
  b = reg68.d[reg9]&7;
  BCLR(a,b);
  write_B(addr,a);
  ADDCYCLE(4);
}

void line038(u32 reg9, u32 reg0)
{
  /* BSET Dx,Dy */
  u32 a,b;
  a = reg68.d[reg0];
  b = reg68.d[reg9]&31;
  BSET(a,b);
  reg68.d[reg0] = a;
  ADDCYCLE(4);
}

void line039(u32 reg9, u32 reg0)
{
  /* MOVEP.L Dn,d(An) */
  u32 addr = reg68.a[reg0] + get_nextw();
  u32 a;
  a = reg68.d[reg9];
  write_B(addr,a>>24);
  write_B(addr+=2,a>>16);
  write_B(addr+=2,a>>8);
  write_B(addr+=2,a>>0);
  ADDCYCLE(16);
}

void line03A(u32 reg9, u32 reg0)
{
  /* BSET Dn,<Ae> */
  u32 a,b;
  u32 addr;
  addr = get_eab[2](reg0);
  a = read_B(addr);
  b = reg68.d[reg9]&7;
  BSET(a,b);
  write_B(addr,a);
  ADDCYCLE(4);
}

void line03B(u32 reg9, u32 reg0)
{
  /* BSET Dn,<Ae> */
  u32 a,b;
  u32 addr;
  addr = get_eab[3](reg0);
  a = read_B(addr);
  b = reg68.d[reg9]&7;
  BSET(a,b);
  write_B(addr,a);
  ADDCYCLE(4);
}

void line03C(u32 reg9, u32 reg0)
{
  /* BSET Dn,<Ae> */
  u32 a,b;
  u32 addr;
  addr = get_eab[4](reg0);
  a = read_B(addr);
  b = reg68.d[reg9]&7;
  BSET(a,b);
  write_B(addr,a);
  ADDCYCLE(4);
}

void line03D(u32 reg9, u32 reg0)
{
  /* BSET Dn,<Ae> */
  u32 a,b;
  u32 addr;
  addr = get_eab[5](reg0);
  a = read_B(addr);
  b = reg68.d[reg9]&7;
  BSET(a,b);
  write_B(addr,a);
  ADDCYCLE(4);
}

void line03E(u32 reg9, u32 reg0)
{
  /* BSET Dn,<Ae> */
  u32 a,b;
  u32 addr;
  addr = get_eab[6](reg0);
  a = read_B(addr);
  b = reg68.d[reg9]&7;
  BSET(a,b);
  write_B(addr,a);
  ADDCYCLE(4);
}

void line03F(u32 reg9, u32 reg0)
{
  /* BSET Dn,<Ae> */
  u32 a,b;
  u32 addr;
  addr = get_eab[7](reg0);
  a = read_B(addr);
  b = reg68.d[reg9]&7;
  BSET(a,b);
  write_B(addr,a);
  ADDCYCLE(4);
}

