void line800(u32 reg9, u32 reg0)
{
  /* OR.B Dx,Dy */
  s32 a,b,s;
  a = reg68.d[reg0]<<24;
  b = reg68.d[reg9]<<24;
  ORB(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0xffffff00) + ((u32)s>>24);
}

void line801(u32 reg9, u32 reg0)
{
  ILLEGAL;
}

void line802(u32 reg9, u32 reg0)
{
  /* OR.B <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eab[2](reg0);
  a = read_B(addr)<<24;
  b = reg68.d[reg9]<<24;
  ORB(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0xffffff00) + ((u32)s>>24);
}

void line803(u32 reg9, u32 reg0)
{
  /* OR.B <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eab[3](reg0);
  a = read_B(addr)<<24;
  b = reg68.d[reg9]<<24;
  ORB(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0xffffff00) + ((u32)s>>24);
}

void line804(u32 reg9, u32 reg0)
{
  /* OR.B <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eab[4](reg0);
  a = read_B(addr)<<24;
  b = reg68.d[reg9]<<24;
  ORB(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0xffffff00) + ((u32)s>>24);
}

void line805(u32 reg9, u32 reg0)
{
  /* OR.B <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eab[5](reg0);
  a = read_B(addr)<<24;
  b = reg68.d[reg9]<<24;
  ORB(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0xffffff00) + ((u32)s>>24);
}

void line806(u32 reg9, u32 reg0)
{
  /* OR.B <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eab[6](reg0);
  a = read_B(addr)<<24;
  b = reg68.d[reg9]<<24;
  ORB(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0xffffff00) + ((u32)s>>24);
}

void line807(u32 reg9, u32 reg0)
{
  /* OR.B <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eab[7](reg0);
  a = read_B(addr)<<24;
  b = reg68.d[reg9]<<24;
  ORB(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0xffffff00) + ((u32)s>>24);
}

void line808(u32 reg9, u32 reg0)
{
  /* OR.W Dx,Dy */
  s32 a,b,s;
  a = reg68.d[reg0]<<16;
  b = reg68.d[reg9]<<16;
  ORW(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0xffff0000) + ((u32)s>>16);
}

void line809(u32 reg9, u32 reg0)
{
  ILLEGAL;
}

void line80A(u32 reg9, u32 reg0)
{
  /* OR.W <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eaw[2](reg0);
  a = read_W(addr)<<16;
  b = reg68.d[reg9]<<16;
  ORW(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0xffff0000) + ((u32)s>>16);
}

void line80B(u32 reg9, u32 reg0)
{
  /* OR.W <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eaw[3](reg0);
  a = read_W(addr)<<16;
  b = reg68.d[reg9]<<16;
  ORW(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0xffff0000) + ((u32)s>>16);
}

void line80C(u32 reg9, u32 reg0)
{
  /* OR.W <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eaw[4](reg0);
  a = read_W(addr)<<16;
  b = reg68.d[reg9]<<16;
  ORW(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0xffff0000) + ((u32)s>>16);
}

void line80D(u32 reg9, u32 reg0)
{
  /* OR.W <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eaw[5](reg0);
  a = read_W(addr)<<16;
  b = reg68.d[reg9]<<16;
  ORW(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0xffff0000) + ((u32)s>>16);
}

void line80E(u32 reg9, u32 reg0)
{
  /* OR.W <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eaw[6](reg0);
  a = read_W(addr)<<16;
  b = reg68.d[reg9]<<16;
  ORW(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0xffff0000) + ((u32)s>>16);
}

void line80F(u32 reg9, u32 reg0)
{
  /* OR.W <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eaw[7](reg0);
  a = read_W(addr)<<16;
  b = reg68.d[reg9]<<16;
  ORW(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0xffff0000) + ((u32)s>>16);
}

void line810(u32 reg9, u32 reg0)
{
  /* OR.L Dx,Dy */
  s32 a,b,s;
  a = reg68.d[reg0]<<0;
  b = reg68.d[reg9]<<0;
  ORL(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0) + ((u32)s>>0);
}

void line811(u32 reg9, u32 reg0)
{
  ILLEGAL;
}

void line812(u32 reg9, u32 reg0)
{
  /* OR.L <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eal[2](reg0);
  a = read_L(addr)<<0;
  b = reg68.d[reg9]<<0;
  ORL(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0) + ((u32)s>>0);
}

void line813(u32 reg9, u32 reg0)
{
  /* OR.L <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eal[3](reg0);
  a = read_L(addr)<<0;
  b = reg68.d[reg9]<<0;
  ORL(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0) + ((u32)s>>0);
}

void line814(u32 reg9, u32 reg0)
{
  /* OR.L <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eal[4](reg0);
  a = read_L(addr)<<0;
  b = reg68.d[reg9]<<0;
  ORL(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0) + ((u32)s>>0);
}

void line815(u32 reg9, u32 reg0)
{
  /* OR.L <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eal[5](reg0);
  a = read_L(addr)<<0;
  b = reg68.d[reg9]<<0;
  ORL(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0) + ((u32)s>>0);
}

void line816(u32 reg9, u32 reg0)
{
  /* OR.L <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eal[6](reg0);
  a = read_L(addr)<<0;
  b = reg68.d[reg9]<<0;
  ORL(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0) + ((u32)s>>0);
}

void line817(u32 reg9, u32 reg0)
{
  /* OR.L <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eal[7](reg0);
  a = read_L(addr)<<0;
  b = reg68.d[reg9]<<0;
  ORL(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0) + ((u32)s>>0);
}

void line818(u32 reg9, u32 reg0)
{
  /* DIVU.W Dx,Dy */
  s32 a,b,s;
  a = reg68.d[reg0]<<16;
  b = reg68.d[reg9]<<0;
  DIVUW(s,a,b);
  reg68.d[reg9] = s;
}

void line819(u32 reg9, u32 reg0)
{
  /* DIVU.W Ax,Dy */
  s32 a,b,s;
  a = reg68.a[reg0]<<16;
  b = reg68.d[reg9]<<0;
  DIVUW(s,a,b);
  reg68.d[reg9] = s;
}

void line81A(u32 reg9, u32 reg0)
{
  /* DIVU.W <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eaw[2](reg0);
  a = read_W(addr)<<16;
  b = reg68.d[reg9]<<0;
  DIVUW(s,a,b);
  reg68.d[reg9] = s;
}

void line81B(u32 reg9, u32 reg0)
{
  /* DIVU.W <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eaw[3](reg0);
  a = read_W(addr)<<16;
  b = reg68.d[reg9]<<0;
  DIVUW(s,a,b);
  reg68.d[reg9] = s;
}

void line81C(u32 reg9, u32 reg0)
{
  /* DIVU.W <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eaw[4](reg0);
  a = read_W(addr)<<16;
  b = reg68.d[reg9]<<0;
  DIVUW(s,a,b);
  reg68.d[reg9] = s;
}

void line81D(u32 reg9, u32 reg0)
{
  /* DIVU.W <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eaw[5](reg0);
  a = read_W(addr)<<16;
  b = reg68.d[reg9]<<0;
  DIVUW(s,a,b);
  reg68.d[reg9] = s;
}

void line81E(u32 reg9, u32 reg0)
{
  /* DIVU.W <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eaw[6](reg0);
  a = read_W(addr)<<16;
  b = reg68.d[reg9]<<0;
  DIVUW(s,a,b);
  reg68.d[reg9] = s;
}

void line81F(u32 reg9, u32 reg0)
{
  /* DIVU.W <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eaw[7](reg0);
  a = read_W(addr)<<16;
  b = reg68.d[reg9]<<0;
  DIVUW(s,a,b);
  reg68.d[reg9] = s;
}

void line820(u32 reg9, u32 reg0)
{
  /* SBCD Dy,Dx */
  reg0=reg0; reg9=reg9;
}

void line821(u32 reg9, u32 reg0)
{
  /* SBCD -(Ay),-(Ax) */
  reg68.a[reg0] -= 2;
  reg68.a[reg9] -= 2;
}

void line822(u32 reg9, u32 reg0)
{
  /* OR.B Dn,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = reg68.d[reg9]<<24;
  addr = get_eab[2](reg0);
  b = read_B(addr)<<24;
  ORB(s,a,b);
  write_B(addr,(u32)s>>24);
}

void line823(u32 reg9, u32 reg0)
{
  /* OR.B Dn,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = reg68.d[reg9]<<24;
  addr = get_eab[3](reg0);
  b = read_B(addr)<<24;
  ORB(s,a,b);
  write_B(addr,(u32)s>>24);
}

void line824(u32 reg9, u32 reg0)
{
  /* OR.B Dn,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = reg68.d[reg9]<<24;
  addr = get_eab[4](reg0);
  b = read_B(addr)<<24;
  ORB(s,a,b);
  write_B(addr,(u32)s>>24);
}

void line825(u32 reg9, u32 reg0)
{
  /* OR.B Dn,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = reg68.d[reg9]<<24;
  addr = get_eab[5](reg0);
  b = read_B(addr)<<24;
  ORB(s,a,b);
  write_B(addr,(u32)s>>24);
}

void line826(u32 reg9, u32 reg0)
{
  /* OR.B Dn,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = reg68.d[reg9]<<24;
  addr = get_eab[6](reg0);
  b = read_B(addr)<<24;
  ORB(s,a,b);
  write_B(addr,(u32)s>>24);
}

void line827(u32 reg9, u32 reg0)
{
  /* OR.B Dn,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = reg68.d[reg9]<<24;
  addr = get_eab[7](reg0);
  b = read_B(addr)<<24;
  ORB(s,a,b);
  write_B(addr,(u32)s>>24);
}

void line828(u32 reg9, u32 reg0)
{
  /* OR.W Dx,Dy */
  s32 a,b,s;
  a = reg68.d[reg9]<<16;
  b = reg68.d[reg0]<<16;
  ORW(s,a,b);
  reg68.d[reg0] = (reg68.d[reg0]&0xffff0000) + ((u32)s>>16);
}

void line829(u32 reg9, u32 reg0)
{
  ILLEGAL;
}

void line82A(u32 reg9, u32 reg0)
{
  /* OR.W Dn,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = reg68.d[reg9]<<16;
  addr = get_eaw[2](reg0);
  b = read_W(addr)<<16;
  ORW(s,a,b);
  write_W(addr,(u32)s>>16);
}

void line82B(u32 reg9, u32 reg0)
{
  /* OR.W Dn,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = reg68.d[reg9]<<16;
  addr = get_eaw[3](reg0);
  b = read_W(addr)<<16;
  ORW(s,a,b);
  write_W(addr,(u32)s>>16);
}

void line82C(u32 reg9, u32 reg0)
{
  /* OR.W Dn,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = reg68.d[reg9]<<16;
  addr = get_eaw[4](reg0);
  b = read_W(addr)<<16;
  ORW(s,a,b);
  write_W(addr,(u32)s>>16);
}

void line82D(u32 reg9, u32 reg0)
{
  /* OR.W Dn,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = reg68.d[reg9]<<16;
  addr = get_eaw[5](reg0);
  b = read_W(addr)<<16;
  ORW(s,a,b);
  write_W(addr,(u32)s>>16);
}

void line82E(u32 reg9, u32 reg0)
{
  /* OR.W Dn,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = reg68.d[reg9]<<16;
  addr = get_eaw[6](reg0);
  b = read_W(addr)<<16;
  ORW(s,a,b);
  write_W(addr,(u32)s>>16);
}

void line82F(u32 reg9, u32 reg0)
{
  /* OR.W Dn,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = reg68.d[reg9]<<16;
  addr = get_eaw[7](reg0);
  b = read_W(addr)<<16;
  ORW(s,a,b);
  write_W(addr,(u32)s>>16);
}

void line830(u32 reg9, u32 reg0)
{
  /* OR.L Dx,Dy */
  s32 a,b,s;
  a = reg68.d[reg9]<<0;
  b = reg68.d[reg0]<<0;
  ORL(s,a,b);
  reg68.d[reg0] = (reg68.d[reg0]&0) + ((u32)s>>0);
}

void line831(u32 reg9, u32 reg0)
{
  ILLEGAL;
}

void line832(u32 reg9, u32 reg0)
{
  /* OR.L Dn,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = reg68.d[reg9]<<0;
  addr = get_eal[2](reg0);
  b = read_L(addr)<<0;
  ORL(s,a,b);
  write_L(addr,(u32)s>>0);
}

void line833(u32 reg9, u32 reg0)
{
  /* OR.L Dn,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = reg68.d[reg9]<<0;
  addr = get_eal[3](reg0);
  b = read_L(addr)<<0;
  ORL(s,a,b);
  write_L(addr,(u32)s>>0);
}

void line834(u32 reg9, u32 reg0)
{
  /* OR.L Dn,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = reg68.d[reg9]<<0;
  addr = get_eal[4](reg0);
  b = read_L(addr)<<0;
  ORL(s,a,b);
  write_L(addr,(u32)s>>0);
}

void line835(u32 reg9, u32 reg0)
{
  /* OR.L Dn,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = reg68.d[reg9]<<0;
  addr = get_eal[5](reg0);
  b = read_L(addr)<<0;
  ORL(s,a,b);
  write_L(addr,(u32)s>>0);
}

void line836(u32 reg9, u32 reg0)
{
  /* OR.L Dn,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = reg68.d[reg9]<<0;
  addr = get_eal[6](reg0);
  b = read_L(addr)<<0;
  ORL(s,a,b);
  write_L(addr,(u32)s>>0);
}

void line837(u32 reg9, u32 reg0)
{
  /* OR.L Dn,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = reg68.d[reg9]<<0;
  addr = get_eal[7](reg0);
  b = read_L(addr)<<0;
  ORL(s,a,b);
  write_L(addr,(u32)s>>0);
}

void line838(u32 reg9, u32 reg0)
{
  /* DIVS.W Dx,Dy */
  s32 a,b,s;
  a = reg68.d[reg0]<<16;
  b = reg68.d[reg9]<<0;
  DIVSW(s,a,b);
  reg68.d[reg9] = s;
}

void line839(u32 reg9, u32 reg0)
{
  /* DIVS.W Ax,Dy */
  s32 a,b,s;
  a = reg68.a[reg0]<<16;
  b = reg68.d[reg9]<<0;
  DIVSW(s,a,b);
  reg68.d[reg9] = s;
}

void line83A(u32 reg9, u32 reg0)
{
  /* DIVS.W <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eaw[2](reg0);
  a = read_W(addr)<<16;
  b = reg68.d[reg9]<<0;
  DIVSW(s,a,b);
  reg68.d[reg9] = s;
}

void line83B(u32 reg9, u32 reg0)
{
  /* DIVS.W <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eaw[3](reg0);
  a = read_W(addr)<<16;
  b = reg68.d[reg9]<<0;
  DIVSW(s,a,b);
  reg68.d[reg9] = s;
}

void line83C(u32 reg9, u32 reg0)
{
  /* DIVS.W <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eaw[4](reg0);
  a = read_W(addr)<<16;
  b = reg68.d[reg9]<<0;
  DIVSW(s,a,b);
  reg68.d[reg9] = s;
}

void line83D(u32 reg9, u32 reg0)
{
  /* DIVS.W <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eaw[5](reg0);
  a = read_W(addr)<<16;
  b = reg68.d[reg9]<<0;
  DIVSW(s,a,b);
  reg68.d[reg9] = s;
}

void line83E(u32 reg9, u32 reg0)
{
  /* DIVS.W <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eaw[6](reg0);
  a = read_W(addr)<<16;
  b = reg68.d[reg9]<<0;
  DIVSW(s,a,b);
  reg68.d[reg9] = s;
}

void line83F(u32 reg9, u32 reg0)
{
  /* DIVS.W <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eaw[7](reg0);
  a = read_W(addr)<<16;
  b = reg68.d[reg9]<<0;
  DIVSW(s,a,b);
  reg68.d[reg9] = s;
}

