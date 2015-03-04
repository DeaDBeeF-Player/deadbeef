void line500(u32 reg9, u32 reg0)
{
  /* ADD.B #imm,Dy */
  s32 a,b,s;
  a = (((reg9-1)&7)+1)<<24;
  b = reg68.d[reg0]<<24;
  ADDB(s,a,b);
  reg68.d[reg0] = (reg68.d[reg0]&0xffffff00) + ((u32)s>>24);
}

void line501(u32 reg9, u32 reg0)
{
  ILLEGAL;
}

void line502(u32 reg9, u32 reg0)
{
  /* ADD.B #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = (((reg9-1)&7)+1)<<24;
  addr = get_eab[2](reg0);
  b = read_B(addr)<<24;
  ADDB(s,a,b);
  write_B(addr,(u32)s>>24);
}

void line503(u32 reg9, u32 reg0)
{
  /* ADD.B #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = (((reg9-1)&7)+1)<<24;
  addr = get_eab[3](reg0);
  b = read_B(addr)<<24;
  ADDB(s,a,b);
  write_B(addr,(u32)s>>24);
}

void line504(u32 reg9, u32 reg0)
{
  /* ADD.B #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = (((reg9-1)&7)+1)<<24;
  addr = get_eab[4](reg0);
  b = read_B(addr)<<24;
  ADDB(s,a,b);
  write_B(addr,(u32)s>>24);
}

void line505(u32 reg9, u32 reg0)
{
  /* ADD.B #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = (((reg9-1)&7)+1)<<24;
  addr = get_eab[5](reg0);
  b = read_B(addr)<<24;
  ADDB(s,a,b);
  write_B(addr,(u32)s>>24);
}

void line506(u32 reg9, u32 reg0)
{
  /* ADD.B #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = (((reg9-1)&7)+1)<<24;
  addr = get_eab[6](reg0);
  b = read_B(addr)<<24;
  ADDB(s,a,b);
  write_B(addr,(u32)s>>24);
}

void line507(u32 reg9, u32 reg0)
{
  /* ADD.B #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = (((reg9-1)&7)+1)<<24;
  addr = get_eab[7](reg0);
  b = read_B(addr)<<24;
  ADDB(s,a,b);
  write_B(addr,(u32)s>>24);
}

void line508(u32 reg9, u32 reg0)
{
  /* ADD.W #imm,Dy */
  s32 a,b,s;
  a = (((reg9-1)&7)+1)<<16;
  b = reg68.d[reg0]<<16;
  ADDW(s,a,b);
  reg68.d[reg0] = (reg68.d[reg0]&0xffff0000) + ((u32)s>>16);
}

void line509(u32 reg9, u32 reg0)
{
  /* ADD.W #imm,Ay */
  s32 a,b,s;
  a = (((reg9-1)&7)+1)<<16;
  b = reg68.a[reg0];
  ADDAW(s,a,b);
  reg68.a[reg0] = s;
}

void line50A(u32 reg9, u32 reg0)
{
  /* ADD.W #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = (((reg9-1)&7)+1)<<16;
  addr = get_eaw[2](reg0);
  b = read_W(addr)<<16;
  ADDW(s,a,b);
  write_W(addr,(u32)s>>16);
}

void line50B(u32 reg9, u32 reg0)
{
  /* ADD.W #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = (((reg9-1)&7)+1)<<16;
  addr = get_eaw[3](reg0);
  b = read_W(addr)<<16;
  ADDW(s,a,b);
  write_W(addr,(u32)s>>16);
}

void line50C(u32 reg9, u32 reg0)
{
  /* ADD.W #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = (((reg9-1)&7)+1)<<16;
  addr = get_eaw[4](reg0);
  b = read_W(addr)<<16;
  ADDW(s,a,b);
  write_W(addr,(u32)s>>16);
}

void line50D(u32 reg9, u32 reg0)
{
  /* ADD.W #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = (((reg9-1)&7)+1)<<16;
  addr = get_eaw[5](reg0);
  b = read_W(addr)<<16;
  ADDW(s,a,b);
  write_W(addr,(u32)s>>16);
}

void line50E(u32 reg9, u32 reg0)
{
  /* ADD.W #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = (((reg9-1)&7)+1)<<16;
  addr = get_eaw[6](reg0);
  b = read_W(addr)<<16;
  ADDW(s,a,b);
  write_W(addr,(u32)s>>16);
}

void line50F(u32 reg9, u32 reg0)
{
  /* ADD.W #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = (((reg9-1)&7)+1)<<16;
  addr = get_eaw[7](reg0);
  b = read_W(addr)<<16;
  ADDW(s,a,b);
  write_W(addr,(u32)s>>16);
}

void line510(u32 reg9, u32 reg0)
{
  /* ADD.L #imm,Dy */
  s32 a,b,s;
  a = (((reg9-1)&7)+1)<<0;
  b = reg68.d[reg0]<<0;
  ADDL(s,a,b);
  reg68.d[reg0] = (reg68.d[reg0]&0) + ((u32)s>>0);
}

void line511(u32 reg9, u32 reg0)
{
  /* ADD.L #imm,Ay */
  s32 a,b,s;
  a = (((reg9-1)&7)+1)<<0;
  b = reg68.a[reg0];
  ADDAL(s,a,b);
  reg68.a[reg0] = s;
}

void line512(u32 reg9, u32 reg0)
{
  /* ADD.L #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = (((reg9-1)&7)+1)<<0;
  addr = get_eal[2](reg0);
  b = read_L(addr)<<0;
  ADDL(s,a,b);
  write_L(addr,(u32)s>>0);
}

void line513(u32 reg9, u32 reg0)
{
  /* ADD.L #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = (((reg9-1)&7)+1)<<0;
  addr = get_eal[3](reg0);
  b = read_L(addr)<<0;
  ADDL(s,a,b);
  write_L(addr,(u32)s>>0);
}

void line514(u32 reg9, u32 reg0)
{
  /* ADD.L #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = (((reg9-1)&7)+1)<<0;
  addr = get_eal[4](reg0);
  b = read_L(addr)<<0;
  ADDL(s,a,b);
  write_L(addr,(u32)s>>0);
}

void line515(u32 reg9, u32 reg0)
{
  /* ADD.L #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = (((reg9-1)&7)+1)<<0;
  addr = get_eal[5](reg0);
  b = read_L(addr)<<0;
  ADDL(s,a,b);
  write_L(addr,(u32)s>>0);
}

void line516(u32 reg9, u32 reg0)
{
  /* ADD.L #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = (((reg9-1)&7)+1)<<0;
  addr = get_eal[6](reg0);
  b = read_L(addr)<<0;
  ADDL(s,a,b);
  write_L(addr,(u32)s>>0);
}

void line517(u32 reg9, u32 reg0)
{
  /* ADD.L #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = (((reg9-1)&7)+1)<<0;
  addr = get_eal[7](reg0);
  b = read_L(addr)<<0;
  ADDL(s,a,b);
  write_L(addr,(u32)s>>0);
}

void line518(u32 reg9, u32 reg0)
{
  /* Scc Dn */
  reg68.d[reg0] = (reg68.d[reg0]&0xFFFFFF00) + (u8)-((is_cc[reg9])()^1);
}

void line519(u32 reg9, u32 reg0)
{
  /* DBcc Dn,<adr> */
  if((is_cc[reg9])())
  {
    s32 a;
    a=reg68.d[reg0]-1;
    a&=0xFFFF;
    reg68.d[reg0] = (reg68.d[reg0]&0xFFFF0000) + a;
    if(a!=0xFFFF)
    {
      u32 pc=reg68.pc;
      reg68.pc = pc+get_nextw();
    }
    else reg68.pc += 2;
  }
  else reg68.pc += 2;
}

void line51A(u32 reg9, u32 reg0)
{
  /* Scc <AE> */
  u32 addr;
  addr = get_eab[2](reg0);
  write_B(addr,-((is_cc[reg9])()^1));
}

void line51B(u32 reg9, u32 reg0)
{
  /* Scc <AE> */
  u32 addr;
  addr = get_eab[3](reg0);
  write_B(addr,-((is_cc[reg9])()^1));
}

void line51C(u32 reg9, u32 reg0)
{
  /* Scc <AE> */
  u32 addr;
  addr = get_eab[4](reg0);
  write_B(addr,-((is_cc[reg9])()^1));
}

void line51D(u32 reg9, u32 reg0)
{
  /* Scc <AE> */
  u32 addr;
  addr = get_eab[5](reg0);
  write_B(addr,-((is_cc[reg9])()^1));
}

void line51E(u32 reg9, u32 reg0)
{
  /* Scc <AE> */
  u32 addr;
  addr = get_eab[6](reg0);
  write_B(addr,-((is_cc[reg9])()^1));
}

void line51F(u32 reg9, u32 reg0)
{
  /* Scc <AE> */
  u32 addr;
  addr = get_eab[7](reg0);
  write_B(addr,-((is_cc[reg9])()^1));
}

void line520(u32 reg9, u32 reg0)
{
  /* SUB.B #imm,Dy */
  s32 a,b,s;
  a = (((reg9-1)&7)+1)<<24;
  b = reg68.d[reg0]<<24;
  SUBB(s,a,b);
  reg68.d[reg0] = (reg68.d[reg0]&0xffffff00) + ((u32)s>>24);
}

void line521(u32 reg9, u32 reg0)
{
  ILLEGAL;
}

void line522(u32 reg9, u32 reg0)
{
  /* SUB.B #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = (((reg9-1)&7)+1)<<24;
  addr = get_eab[2](reg0);
  b = read_B(addr)<<24;
  SUBB(s,a,b);
  write_B(addr,(u32)s>>24);
}

void line523(u32 reg9, u32 reg0)
{
  /* SUB.B #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = (((reg9-1)&7)+1)<<24;
  addr = get_eab[3](reg0);
  b = read_B(addr)<<24;
  SUBB(s,a,b);
  write_B(addr,(u32)s>>24);
}

void line524(u32 reg9, u32 reg0)
{
  /* SUB.B #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = (((reg9-1)&7)+1)<<24;
  addr = get_eab[4](reg0);
  b = read_B(addr)<<24;
  SUBB(s,a,b);
  write_B(addr,(u32)s>>24);
}

void line525(u32 reg9, u32 reg0)
{
  /* SUB.B #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = (((reg9-1)&7)+1)<<24;
  addr = get_eab[5](reg0);
  b = read_B(addr)<<24;
  SUBB(s,a,b);
  write_B(addr,(u32)s>>24);
}

void line526(u32 reg9, u32 reg0)
{
  /* SUB.B #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = (((reg9-1)&7)+1)<<24;
  addr = get_eab[6](reg0);
  b = read_B(addr)<<24;
  SUBB(s,a,b);
  write_B(addr,(u32)s>>24);
}

void line527(u32 reg9, u32 reg0)
{
  /* SUB.B #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = (((reg9-1)&7)+1)<<24;
  addr = get_eab[7](reg0);
  b = read_B(addr)<<24;
  SUBB(s,a,b);
  write_B(addr,(u32)s>>24);
}

void line528(u32 reg9, u32 reg0)
{
  /* SUB.W #imm,Dy */
  s32 a,b,s;
  a = (((reg9-1)&7)+1)<<16;
  b = reg68.d[reg0]<<16;
  SUBW(s,a,b);
  reg68.d[reg0] = (reg68.d[reg0]&0xffff0000) + ((u32)s>>16);
}

void line529(u32 reg9, u32 reg0)
{
  /* SUB.W #imm,Ay */
  s32 a,b,s;
  a = (((reg9-1)&7)+1)<<16;
  b = reg68.a[reg0];
  SUBAW(s,a,b);
  reg68.a[reg0] = s;
}

void line52A(u32 reg9, u32 reg0)
{
  /* SUB.W #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = (((reg9-1)&7)+1)<<16;
  addr = get_eaw[2](reg0);
  b = read_W(addr)<<16;
  SUBW(s,a,b);
  write_W(addr,(u32)s>>16);
}

void line52B(u32 reg9, u32 reg0)
{
  /* SUB.W #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = (((reg9-1)&7)+1)<<16;
  addr = get_eaw[3](reg0);
  b = read_W(addr)<<16;
  SUBW(s,a,b);
  write_W(addr,(u32)s>>16);
}

void line52C(u32 reg9, u32 reg0)
{
  /* SUB.W #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = (((reg9-1)&7)+1)<<16;
  addr = get_eaw[4](reg0);
  b = read_W(addr)<<16;
  SUBW(s,a,b);
  write_W(addr,(u32)s>>16);
}

void line52D(u32 reg9, u32 reg0)
{
  /* SUB.W #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = (((reg9-1)&7)+1)<<16;
  addr = get_eaw[5](reg0);
  b = read_W(addr)<<16;
  SUBW(s,a,b);
  write_W(addr,(u32)s>>16);
}

void line52E(u32 reg9, u32 reg0)
{
  /* SUB.W #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = (((reg9-1)&7)+1)<<16;
  addr = get_eaw[6](reg0);
  b = read_W(addr)<<16;
  SUBW(s,a,b);
  write_W(addr,(u32)s>>16);
}

void line52F(u32 reg9, u32 reg0)
{
  /* SUB.W #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = (((reg9-1)&7)+1)<<16;
  addr = get_eaw[7](reg0);
  b = read_W(addr)<<16;
  SUBW(s,a,b);
  write_W(addr,(u32)s>>16);
}

void line530(u32 reg9, u32 reg0)
{
  /* SUB.L #imm,Dy */
  s32 a,b,s;
  a = (((reg9-1)&7)+1)<<0;
  b = reg68.d[reg0]<<0;
  SUBL(s,a,b);
  reg68.d[reg0] = (reg68.d[reg0]&0) + ((u32)s>>0);
}

void line531(u32 reg9, u32 reg0)
{
  /* SUB.L #imm,Ay */
  s32 a,b,s;
  a = (((reg9-1)&7)+1)<<0;
  b = reg68.a[reg0];
  SUBAL(s,a,b);
  reg68.a[reg0] = s;
}

void line532(u32 reg9, u32 reg0)
{
  /* SUB.L #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = (((reg9-1)&7)+1)<<0;
  addr = get_eal[2](reg0);
  b = read_L(addr)<<0;
  SUBL(s,a,b);
  write_L(addr,(u32)s>>0);
}

void line533(u32 reg9, u32 reg0)
{
  /* SUB.L #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = (((reg9-1)&7)+1)<<0;
  addr = get_eal[3](reg0);
  b = read_L(addr)<<0;
  SUBL(s,a,b);
  write_L(addr,(u32)s>>0);
}

void line534(u32 reg9, u32 reg0)
{
  /* SUB.L #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = (((reg9-1)&7)+1)<<0;
  addr = get_eal[4](reg0);
  b = read_L(addr)<<0;
  SUBL(s,a,b);
  write_L(addr,(u32)s>>0);
}

void line535(u32 reg9, u32 reg0)
{
  /* SUB.L #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = (((reg9-1)&7)+1)<<0;
  addr = get_eal[5](reg0);
  b = read_L(addr)<<0;
  SUBL(s,a,b);
  write_L(addr,(u32)s>>0);
}

void line536(u32 reg9, u32 reg0)
{
  /* SUB.L #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = (((reg9-1)&7)+1)<<0;
  addr = get_eal[6](reg0);
  b = read_L(addr)<<0;
  SUBL(s,a,b);
  write_L(addr,(u32)s>>0);
}

void line537(u32 reg9, u32 reg0)
{
  /* SUB.L #imm,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = (((reg9-1)&7)+1)<<0;
  addr = get_eal[7](reg0);
  b = read_L(addr)<<0;
  SUBL(s,a,b);
  write_L(addr,(u32)s>>0);
}

void line538(u32 reg9, u32 reg0)
{
  /* Scc Dn */
  reg68.d[reg0] = (reg68.d[reg0]&0xFFFFFF00) + (u8)-(is_cc[reg9])();
}

void line539(u32 reg9, u32 reg0)
{
  /* DBcc Dn,<adr> */
  if(!(is_cc[reg9])())
  {
    s32 a;
    a=reg68.d[reg0]-1;
    a&=0xFFFF;
    reg68.d[reg0] = (reg68.d[reg0]&0xFFFF0000) + a;
    if(a!=0xFFFF)
    {
      u32 pc=reg68.pc;
      reg68.pc = pc+get_nextw();
    }
    else reg68.pc += 2;
  }
  else reg68.pc += 2;
}

void line53A(u32 reg9, u32 reg0)
{
  /* Scc <AE> */
  u32 addr;
  addr = get_eab[2](reg0);
  write_B(addr,-(is_cc[reg9])());
}

void line53B(u32 reg9, u32 reg0)
{
  /* Scc <AE> */
  u32 addr;
  addr = get_eab[3](reg0);
  write_B(addr,-(is_cc[reg9])());
}

void line53C(u32 reg9, u32 reg0)
{
  /* Scc <AE> */
  u32 addr;
  addr = get_eab[4](reg0);
  write_B(addr,-(is_cc[reg9])());
}

void line53D(u32 reg9, u32 reg0)
{
  /* Scc <AE> */
  u32 addr;
  addr = get_eab[5](reg0);
  write_B(addr,-(is_cc[reg9])());
}

void line53E(u32 reg9, u32 reg0)
{
  /* Scc <AE> */
  u32 addr;
  addr = get_eab[6](reg0);
  write_B(addr,-(is_cc[reg9])());
}

void line53F(u32 reg9, u32 reg0)
{
  /* Scc <AE> */
  u32 addr;
  addr = get_eab[7](reg0);
  write_B(addr,-(is_cc[reg9])());
}

