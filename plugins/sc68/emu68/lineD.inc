void lineD00(u32 reg9, u32 reg0)
{
  /* ADD.B Dx,Dy */
  s32 a,b,s;
  a = reg68.d[reg0]<<24;
  b = reg68.d[reg9]<<24;
  ADDB(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0xffffff00) + ((u32)s>>24);
}

void lineD01(u32 reg9, u32 reg0)
{
  /* ADD.B Ax,Dy */
  s32 a,b,s;
  a = reg68.a[reg0]<<24;
  b = reg68.d[reg9]<<24;
  ADDB(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0xffffff00) + ((u32)s>>24);
}

void lineD02(u32 reg9, u32 reg0)
{
  /* ADD.B <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eab[2](reg0);
  a = read_B(addr)<<24;
  b = reg68.d[reg9]<<24;
  ADDB(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0xffffff00) + ((u32)s>>24);
}

void lineD03(u32 reg9, u32 reg0)
{
  /* ADD.B <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eab[3](reg0);
  a = read_B(addr)<<24;
  b = reg68.d[reg9]<<24;
  ADDB(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0xffffff00) + ((u32)s>>24);
}

void lineD04(u32 reg9, u32 reg0)
{
  /* ADD.B <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eab[4](reg0);
  a = read_B(addr)<<24;
  b = reg68.d[reg9]<<24;
  ADDB(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0xffffff00) + ((u32)s>>24);
}

void lineD05(u32 reg9, u32 reg0)
{
  /* ADD.B <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eab[5](reg0);
  a = read_B(addr)<<24;
  b = reg68.d[reg9]<<24;
  ADDB(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0xffffff00) + ((u32)s>>24);
}

void lineD06(u32 reg9, u32 reg0)
{
  /* ADD.B <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eab[6](reg0);
  a = read_B(addr)<<24;
  b = reg68.d[reg9]<<24;
  ADDB(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0xffffff00) + ((u32)s>>24);
}

void lineD07(u32 reg9, u32 reg0)
{
  /* ADD.B <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eab[7](reg0);
  a = read_B(addr)<<24;
  b = reg68.d[reg9]<<24;
  ADDB(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0xffffff00) + ((u32)s>>24);
}

void lineD08(u32 reg9, u32 reg0)
{
  /* ADD.W Dx,Dy */
  s32 a,b,s;
  a = reg68.d[reg0]<<16;
  b = reg68.d[reg9]<<16;
  ADDW(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0xffff0000) + ((u32)s>>16);
}

void lineD09(u32 reg9, u32 reg0)
{
  /* ADD.W Ax,Dy */
  s32 a,b,s;
  a = reg68.a[reg0]<<16;
  b = reg68.d[reg9]<<16;
  ADDW(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0xffff0000) + ((u32)s>>16);
}

void lineD0A(u32 reg9, u32 reg0)
{
  /* ADD.W <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eaw[2](reg0);
  a = read_W(addr)<<16;
  b = reg68.d[reg9]<<16;
  ADDW(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0xffff0000) + ((u32)s>>16);
}

void lineD0B(u32 reg9, u32 reg0)
{
  /* ADD.W <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eaw[3](reg0);
  a = read_W(addr)<<16;
  b = reg68.d[reg9]<<16;
  ADDW(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0xffff0000) + ((u32)s>>16);
}

void lineD0C(u32 reg9, u32 reg0)
{
  /* ADD.W <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eaw[4](reg0);
  a = read_W(addr)<<16;
  b = reg68.d[reg9]<<16;
  ADDW(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0xffff0000) + ((u32)s>>16);
}

void lineD0D(u32 reg9, u32 reg0)
{
  /* ADD.W <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eaw[5](reg0);
  a = read_W(addr)<<16;
  b = reg68.d[reg9]<<16;
  ADDW(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0xffff0000) + ((u32)s>>16);
}

void lineD0E(u32 reg9, u32 reg0)
{
  /* ADD.W <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eaw[6](reg0);
  a = read_W(addr)<<16;
  b = reg68.d[reg9]<<16;
  ADDW(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0xffff0000) + ((u32)s>>16);
}

void lineD0F(u32 reg9, u32 reg0)
{
  /* ADD.W <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eaw[7](reg0);
  a = read_W(addr)<<16;
  b = reg68.d[reg9]<<16;
  ADDW(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0xffff0000) + ((u32)s>>16);
}

void lineD10(u32 reg9, u32 reg0)
{
  /* ADD.L Dx,Dy */
  s32 a,b,s;
  a = reg68.d[reg0]<<0;
  b = reg68.d[reg9]<<0;
  ADDL(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0) + ((u32)s>>0);
}

void lineD11(u32 reg9, u32 reg0)
{
  /* ADD.L Ax,Dy */
  s32 a,b,s;
  a = reg68.a[reg0]<<0;
  b = reg68.d[reg9]<<0;
  ADDL(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0) + ((u32)s>>0);
}

void lineD12(u32 reg9, u32 reg0)
{
  /* ADD.L <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eal[2](reg0);
  a = read_L(addr)<<0;
  b = reg68.d[reg9]<<0;
  ADDL(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0) + ((u32)s>>0);
}

void lineD13(u32 reg9, u32 reg0)
{
  /* ADD.L <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eal[3](reg0);
  a = read_L(addr)<<0;
  b = reg68.d[reg9]<<0;
  ADDL(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0) + ((u32)s>>0);
}

void lineD14(u32 reg9, u32 reg0)
{
  /* ADD.L <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eal[4](reg0);
  a = read_L(addr)<<0;
  b = reg68.d[reg9]<<0;
  ADDL(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0) + ((u32)s>>0);
}

void lineD15(u32 reg9, u32 reg0)
{
  /* ADD.L <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eal[5](reg0);
  a = read_L(addr)<<0;
  b = reg68.d[reg9]<<0;
  ADDL(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0) + ((u32)s>>0);
}

void lineD16(u32 reg9, u32 reg0)
{
  /* ADD.L <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eal[6](reg0);
  a = read_L(addr)<<0;
  b = reg68.d[reg9]<<0;
  ADDL(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0) + ((u32)s>>0);
}

void lineD17(u32 reg9, u32 reg0)
{
  /* ADD.L <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eal[7](reg0);
  a = read_L(addr)<<0;
  b = reg68.d[reg9]<<0;
  ADDL(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0) + ((u32)s>>0);
}

void lineD18(u32 reg9, u32 reg0)
{
  /* ADD.W Dx,Ay */
  s32 a,b,s;
  a = reg68.d[reg0]<<16;
  b = reg68.a[reg9];
  ADDAW(s,a,b);
  reg68.a[reg9] = s;
}

void lineD19(u32 reg9, u32 reg0)
{
  /* ADD.W Ax,Ay */
  s32 a,b,s;
  a = reg68.a[reg0]<<16;
  b = reg68.a[reg9];
  ADDAW(s,a,b);
  reg68.a[reg9] = s;
}

void lineD1A(u32 reg9, u32 reg0)
{
  /* ADD.W <Ae>,An */
  s32 a,b,s;
  u32 addr;
  addr = get_eaw[2](reg0);
  a = read_W(addr)<<16;
  b = reg68.a[reg9];
  ADDAW(s,a,b);
  reg68.a[reg9] = s;
}

void lineD1B(u32 reg9, u32 reg0)
{
  /* ADD.W <Ae>,An */
  s32 a,b,s;
  u32 addr;
  addr = get_eaw[3](reg0);
  a = read_W(addr)<<16;
  b = reg68.a[reg9];
  ADDAW(s,a,b);
  reg68.a[reg9] = s;
}

void lineD1C(u32 reg9, u32 reg0)
{
  /* ADD.W <Ae>,An */
  s32 a,b,s;
  u32 addr;
  addr = get_eaw[4](reg0);
  a = read_W(addr)<<16;
  b = reg68.a[reg9];
  ADDAW(s,a,b);
  reg68.a[reg9] = s;
}

void lineD1D(u32 reg9, u32 reg0)
{
  /* ADD.W <Ae>,An */
  s32 a,b,s;
  u32 addr;
  addr = get_eaw[5](reg0);
  a = read_W(addr)<<16;
  b = reg68.a[reg9];
  ADDAW(s,a,b);
  reg68.a[reg9] = s;
}

void lineD1E(u32 reg9, u32 reg0)
{
  /* ADD.W <Ae>,An */
  s32 a,b,s;
  u32 addr;
  addr = get_eaw[6](reg0);
  a = read_W(addr)<<16;
  b = reg68.a[reg9];
  ADDAW(s,a,b);
  reg68.a[reg9] = s;
}

void lineD1F(u32 reg9, u32 reg0)
{
  /* ADD.W <Ae>,An */
  s32 a,b,s;
  u32 addr;
  addr = get_eaw[7](reg0);
  a = read_W(addr)<<16;
  b = reg68.a[reg9];
  ADDAW(s,a,b);
  reg68.a[reg9] = s;
}

void lineD20(u32 reg9, u32 reg0)
{
  s32 a,b,s;
  a = reg68.d[reg0]<<24;
  b = reg68.d[reg9]<<24;
  ADDXB(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0xffffff00) + ((u32)s>>24);
}

void lineD21(u32 reg9, u32 reg0)
{
  s32 a,b,s;
  a = read_B(reg68.a[reg0]-=1)<<24;
  b = read_B(reg68.a[reg9]-=1)<<24;
  ADDXB(s,a,b);
  write_B(reg68.a[reg9],(u32)s>>24);
}

void lineD22(u32 reg9, u32 reg0)
{
  /* ADD.B Dn,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = reg68.d[reg9]<<24;
  addr = get_eab[2](reg0);
  b = read_B(addr)<<24;
  ADDB(s,a,b);
  write_B(addr,(u32)s>>24);
}

void lineD23(u32 reg9, u32 reg0)
{
  /* ADD.B Dn,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = reg68.d[reg9]<<24;
  addr = get_eab[3](reg0);
  b = read_B(addr)<<24;
  ADDB(s,a,b);
  write_B(addr,(u32)s>>24);
}

void lineD24(u32 reg9, u32 reg0)
{
  /* ADD.B Dn,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = reg68.d[reg9]<<24;
  addr = get_eab[4](reg0);
  b = read_B(addr)<<24;
  ADDB(s,a,b);
  write_B(addr,(u32)s>>24);
}

void lineD25(u32 reg9, u32 reg0)
{
  /* ADD.B Dn,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = reg68.d[reg9]<<24;
  addr = get_eab[5](reg0);
  b = read_B(addr)<<24;
  ADDB(s,a,b);
  write_B(addr,(u32)s>>24);
}

void lineD26(u32 reg9, u32 reg0)
{
  /* ADD.B Dn,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = reg68.d[reg9]<<24;
  addr = get_eab[6](reg0);
  b = read_B(addr)<<24;
  ADDB(s,a,b);
  write_B(addr,(u32)s>>24);
}

void lineD27(u32 reg9, u32 reg0)
{
  /* ADD.B Dn,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = reg68.d[reg9]<<24;
  addr = get_eab[7](reg0);
  b = read_B(addr)<<24;
  ADDB(s,a,b);
  write_B(addr,(u32)s>>24);
}

void lineD28(u32 reg9, u32 reg0)
{
  s32 a,b,s;
  a = reg68.d[reg0]<<16;
  b = reg68.d[reg9]<<16;
  ADDXW(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0xffff0000) + ((u32)s>>16);
}

void lineD29(u32 reg9, u32 reg0)
{
  s32 a,b,s;
  a = read_W(reg68.a[reg0]-=2)<<16;
  b = read_W(reg68.a[reg9]-=2)<<16;
  ADDXW(s,a,b);
  write_W(reg68.a[reg9],(u32)s>>16);
}

void lineD2A(u32 reg9, u32 reg0)
{
  /* ADD.W Dn,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = reg68.d[reg9]<<16;
  addr = get_eaw[2](reg0);
  b = read_W(addr)<<16;
  ADDW(s,a,b);
  write_W(addr,(u32)s>>16);
}

void lineD2B(u32 reg9, u32 reg0)
{
  /* ADD.W Dn,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = reg68.d[reg9]<<16;
  addr = get_eaw[3](reg0);
  b = read_W(addr)<<16;
  ADDW(s,a,b);
  write_W(addr,(u32)s>>16);
}

void lineD2C(u32 reg9, u32 reg0)
{
  /* ADD.W Dn,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = reg68.d[reg9]<<16;
  addr = get_eaw[4](reg0);
  b = read_W(addr)<<16;
  ADDW(s,a,b);
  write_W(addr,(u32)s>>16);
}

void lineD2D(u32 reg9, u32 reg0)
{
  /* ADD.W Dn,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = reg68.d[reg9]<<16;
  addr = get_eaw[5](reg0);
  b = read_W(addr)<<16;
  ADDW(s,a,b);
  write_W(addr,(u32)s>>16);
}

void lineD2E(u32 reg9, u32 reg0)
{
  /* ADD.W Dn,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = reg68.d[reg9]<<16;
  addr = get_eaw[6](reg0);
  b = read_W(addr)<<16;
  ADDW(s,a,b);
  write_W(addr,(u32)s>>16);
}

void lineD2F(u32 reg9, u32 reg0)
{
  /* ADD.W Dn,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = reg68.d[reg9]<<16;
  addr = get_eaw[7](reg0);
  b = read_W(addr)<<16;
  ADDW(s,a,b);
  write_W(addr,(u32)s>>16);
}

void lineD30(u32 reg9, u32 reg0)
{
  s32 a,b,s;
  a = reg68.d[reg0]<<0;
  b = reg68.d[reg9]<<0;
  ADDXL(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0) + ((u32)s>>0);
}

void lineD31(u32 reg9, u32 reg0)
{
  s32 a,b,s;
  a = read_L(reg68.a[reg0]-=4)<<0;
  b = read_L(reg68.a[reg9]-=4)<<0;
  ADDXL(s,a,b);
  write_L(reg68.a[reg9],(u32)s>>0);
}

void lineD32(u32 reg9, u32 reg0)
{
  /* ADD.L Dn,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = reg68.d[reg9]<<0;
  addr = get_eal[2](reg0);
  b = read_L(addr)<<0;
  ADDL(s,a,b);
  write_L(addr,(u32)s>>0);
}

void lineD33(u32 reg9, u32 reg0)
{
  /* ADD.L Dn,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = reg68.d[reg9]<<0;
  addr = get_eal[3](reg0);
  b = read_L(addr)<<0;
  ADDL(s,a,b);
  write_L(addr,(u32)s>>0);
}

void lineD34(u32 reg9, u32 reg0)
{
  /* ADD.L Dn,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = reg68.d[reg9]<<0;
  addr = get_eal[4](reg0);
  b = read_L(addr)<<0;
  ADDL(s,a,b);
  write_L(addr,(u32)s>>0);
}

void lineD35(u32 reg9, u32 reg0)
{
  /* ADD.L Dn,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = reg68.d[reg9]<<0;
  addr = get_eal[5](reg0);
  b = read_L(addr)<<0;
  ADDL(s,a,b);
  write_L(addr,(u32)s>>0);
}

void lineD36(u32 reg9, u32 reg0)
{
  /* ADD.L Dn,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = reg68.d[reg9]<<0;
  addr = get_eal[6](reg0);
  b = read_L(addr)<<0;
  ADDL(s,a,b);
  write_L(addr,(u32)s>>0);
}

void lineD37(u32 reg9, u32 reg0)
{
  /* ADD.L Dn,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = reg68.d[reg9]<<0;
  addr = get_eal[7](reg0);
  b = read_L(addr)<<0;
  ADDL(s,a,b);
  write_L(addr,(u32)s>>0);
}

void lineD38(u32 reg9, u32 reg0)
{
  /* ADD.L Dx,Ay */
  s32 a,b,s;
  a = reg68.d[reg0]<<0;
  b = reg68.a[reg9];
  ADDAL(s,a,b);
  reg68.a[reg9] = s;
}

void lineD39(u32 reg9, u32 reg0)
{
  /* ADD.L Ax,Ay */
  s32 a,b,s;
  a = reg68.a[reg0]<<0;
  b = reg68.a[reg9];
  ADDAL(s,a,b);
  reg68.a[reg9] = s;
}

void lineD3A(u32 reg9, u32 reg0)
{
  /* ADD.L <Ae>,An */
  s32 a,b,s;
  u32 addr;
  addr = get_eal[2](reg0);
  a = read_L(addr)<<0;
  b = reg68.a[reg9];
  ADDAL(s,a,b);
  reg68.a[reg9] = s;
}

void lineD3B(u32 reg9, u32 reg0)
{
  /* ADD.L <Ae>,An */
  s32 a,b,s;
  u32 addr;
  addr = get_eal[3](reg0);
  a = read_L(addr)<<0;
  b = reg68.a[reg9];
  ADDAL(s,a,b);
  reg68.a[reg9] = s;
}

void lineD3C(u32 reg9, u32 reg0)
{
  /* ADD.L <Ae>,An */
  s32 a,b,s;
  u32 addr;
  addr = get_eal[4](reg0);
  a = read_L(addr)<<0;
  b = reg68.a[reg9];
  ADDAL(s,a,b);
  reg68.a[reg9] = s;
}

void lineD3D(u32 reg9, u32 reg0)
{
  /* ADD.L <Ae>,An */
  s32 a,b,s;
  u32 addr;
  addr = get_eal[5](reg0);
  a = read_L(addr)<<0;
  b = reg68.a[reg9];
  ADDAL(s,a,b);
  reg68.a[reg9] = s;
}

void lineD3E(u32 reg9, u32 reg0)
{
  /* ADD.L <Ae>,An */
  s32 a,b,s;
  u32 addr;
  addr = get_eal[6](reg0);
  a = read_L(addr)<<0;
  b = reg68.a[reg9];
  ADDAL(s,a,b);
  reg68.a[reg9] = s;
}

void lineD3F(u32 reg9, u32 reg0)
{
  /* ADD.L <Ae>,An */
  s32 a,b,s;
  u32 addr;
  addr = get_eal[7](reg0);
  a = read_L(addr)<<0;
  b = reg68.a[reg9];
  ADDAL(s,a,b);
  reg68.a[reg9] = s;
}

