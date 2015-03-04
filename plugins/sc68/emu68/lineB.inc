void lineB00(u32 reg9, u32 reg0)
{
  /* CMP.B Dx,Dy */
  s32 a,b;
  a = reg68.d[reg0]<<24;
  b = reg68.d[reg9]<<24;
  CMPB(a,b);
}

void lineB01(u32 reg9, u32 reg0)
{
  /* CMP.B Ax,Dy */
  s32 a,b;
  a = reg68.a[reg0]<<24;
  b = reg68.d[reg9]<<24;
  CMPB(a,b);
}

void lineB02(u32 reg9, u32 reg0)
{
  /* CMP.B <Ae>,Dn */
  s32 a,b;
  u32 addr;
  addr = get_eab[2](reg0);
  a = read_B(addr)<<24;
  b = reg68.d[reg9]<<24;
  CMPB(a,b);
}

void lineB03(u32 reg9, u32 reg0)
{
  /* CMP.B <Ae>,Dn */
  s32 a,b;
  u32 addr;
  addr = get_eab[3](reg0);
  a = read_B(addr)<<24;
  b = reg68.d[reg9]<<24;
  CMPB(a,b);
}

void lineB04(u32 reg9, u32 reg0)
{
  /* CMP.B <Ae>,Dn */
  s32 a,b;
  u32 addr;
  addr = get_eab[4](reg0);
  a = read_B(addr)<<24;
  b = reg68.d[reg9]<<24;
  CMPB(a,b);
}

void lineB05(u32 reg9, u32 reg0)
{
  /* CMP.B <Ae>,Dn */
  s32 a,b;
  u32 addr;
  addr = get_eab[5](reg0);
  a = read_B(addr)<<24;
  b = reg68.d[reg9]<<24;
  CMPB(a,b);
}

void lineB06(u32 reg9, u32 reg0)
{
  /* CMP.B <Ae>,Dn */
  s32 a,b;
  u32 addr;
  addr = get_eab[6](reg0);
  a = read_B(addr)<<24;
  b = reg68.d[reg9]<<24;
  CMPB(a,b);
}

void lineB07(u32 reg9, u32 reg0)
{
  /* CMP.B <Ae>,Dn */
  s32 a,b;
  u32 addr;
  addr = get_eab[7](reg0);
  a = read_B(addr)<<24;
  b = reg68.d[reg9]<<24;
  CMPB(a,b);
}

void lineB08(u32 reg9, u32 reg0)
{
  /* CMP.W Dx,Dy */
  s32 a,b;
  a = reg68.d[reg0]<<16;
  b = reg68.d[reg9]<<16;
  CMPW(a,b);
}

void lineB09(u32 reg9, u32 reg0)
{
  /* CMP.W Ax,Dy */
  s32 a,b;
  a = reg68.a[reg0]<<16;
  b = reg68.d[reg9]<<16;
  CMPW(a,b);
}

void lineB0A(u32 reg9, u32 reg0)
{
  /* CMP.W <Ae>,Dn */
  s32 a,b;
  u32 addr;
  addr = get_eaw[2](reg0);
  a = read_W(addr)<<16;
  b = reg68.d[reg9]<<16;
  CMPW(a,b);
}

void lineB0B(u32 reg9, u32 reg0)
{
  /* CMP.W <Ae>,Dn */
  s32 a,b;
  u32 addr;
  addr = get_eaw[3](reg0);
  a = read_W(addr)<<16;
  b = reg68.d[reg9]<<16;
  CMPW(a,b);
}

void lineB0C(u32 reg9, u32 reg0)
{
  /* CMP.W <Ae>,Dn */
  s32 a,b;
  u32 addr;
  addr = get_eaw[4](reg0);
  a = read_W(addr)<<16;
  b = reg68.d[reg9]<<16;
  CMPW(a,b);
}

void lineB0D(u32 reg9, u32 reg0)
{
  /* CMP.W <Ae>,Dn */
  s32 a,b;
  u32 addr;
  addr = get_eaw[5](reg0);
  a = read_W(addr)<<16;
  b = reg68.d[reg9]<<16;
  CMPW(a,b);
}

void lineB0E(u32 reg9, u32 reg0)
{
  /* CMP.W <Ae>,Dn */
  s32 a,b;
  u32 addr;
  addr = get_eaw[6](reg0);
  a = read_W(addr)<<16;
  b = reg68.d[reg9]<<16;
  CMPW(a,b);
}

void lineB0F(u32 reg9, u32 reg0)
{
  /* CMP.W <Ae>,Dn */
  s32 a,b;
  u32 addr;
  addr = get_eaw[7](reg0);
  a = read_W(addr)<<16;
  b = reg68.d[reg9]<<16;
  CMPW(a,b);
}

void lineB10(u32 reg9, u32 reg0)
{
  /* CMP.L Dx,Dy */
  s32 a,b;
  a = reg68.d[reg0]<<0;
  b = reg68.d[reg9]<<0;
  CMPL(a,b);
}

void lineB11(u32 reg9, u32 reg0)
{
  /* CMP.L Ax,Dy */
  s32 a,b;
  a = reg68.a[reg0]<<0;
  b = reg68.d[reg9]<<0;
  CMPL(a,b);
}

void lineB12(u32 reg9, u32 reg0)
{
  /* CMP.L <Ae>,Dn */
  s32 a,b;
  u32 addr;
  addr = get_eal[2](reg0);
  a = read_L(addr)<<0;
  b = reg68.d[reg9]<<0;
  CMPL(a,b);
}

void lineB13(u32 reg9, u32 reg0)
{
  /* CMP.L <Ae>,Dn */
  s32 a,b;
  u32 addr;
  addr = get_eal[3](reg0);
  a = read_L(addr)<<0;
  b = reg68.d[reg9]<<0;
  CMPL(a,b);
}

void lineB14(u32 reg9, u32 reg0)
{
  /* CMP.L <Ae>,Dn */
  s32 a,b;
  u32 addr;
  addr = get_eal[4](reg0);
  a = read_L(addr)<<0;
  b = reg68.d[reg9]<<0;
  CMPL(a,b);
}

void lineB15(u32 reg9, u32 reg0)
{
  /* CMP.L <Ae>,Dn */
  s32 a,b;
  u32 addr;
  addr = get_eal[5](reg0);
  a = read_L(addr)<<0;
  b = reg68.d[reg9]<<0;
  CMPL(a,b);
}

void lineB16(u32 reg9, u32 reg0)
{
  /* CMP.L <Ae>,Dn */
  s32 a,b;
  u32 addr;
  addr = get_eal[6](reg0);
  a = read_L(addr)<<0;
  b = reg68.d[reg9]<<0;
  CMPL(a,b);
}

void lineB17(u32 reg9, u32 reg0)
{
  /* CMP.L <Ae>,Dn */
  s32 a,b;
  u32 addr;
  addr = get_eal[7](reg0);
  a = read_L(addr)<<0;
  b = reg68.d[reg9]<<0;
  CMPL(a,b);
}

void lineB18(u32 reg9, u32 reg0)
{
  /* CMP.W Dx,Ay */
  s32 a,b;
  a = reg68.d[reg0]<<16;
  b = reg68.a[reg9];
  CMPAW(a,b);
}

void lineB19(u32 reg9, u32 reg0)
{
  /* CMP.W Ax,Ay */
  s32 a,b;
  a = reg68.a[reg0]<<16;
  b = reg68.a[reg9];
  CMPAW(a,b);
}

void lineB1A(u32 reg9, u32 reg0)
{
  /* CMP.W <Ae>,An */
  s32 a,b;
  u32 addr;
  addr = get_eaw[2](reg0);
  a = read_W(addr)<<16;
  b = reg68.a[reg9];
  CMPAW(a,b);
}

void lineB1B(u32 reg9, u32 reg0)
{
  /* CMP.W <Ae>,An */
  s32 a,b;
  u32 addr;
  addr = get_eaw[3](reg0);
  a = read_W(addr)<<16;
  b = reg68.a[reg9];
  CMPAW(a,b);
}

void lineB1C(u32 reg9, u32 reg0)
{
  /* CMP.W <Ae>,An */
  s32 a,b;
  u32 addr;
  addr = get_eaw[4](reg0);
  a = read_W(addr)<<16;
  b = reg68.a[reg9];
  CMPAW(a,b);
}

void lineB1D(u32 reg9, u32 reg0)
{
  /* CMP.W <Ae>,An */
  s32 a,b;
  u32 addr;
  addr = get_eaw[5](reg0);
  a = read_W(addr)<<16;
  b = reg68.a[reg9];
  CMPAW(a,b);
}

void lineB1E(u32 reg9, u32 reg0)
{
  /* CMP.W <Ae>,An */
  s32 a,b;
  u32 addr;
  addr = get_eaw[6](reg0);
  a = read_W(addr)<<16;
  b = reg68.a[reg9];
  CMPAW(a,b);
}

void lineB1F(u32 reg9, u32 reg0)
{
  /* CMP.W <Ae>,An */
  s32 a,b;
  u32 addr;
  addr = get_eaw[7](reg0);
  a = read_W(addr)<<16;
  b = reg68.a[reg9];
  CMPAW(a,b);
}

void lineB20(u32 reg9, u32 reg0)
{
  /* EOR.B Dx,Dy */
  s32 a,b,s;
  a = reg68.d[reg9]<<24;
  b = reg68.d[reg0]<<24;
  EORB(s,a,b);
  reg68.d[reg0] = (reg68.d[reg0]&0xffffff00) + ((u32)s>>24);
}

void lineB21(u32 reg9, u32 reg0)
{
  /* CMPM.B (Ay)+,(Ax)+ */
  s32 x,y;
  y = read_B(reg68.a[reg0]-=1);
  x = read_B(reg68.a[reg9]-=1);
  CMPB(y,x);
}

void lineB22(u32 reg9, u32 reg0)
{
  /* EOR.B Dn,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = reg68.d[reg9]<<24;
  addr = get_eab[2](reg0);
  b = read_B(addr)<<24;
  EORB(s,a,b);
  write_B(addr,(u32)s>>24);
}

void lineB23(u32 reg9, u32 reg0)
{
  /* EOR.B Dn,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = reg68.d[reg9]<<24;
  addr = get_eab[3](reg0);
  b = read_B(addr)<<24;
  EORB(s,a,b);
  write_B(addr,(u32)s>>24);
}

void lineB24(u32 reg9, u32 reg0)
{
  /* EOR.B Dn,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = reg68.d[reg9]<<24;
  addr = get_eab[4](reg0);
  b = read_B(addr)<<24;
  EORB(s,a,b);
  write_B(addr,(u32)s>>24);
}

void lineB25(u32 reg9, u32 reg0)
{
  /* EOR.B Dn,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = reg68.d[reg9]<<24;
  addr = get_eab[5](reg0);
  b = read_B(addr)<<24;
  EORB(s,a,b);
  write_B(addr,(u32)s>>24);
}

void lineB26(u32 reg9, u32 reg0)
{
  /* EOR.B Dn,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = reg68.d[reg9]<<24;
  addr = get_eab[6](reg0);
  b = read_B(addr)<<24;
  EORB(s,a,b);
  write_B(addr,(u32)s>>24);
}

void lineB27(u32 reg9, u32 reg0)
{
  /* EOR.B Dn,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = reg68.d[reg9]<<24;
  addr = get_eab[7](reg0);
  b = read_B(addr)<<24;
  EORB(s,a,b);
  write_B(addr,(u32)s>>24);
}

void lineB28(u32 reg9, u32 reg0)
{
  /* EOR.W Dx,Dy */
  s32 a,b,s;
  a = reg68.d[reg9]<<16;
  b = reg68.d[reg0]<<16;
  EORW(s,a,b);
  reg68.d[reg0] = (reg68.d[reg0]&0xffff0000) + ((u32)s>>16);
}

void lineB29(u32 reg9, u32 reg0)
{
  /* CMPM.W (Ay)+,(Ax)+ */
  s32 x,y;
  y = read_W(reg68.a[reg0]-=2);
  x = read_W(reg68.a[reg9]-=2);
  CMPW(y,x);
}

void lineB2A(u32 reg9, u32 reg0)
{
  /* EOR.W Dn,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = reg68.d[reg9]<<16;
  addr = get_eaw[2](reg0);
  b = read_W(addr)<<16;
  EORW(s,a,b);
  write_W(addr,(u32)s>>16);
}

void lineB2B(u32 reg9, u32 reg0)
{
  /* EOR.W Dn,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = reg68.d[reg9]<<16;
  addr = get_eaw[3](reg0);
  b = read_W(addr)<<16;
  EORW(s,a,b);
  write_W(addr,(u32)s>>16);
}

void lineB2C(u32 reg9, u32 reg0)
{
  /* EOR.W Dn,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = reg68.d[reg9]<<16;
  addr = get_eaw[4](reg0);
  b = read_W(addr)<<16;
  EORW(s,a,b);
  write_W(addr,(u32)s>>16);
}

void lineB2D(u32 reg9, u32 reg0)
{
  /* EOR.W Dn,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = reg68.d[reg9]<<16;
  addr = get_eaw[5](reg0);
  b = read_W(addr)<<16;
  EORW(s,a,b);
  write_W(addr,(u32)s>>16);
}

void lineB2E(u32 reg9, u32 reg0)
{
  /* EOR.W Dn,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = reg68.d[reg9]<<16;
  addr = get_eaw[6](reg0);
  b = read_W(addr)<<16;
  EORW(s,a,b);
  write_W(addr,(u32)s>>16);
}

void lineB2F(u32 reg9, u32 reg0)
{
  /* EOR.W Dn,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = reg68.d[reg9]<<16;
  addr = get_eaw[7](reg0);
  b = read_W(addr)<<16;
  EORW(s,a,b);
  write_W(addr,(u32)s>>16);
}

void lineB30(u32 reg9, u32 reg0)
{
  /* EOR.L Dx,Dy */
  s32 a,b,s;
  a = reg68.d[reg9]<<0;
  b = reg68.d[reg0]<<0;
  EORL(s,a,b);
  reg68.d[reg0] = (reg68.d[reg0]&0) + ((u32)s>>0);
}

void lineB31(u32 reg9, u32 reg0)
{
  /* CMPM.L (Ay)+,(Ax)+ */
  s32 x,y;
  y = read_L(reg68.a[reg0]-=4);
  x = read_L(reg68.a[reg9]-=4);
  CMPL(y,x);
}

void lineB32(u32 reg9, u32 reg0)
{
  /* EOR.L Dn,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = reg68.d[reg9]<<0;
  addr = get_eal[2](reg0);
  b = read_L(addr)<<0;
  EORL(s,a,b);
  write_L(addr,(u32)s>>0);
}

void lineB33(u32 reg9, u32 reg0)
{
  /* EOR.L Dn,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = reg68.d[reg9]<<0;
  addr = get_eal[3](reg0);
  b = read_L(addr)<<0;
  EORL(s,a,b);
  write_L(addr,(u32)s>>0);
}

void lineB34(u32 reg9, u32 reg0)
{
  /* EOR.L Dn,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = reg68.d[reg9]<<0;
  addr = get_eal[4](reg0);
  b = read_L(addr)<<0;
  EORL(s,a,b);
  write_L(addr,(u32)s>>0);
}

void lineB35(u32 reg9, u32 reg0)
{
  /* EOR.L Dn,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = reg68.d[reg9]<<0;
  addr = get_eal[5](reg0);
  b = read_L(addr)<<0;
  EORL(s,a,b);
  write_L(addr,(u32)s>>0);
}

void lineB36(u32 reg9, u32 reg0)
{
  /* EOR.L Dn,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = reg68.d[reg9]<<0;
  addr = get_eal[6](reg0);
  b = read_L(addr)<<0;
  EORL(s,a,b);
  write_L(addr,(u32)s>>0);
}

void lineB37(u32 reg9, u32 reg0)
{
  /* EOR.L Dn,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = reg68.d[reg9]<<0;
  addr = get_eal[7](reg0);
  b = read_L(addr)<<0;
  EORL(s,a,b);
  write_L(addr,(u32)s>>0);
}

void lineB38(u32 reg9, u32 reg0)
{
  /* CMP.L Dx,Ay */
  s32 a,b;
  a = reg68.d[reg0]<<0;
  b = reg68.a[reg9];
  CMPAL(a,b);
}

void lineB39(u32 reg9, u32 reg0)
{
  /* CMP.L Ax,Ay */
  s32 a,b;
  a = reg68.a[reg0]<<0;
  b = reg68.a[reg9];
  CMPAL(a,b);
}

void lineB3A(u32 reg9, u32 reg0)
{
  /* CMP.L <Ae>,An */
  s32 a,b;
  u32 addr;
  addr = get_eal[2](reg0);
  a = read_L(addr)<<0;
  b = reg68.a[reg9];
  CMPAL(a,b);
}

void lineB3B(u32 reg9, u32 reg0)
{
  /* CMP.L <Ae>,An */
  s32 a,b;
  u32 addr;
  addr = get_eal[3](reg0);
  a = read_L(addr)<<0;
  b = reg68.a[reg9];
  CMPAL(a,b);
}

void lineB3C(u32 reg9, u32 reg0)
{
  /* CMP.L <Ae>,An */
  s32 a,b;
  u32 addr;
  addr = get_eal[4](reg0);
  a = read_L(addr)<<0;
  b = reg68.a[reg9];
  CMPAL(a,b);
}

void lineB3D(u32 reg9, u32 reg0)
{
  /* CMP.L <Ae>,An */
  s32 a,b;
  u32 addr;
  addr = get_eal[5](reg0);
  a = read_L(addr)<<0;
  b = reg68.a[reg9];
  CMPAL(a,b);
}

void lineB3E(u32 reg9, u32 reg0)
{
  /* CMP.L <Ae>,An */
  s32 a,b;
  u32 addr;
  addr = get_eal[6](reg0);
  a = read_L(addr)<<0;
  b = reg68.a[reg9];
  CMPAL(a,b);
}

void lineB3F(u32 reg9, u32 reg0)
{
  /* CMP.L <Ae>,An */
  s32 a,b;
  u32 addr;
  addr = get_eal[7](reg0);
  a = read_L(addr)<<0;
  b = reg68.a[reg9];
  CMPAL(a,b);
}

