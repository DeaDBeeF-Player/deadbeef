void lineC00(u32 reg9, u32 reg0)
{
  /* AND.B Dx,Dy */
  s32 a,b,s;
  a = reg68.d[reg0]<<24;
  b = reg68.d[reg9]<<24;
  ANDB(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0xffffff00) + ((u32)s>>24);
}

void lineC01(u32 reg9, u32 reg0)
{
  /*ILLEGAL in EXG*/
  ILLEGAL
}

void lineC02(u32 reg9, u32 reg0)
{
  /* AND.B <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eab[2](reg0);
  a = read_B(addr)<<24;
  b = reg68.d[reg9]<<24;
  ANDB(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0xffffff00) + ((u32)s>>24);
}

void lineC03(u32 reg9, u32 reg0)
{
  /* AND.B <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eab[3](reg0);
  a = read_B(addr)<<24;
  b = reg68.d[reg9]<<24;
  ANDB(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0xffffff00) + ((u32)s>>24);
}

void lineC04(u32 reg9, u32 reg0)
{
  /* AND.B <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eab[4](reg0);
  a = read_B(addr)<<24;
  b = reg68.d[reg9]<<24;
  ANDB(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0xffffff00) + ((u32)s>>24);
}

void lineC05(u32 reg9, u32 reg0)
{
  /* AND.B <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eab[5](reg0);
  a = read_B(addr)<<24;
  b = reg68.d[reg9]<<24;
  ANDB(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0xffffff00) + ((u32)s>>24);
}

void lineC06(u32 reg9, u32 reg0)
{
  /* AND.B <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eab[6](reg0);
  a = read_B(addr)<<24;
  b = reg68.d[reg9]<<24;
  ANDB(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0xffffff00) + ((u32)s>>24);
}

void lineC07(u32 reg9, u32 reg0)
{
  /* AND.B <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eab[7](reg0);
  a = read_B(addr)<<24;
  b = reg68.d[reg9]<<24;
  ANDB(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0xffffff00) + ((u32)s>>24);
}

void lineC08(u32 reg9, u32 reg0)
{
  /* AND.W Dx,Dy */
  s32 a,b,s;
  a = reg68.d[reg0]<<16;
  b = reg68.d[reg9]<<16;
  ANDW(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0xffff0000) + ((u32)s>>16);
}

void lineC09(u32 reg9, u32 reg0)
{
  /* EXG Ax,Ay */
  EXG(reg68.a[reg9],reg68.a[reg0]);
}

void lineC0A(u32 reg9, u32 reg0)
{
  /* AND.W <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eaw[2](reg0);
  a = read_W(addr)<<16;
  b = reg68.d[reg9]<<16;
  ANDW(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0xffff0000) + ((u32)s>>16);
}

void lineC0B(u32 reg9, u32 reg0)
{
  /* AND.W <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eaw[3](reg0);
  a = read_W(addr)<<16;
  b = reg68.d[reg9]<<16;
  ANDW(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0xffff0000) + ((u32)s>>16);
}

void lineC0C(u32 reg9, u32 reg0)
{
  /* AND.W <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eaw[4](reg0);
  a = read_W(addr)<<16;
  b = reg68.d[reg9]<<16;
  ANDW(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0xffff0000) + ((u32)s>>16);
}

void lineC0D(u32 reg9, u32 reg0)
{
  /* AND.W <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eaw[5](reg0);
  a = read_W(addr)<<16;
  b = reg68.d[reg9]<<16;
  ANDW(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0xffff0000) + ((u32)s>>16);
}

void lineC0E(u32 reg9, u32 reg0)
{
  /* AND.W <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eaw[6](reg0);
  a = read_W(addr)<<16;
  b = reg68.d[reg9]<<16;
  ANDW(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0xffff0000) + ((u32)s>>16);
}

void lineC0F(u32 reg9, u32 reg0)
{
  /* AND.W <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eaw[7](reg0);
  a = read_W(addr)<<16;
  b = reg68.d[reg9]<<16;
  ANDW(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0xffff0000) + ((u32)s>>16);
}

void lineC10(u32 reg9, u32 reg0)
{
  /* AND.L Dx,Dy */
  s32 a,b,s;
  a = reg68.d[reg0]<<0;
  b = reg68.d[reg9]<<0;
  ANDL(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0) + ((u32)s>>0);
}

void lineC11(u32 reg9, u32 reg0)
{
  /* EXG Dx,Ay */
  EXG(reg68.d[reg9],reg68.a[reg0]);
}

void lineC12(u32 reg9, u32 reg0)
{
  /* AND.L <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eal[2](reg0);
  a = read_L(addr)<<0;
  b = reg68.d[reg9]<<0;
  ANDL(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0) + ((u32)s>>0);
}

void lineC13(u32 reg9, u32 reg0)
{
  /* AND.L <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eal[3](reg0);
  a = read_L(addr)<<0;
  b = reg68.d[reg9]<<0;
  ANDL(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0) + ((u32)s>>0);
}

void lineC14(u32 reg9, u32 reg0)
{
  /* AND.L <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eal[4](reg0);
  a = read_L(addr)<<0;
  b = reg68.d[reg9]<<0;
  ANDL(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0) + ((u32)s>>0);
}

void lineC15(u32 reg9, u32 reg0)
{
  /* AND.L <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eal[5](reg0);
  a = read_L(addr)<<0;
  b = reg68.d[reg9]<<0;
  ANDL(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0) + ((u32)s>>0);
}

void lineC16(u32 reg9, u32 reg0)
{
  /* AND.L <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eal[6](reg0);
  a = read_L(addr)<<0;
  b = reg68.d[reg9]<<0;
  ANDL(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0) + ((u32)s>>0);
}

void lineC17(u32 reg9, u32 reg0)
{
  /* AND.L <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eal[7](reg0);
  a = read_L(addr)<<0;
  b = reg68.d[reg9]<<0;
  ANDL(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0) + ((u32)s>>0);
}

void lineC18(u32 reg9, u32 reg0)
{
  /* MULU.W Dx,Dy */
  s32 a,b,s;
  a = reg68.d[reg0]<<16;
  b = reg68.d[reg9]<<16;
  MULUW(s,a,b);
  reg68.d[reg9] = s;
}

void lineC19(u32 reg9, u32 reg0)
{
  /* MULU.W Ax,Dy */
  s32 a,b,s;
  a = reg68.a[reg0]<<16;
  b = reg68.d[reg9]<<16;
  MULUW(s,a,b);
  reg68.d[reg9] = s;
}

void lineC1A(u32 reg9, u32 reg0)
{
  /* MULU.W <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eaw[2](reg0);
  a = read_W(addr)<<16;
  b = reg68.d[reg9]<<16;
  MULUW(s,a,b);
  reg68.d[reg9] = s;
}

void lineC1B(u32 reg9, u32 reg0)
{
  /* MULU.W <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eaw[3](reg0);
  a = read_W(addr)<<16;
  b = reg68.d[reg9]<<16;
  MULUW(s,a,b);
  reg68.d[reg9] = s;
}

void lineC1C(u32 reg9, u32 reg0)
{
  /* MULU.W <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eaw[4](reg0);
  a = read_W(addr)<<16;
  b = reg68.d[reg9]<<16;
  MULUW(s,a,b);
  reg68.d[reg9] = s;
}

void lineC1D(u32 reg9, u32 reg0)
{
  /* MULU.W <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eaw[5](reg0);
  a = read_W(addr)<<16;
  b = reg68.d[reg9]<<16;
  MULUW(s,a,b);
  reg68.d[reg9] = s;
}

void lineC1E(u32 reg9, u32 reg0)
{
  /* MULU.W <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eaw[6](reg0);
  a = read_W(addr)<<16;
  b = reg68.d[reg9]<<16;
  MULUW(s,a,b);
  reg68.d[reg9] = s;
}

void lineC1F(u32 reg9, u32 reg0)
{
  /* MULU.W <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eaw[7](reg0);
  a = read_W(addr)<<16;
  b = reg68.d[reg9]<<16;
  MULUW(s,a,b);
  reg68.d[reg9] = s;
}

void lineC20(u32 reg9, u32 reg0)
{
  /* ABCD Dy,Dx */
  reg0=reg0; reg9=reg9;
}

void lineC21(u32 reg9, u32 reg0)
{
  /* ABCD -(Ay),-(Ax) */
  reg68.a[reg0] -= 2;
  reg68.a[reg9] -= 2;
}

void lineC22(u32 reg9, u32 reg0)
{
  /* AND.B Dn,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = reg68.d[reg9]<<24;
  addr = get_eab[2](reg0);
  b = read_B(addr)<<24;
  ANDB(s,a,b);
  write_B(addr,(u32)s>>24);
}

void lineC23(u32 reg9, u32 reg0)
{
  /* AND.B Dn,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = reg68.d[reg9]<<24;
  addr = get_eab[3](reg0);
  b = read_B(addr)<<24;
  ANDB(s,a,b);
  write_B(addr,(u32)s>>24);
}

void lineC24(u32 reg9, u32 reg0)
{
  /* AND.B Dn,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = reg68.d[reg9]<<24;
  addr = get_eab[4](reg0);
  b = read_B(addr)<<24;
  ANDB(s,a,b);
  write_B(addr,(u32)s>>24);
}

void lineC25(u32 reg9, u32 reg0)
{
  /* AND.B Dn,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = reg68.d[reg9]<<24;
  addr = get_eab[5](reg0);
  b = read_B(addr)<<24;
  ANDB(s,a,b);
  write_B(addr,(u32)s>>24);
}

void lineC26(u32 reg9, u32 reg0)
{
  /* AND.B Dn,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = reg68.d[reg9]<<24;
  addr = get_eab[6](reg0);
  b = read_B(addr)<<24;
  ANDB(s,a,b);
  write_B(addr,(u32)s>>24);
}

void lineC27(u32 reg9, u32 reg0)
{
  /* AND.B Dn,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = reg68.d[reg9]<<24;
  addr = get_eab[7](reg0);
  b = read_B(addr)<<24;
  ANDB(s,a,b);
  write_B(addr,(u32)s>>24);
}

void lineC28(u32 reg9, u32 reg0)
{
  /* EXG Dx,Dy */
  EXG(reg68.d[reg9],reg68.d[reg0]);
}

void lineC29(u32 reg9, u32 reg0)
{
  /* EXG Ax,Ay */
  EXG(reg68.a[reg9],reg68.a[reg0]);
}

void lineC2A(u32 reg9, u32 reg0)
{
  /* AND.W Dn,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = reg68.d[reg9]<<16;
  addr = get_eaw[2](reg0);
  b = read_W(addr)<<16;
  ANDW(s,a,b);
  write_W(addr,(u32)s>>16);
}

void lineC2B(u32 reg9, u32 reg0)
{
  /* AND.W Dn,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = reg68.d[reg9]<<16;
  addr = get_eaw[3](reg0);
  b = read_W(addr)<<16;
  ANDW(s,a,b);
  write_W(addr,(u32)s>>16);
}

void lineC2C(u32 reg9, u32 reg0)
{
  /* AND.W Dn,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = reg68.d[reg9]<<16;
  addr = get_eaw[4](reg0);
  b = read_W(addr)<<16;
  ANDW(s,a,b);
  write_W(addr,(u32)s>>16);
}

void lineC2D(u32 reg9, u32 reg0)
{
  /* AND.W Dn,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = reg68.d[reg9]<<16;
  addr = get_eaw[5](reg0);
  b = read_W(addr)<<16;
  ANDW(s,a,b);
  write_W(addr,(u32)s>>16);
}

void lineC2E(u32 reg9, u32 reg0)
{
  /* AND.W Dn,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = reg68.d[reg9]<<16;
  addr = get_eaw[6](reg0);
  b = read_W(addr)<<16;
  ANDW(s,a,b);
  write_W(addr,(u32)s>>16);
}

void lineC2F(u32 reg9, u32 reg0)
{
  /* AND.W Dn,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = reg68.d[reg9]<<16;
  addr = get_eaw[7](reg0);
  b = read_W(addr)<<16;
  ANDW(s,a,b);
  write_W(addr,(u32)s>>16);
}

void lineC30(u32 reg9, u32 reg0)
{
  /* AND.L Dx,Dy */
  s32 a,b,s;
  a = reg68.d[reg9]<<0;
  b = reg68.d[reg0]<<0;
  ANDL(s,a,b);
  reg68.d[reg0] = (reg68.d[reg0]&0) + ((u32)s>>0);
}

void lineC31(u32 reg9, u32 reg0)
{
  /* EXG Dx,Ay */
  EXG(reg68.d[reg9],reg68.a[reg0]);
}

void lineC32(u32 reg9, u32 reg0)
{
  /* AND.L Dn,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = reg68.d[reg9]<<0;
  addr = get_eal[2](reg0);
  b = read_L(addr)<<0;
  ANDL(s,a,b);
  write_L(addr,(u32)s>>0);
}

void lineC33(u32 reg9, u32 reg0)
{
  /* AND.L Dn,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = reg68.d[reg9]<<0;
  addr = get_eal[3](reg0);
  b = read_L(addr)<<0;
  ANDL(s,a,b);
  write_L(addr,(u32)s>>0);
}

void lineC34(u32 reg9, u32 reg0)
{
  /* AND.L Dn,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = reg68.d[reg9]<<0;
  addr = get_eal[4](reg0);
  b = read_L(addr)<<0;
  ANDL(s,a,b);
  write_L(addr,(u32)s>>0);
}

void lineC35(u32 reg9, u32 reg0)
{
  /* AND.L Dn,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = reg68.d[reg9]<<0;
  addr = get_eal[5](reg0);
  b = read_L(addr)<<0;
  ANDL(s,a,b);
  write_L(addr,(u32)s>>0);
}

void lineC36(u32 reg9, u32 reg0)
{
  /* AND.L Dn,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = reg68.d[reg9]<<0;
  addr = get_eal[6](reg0);
  b = read_L(addr)<<0;
  ANDL(s,a,b);
  write_L(addr,(u32)s>>0);
}

void lineC37(u32 reg9, u32 reg0)
{
  /* AND.L Dn,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = reg68.d[reg9]<<0;
  addr = get_eal[7](reg0);
  b = read_L(addr)<<0;
  ANDL(s,a,b);
  write_L(addr,(u32)s>>0);
}

void lineC38(u32 reg9, u32 reg0)
{
  /* MULS.W Dx,Dy */
  s32 a,b,s;
  a = reg68.d[reg0]<<16;
  b = reg68.d[reg9]<<16;
  MULSW(s,a,b);
  reg68.d[reg9] = s;
}

void lineC39(u32 reg9, u32 reg0)
{
  /* MULS.W Ax,Dy */
  s32 a,b,s;
  a = reg68.a[reg0]<<16;
  b = reg68.d[reg9]<<16;
  MULSW(s,a,b);
  reg68.d[reg9] = s;
}

void lineC3A(u32 reg9, u32 reg0)
{
  /* MULS.W <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eaw[2](reg0);
  a = read_W(addr)<<16;
  b = reg68.d[reg9]<<16;
  MULSW(s,a,b);
  reg68.d[reg9] = s;
}

void lineC3B(u32 reg9, u32 reg0)
{
  /* MULS.W <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eaw[3](reg0);
  a = read_W(addr)<<16;
  b = reg68.d[reg9]<<16;
  MULSW(s,a,b);
  reg68.d[reg9] = s;
}

void lineC3C(u32 reg9, u32 reg0)
{
  /* MULS.W <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eaw[4](reg0);
  a = read_W(addr)<<16;
  b = reg68.d[reg9]<<16;
  MULSW(s,a,b);
  reg68.d[reg9] = s;
}

void lineC3D(u32 reg9, u32 reg0)
{
  /* MULS.W <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eaw[5](reg0);
  a = read_W(addr)<<16;
  b = reg68.d[reg9]<<16;
  MULSW(s,a,b);
  reg68.d[reg9] = s;
}

void lineC3E(u32 reg9, u32 reg0)
{
  /* MULS.W <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eaw[6](reg0);
  a = read_W(addr)<<16;
  b = reg68.d[reg9]<<16;
  MULSW(s,a,b);
  reg68.d[reg9] = s;
}

void lineC3F(u32 reg9, u32 reg0)
{
  /* MULS.W <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eaw[7](reg0);
  a = read_W(addr)<<16;
  b = reg68.d[reg9]<<16;
  MULSW(s,a,b);
  reg68.d[reg9] = s;
}

