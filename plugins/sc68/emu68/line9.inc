void line900(u32 reg9, u32 reg0)
{
  /* SUB.B Dx,Dy */
  s32 a,b,s;
  a = reg68.d[reg0]<<24;
  b = reg68.d[reg9]<<24;
  SUBB(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0xffffff00) + ((u32)s>>24);
}

void line901(u32 reg9, u32 reg0)
{
  /* SUB.B Ax,Dy */
  s32 a,b,s;
  a = reg68.a[reg0]<<24;
  b = reg68.d[reg9]<<24;
  SUBB(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0xffffff00) + ((u32)s>>24);
}

void line902(u32 reg9, u32 reg0)
{
  /* SUB.B <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eab[2](reg0);
  a = read_B(addr)<<24;
  b = reg68.d[reg9]<<24;
  SUBB(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0xffffff00) + ((u32)s>>24);
}

void line903(u32 reg9, u32 reg0)
{
  /* SUB.B <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eab[3](reg0);
  a = read_B(addr)<<24;
  b = reg68.d[reg9]<<24;
  SUBB(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0xffffff00) + ((u32)s>>24);
}

void line904(u32 reg9, u32 reg0)
{
  /* SUB.B <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eab[4](reg0);
  a = read_B(addr)<<24;
  b = reg68.d[reg9]<<24;
  SUBB(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0xffffff00) + ((u32)s>>24);
}

void line905(u32 reg9, u32 reg0)
{
  /* SUB.B <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eab[5](reg0);
  a = read_B(addr)<<24;
  b = reg68.d[reg9]<<24;
  SUBB(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0xffffff00) + ((u32)s>>24);
}

void line906(u32 reg9, u32 reg0)
{
  /* SUB.B <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eab[6](reg0);
  a = read_B(addr)<<24;
  b = reg68.d[reg9]<<24;
  SUBB(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0xffffff00) + ((u32)s>>24);
}

void line907(u32 reg9, u32 reg0)
{
  /* SUB.B <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eab[7](reg0);
  a = read_B(addr)<<24;
  b = reg68.d[reg9]<<24;
  SUBB(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0xffffff00) + ((u32)s>>24);
}

void line908(u32 reg9, u32 reg0)
{
  /* SUB.W Dx,Dy */
  s32 a,b,s;
  a = reg68.d[reg0]<<16;
  b = reg68.d[reg9]<<16;
  SUBW(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0xffff0000) + ((u32)s>>16);
}

void line909(u32 reg9, u32 reg0)
{
  /* SUB.W Ax,Dy */
  s32 a,b,s;
  a = reg68.a[reg0]<<16;
  b = reg68.d[reg9]<<16;
  SUBW(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0xffff0000) + ((u32)s>>16);
}

void line90A(u32 reg9, u32 reg0)
{
  /* SUB.W <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eaw[2](reg0);
  a = read_W(addr)<<16;
  b = reg68.d[reg9]<<16;
  SUBW(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0xffff0000) + ((u32)s>>16);
}

void line90B(u32 reg9, u32 reg0)
{
  /* SUB.W <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eaw[3](reg0);
  a = read_W(addr)<<16;
  b = reg68.d[reg9]<<16;
  SUBW(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0xffff0000) + ((u32)s>>16);
}

void line90C(u32 reg9, u32 reg0)
{
  /* SUB.W <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eaw[4](reg0);
  a = read_W(addr)<<16;
  b = reg68.d[reg9]<<16;
  SUBW(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0xffff0000) + ((u32)s>>16);
}

void line90D(u32 reg9, u32 reg0)
{
  /* SUB.W <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eaw[5](reg0);
  a = read_W(addr)<<16;
  b = reg68.d[reg9]<<16;
  SUBW(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0xffff0000) + ((u32)s>>16);
}

void line90E(u32 reg9, u32 reg0)
{
  /* SUB.W <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eaw[6](reg0);
  a = read_W(addr)<<16;
  b = reg68.d[reg9]<<16;
  SUBW(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0xffff0000) + ((u32)s>>16);
}

void line90F(u32 reg9, u32 reg0)
{
  /* SUB.W <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eaw[7](reg0);
  a = read_W(addr)<<16;
  b = reg68.d[reg9]<<16;
  SUBW(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0xffff0000) + ((u32)s>>16);
}

void line910(u32 reg9, u32 reg0)
{
  /* SUB.L Dx,Dy */
  s32 a,b,s;
  a = reg68.d[reg0]<<0;
  b = reg68.d[reg9]<<0;
  SUBL(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0) + ((u32)s>>0);
}

void line911(u32 reg9, u32 reg0)
{
  /* SUB.L Ax,Dy */
  s32 a,b,s;
  a = reg68.a[reg0]<<0;
  b = reg68.d[reg9]<<0;
  SUBL(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0) + ((u32)s>>0);
}

void line912(u32 reg9, u32 reg0)
{
  /* SUB.L <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eal[2](reg0);
  a = read_L(addr)<<0;
  b = reg68.d[reg9]<<0;
  SUBL(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0) + ((u32)s>>0);
}

void line913(u32 reg9, u32 reg0)
{
  /* SUB.L <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eal[3](reg0);
  a = read_L(addr)<<0;
  b = reg68.d[reg9]<<0;
  SUBL(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0) + ((u32)s>>0);
}

void line914(u32 reg9, u32 reg0)
{
  /* SUB.L <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eal[4](reg0);
  a = read_L(addr)<<0;
  b = reg68.d[reg9]<<0;
  SUBL(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0) + ((u32)s>>0);
}

void line915(u32 reg9, u32 reg0)
{
  /* SUB.L <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eal[5](reg0);
  a = read_L(addr)<<0;
  b = reg68.d[reg9]<<0;
  SUBL(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0) + ((u32)s>>0);
}

void line916(u32 reg9, u32 reg0)
{
  /* SUB.L <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eal[6](reg0);
  a = read_L(addr)<<0;
  b = reg68.d[reg9]<<0;
  SUBL(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0) + ((u32)s>>0);
}

void line917(u32 reg9, u32 reg0)
{
  /* SUB.L <Ae>,Dn */
  s32 a,b,s;
  u32 addr;
  addr = get_eal[7](reg0);
  a = read_L(addr)<<0;
  b = reg68.d[reg9]<<0;
  SUBL(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0) + ((u32)s>>0);
}

void line918(u32 reg9, u32 reg0)
{
  /* SUB.W Dx,Ay */
  s32 a,b,s;
  a = reg68.d[reg0]<<16;
  b = reg68.a[reg9];
  SUBAW(s,a,b);
  reg68.a[reg9] = s;
}

void line919(u32 reg9, u32 reg0)
{
  /* SUB.W Ax,Ay */
  s32 a,b,s;
  a = reg68.a[reg0]<<16;
  b = reg68.a[reg9];
  SUBAW(s,a,b);
  reg68.a[reg9] = s;
}

void line91A(u32 reg9, u32 reg0)
{
  /* SUB.W <Ae>,An */
  s32 a,b,s;
  u32 addr;
  addr = get_eaw[2](reg0);
  a = read_W(addr)<<16;
  b = reg68.a[reg9];
  SUBAW(s,a,b);
  reg68.a[reg9] = s;
}

void line91B(u32 reg9, u32 reg0)
{
  /* SUB.W <Ae>,An */
  s32 a,b,s;
  u32 addr;
  addr = get_eaw[3](reg0);
  a = read_W(addr)<<16;
  b = reg68.a[reg9];
  SUBAW(s,a,b);
  reg68.a[reg9] = s;
}

void line91C(u32 reg9, u32 reg0)
{
  /* SUB.W <Ae>,An */
  s32 a,b,s;
  u32 addr;
  addr = get_eaw[4](reg0);
  a = read_W(addr)<<16;
  b = reg68.a[reg9];
  SUBAW(s,a,b);
  reg68.a[reg9] = s;
}

void line91D(u32 reg9, u32 reg0)
{
  /* SUB.W <Ae>,An */
  s32 a,b,s;
  u32 addr;
  addr = get_eaw[5](reg0);
  a = read_W(addr)<<16;
  b = reg68.a[reg9];
  SUBAW(s,a,b);
  reg68.a[reg9] = s;
}

void line91E(u32 reg9, u32 reg0)
{
  /* SUB.W <Ae>,An */
  s32 a,b,s;
  u32 addr;
  addr = get_eaw[6](reg0);
  a = read_W(addr)<<16;
  b = reg68.a[reg9];
  SUBAW(s,a,b);
  reg68.a[reg9] = s;
}

void line91F(u32 reg9, u32 reg0)
{
  /* SUB.W <Ae>,An */
  s32 a,b,s;
  u32 addr;
  addr = get_eaw[7](reg0);
  a = read_W(addr)<<16;
  b = reg68.a[reg9];
  SUBAW(s,a,b);
  reg68.a[reg9] = s;
}

void line920(u32 reg9, u32 reg0)
{
  s32 a,b,s;
  a = reg68.d[reg0]<<24;
  b = reg68.d[reg9]<<24;
  SUBXB(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0xffffff00) + ((u32)s>>24);
}

void line921(u32 reg9, u32 reg0)
{
  s32 a,b,s;
  a = read_B(reg68.a[reg0]-=1)<<24;
  b = read_B(reg68.a[reg9]-=1)<<24;
  SUBXB(s,a,b);
  write_B(reg68.a[reg9],(u32)s>>24);
}

void line922(u32 reg9, u32 reg0)
{
  /* SUB.B Dn,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = reg68.d[reg9]<<24;
  addr = get_eab[2](reg0);
  b = read_B(addr)<<24;
  SUBB(s,a,b);
  write_B(addr,(u32)s>>24);
}

void line923(u32 reg9, u32 reg0)
{
  /* SUB.B Dn,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = reg68.d[reg9]<<24;
  addr = get_eab[3](reg0);
  b = read_B(addr)<<24;
  SUBB(s,a,b);
  write_B(addr,(u32)s>>24);
}

void line924(u32 reg9, u32 reg0)
{
  /* SUB.B Dn,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = reg68.d[reg9]<<24;
  addr = get_eab[4](reg0);
  b = read_B(addr)<<24;
  SUBB(s,a,b);
  write_B(addr,(u32)s>>24);
}

void line925(u32 reg9, u32 reg0)
{
  /* SUB.B Dn,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = reg68.d[reg9]<<24;
  addr = get_eab[5](reg0);
  b = read_B(addr)<<24;
  SUBB(s,a,b);
  write_B(addr,(u32)s>>24);
}

void line926(u32 reg9, u32 reg0)
{
  /* SUB.B Dn,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = reg68.d[reg9]<<24;
  addr = get_eab[6](reg0);
  b = read_B(addr)<<24;
  SUBB(s,a,b);
  write_B(addr,(u32)s>>24);
}

void line927(u32 reg9, u32 reg0)
{
  /* SUB.B Dn,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = reg68.d[reg9]<<24;
  addr = get_eab[7](reg0);
  b = read_B(addr)<<24;
  SUBB(s,a,b);
  write_B(addr,(u32)s>>24);
}

void line928(u32 reg9, u32 reg0)
{
  s32 a,b,s;
  a = reg68.d[reg0]<<16;
  b = reg68.d[reg9]<<16;
  SUBXW(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0xffff0000) + ((u32)s>>16);
}

void line929(u32 reg9, u32 reg0)
{
  s32 a,b,s;
  a = read_W(reg68.a[reg0]-=2)<<16;
  b = read_W(reg68.a[reg9]-=2)<<16;
  SUBXW(s,a,b);
  write_W(reg68.a[reg9],(u32)s>>16);
}

void line92A(u32 reg9, u32 reg0)
{
  /* SUB.W Dn,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = reg68.d[reg9]<<16;
  addr = get_eaw[2](reg0);
  b = read_W(addr)<<16;
  SUBW(s,a,b);
  write_W(addr,(u32)s>>16);
}

void line92B(u32 reg9, u32 reg0)
{
  /* SUB.W Dn,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = reg68.d[reg9]<<16;
  addr = get_eaw[3](reg0);
  b = read_W(addr)<<16;
  SUBW(s,a,b);
  write_W(addr,(u32)s>>16);
}

void line92C(u32 reg9, u32 reg0)
{
  /* SUB.W Dn,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = reg68.d[reg9]<<16;
  addr = get_eaw[4](reg0);
  b = read_W(addr)<<16;
  SUBW(s,a,b);
  write_W(addr,(u32)s>>16);
}

void line92D(u32 reg9, u32 reg0)
{
  /* SUB.W Dn,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = reg68.d[reg9]<<16;
  addr = get_eaw[5](reg0);
  b = read_W(addr)<<16;
  SUBW(s,a,b);
  write_W(addr,(u32)s>>16);
}

void line92E(u32 reg9, u32 reg0)
{
  /* SUB.W Dn,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = reg68.d[reg9]<<16;
  addr = get_eaw[6](reg0);
  b = read_W(addr)<<16;
  SUBW(s,a,b);
  write_W(addr,(u32)s>>16);
}

void line92F(u32 reg9, u32 reg0)
{
  /* SUB.W Dn,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = reg68.d[reg9]<<16;
  addr = get_eaw[7](reg0);
  b = read_W(addr)<<16;
  SUBW(s,a,b);
  write_W(addr,(u32)s>>16);
}

void line930(u32 reg9, u32 reg0)
{
  s32 a,b,s;
  a = reg68.d[reg0]<<0;
  b = reg68.d[reg9]<<0;
  SUBXL(s,a,b);
  reg68.d[reg9] = (reg68.d[reg9]&0) + ((u32)s>>0);
}

void line931(u32 reg9, u32 reg0)
{
  s32 a,b,s;
  a = read_L(reg68.a[reg0]-=4)<<0;
  b = read_L(reg68.a[reg9]-=4)<<0;
  SUBXL(s,a,b);
  write_L(reg68.a[reg9],(u32)s>>0);
}

void line932(u32 reg9, u32 reg0)
{
  /* SUB.L Dn,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = reg68.d[reg9]<<0;
  addr = get_eal[2](reg0);
  b = read_L(addr)<<0;
  SUBL(s,a,b);
  write_L(addr,(u32)s>>0);
}

void line933(u32 reg9, u32 reg0)
{
  /* SUB.L Dn,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = reg68.d[reg9]<<0;
  addr = get_eal[3](reg0);
  b = read_L(addr)<<0;
  SUBL(s,a,b);
  write_L(addr,(u32)s>>0);
}

void line934(u32 reg9, u32 reg0)
{
  /* SUB.L Dn,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = reg68.d[reg9]<<0;
  addr = get_eal[4](reg0);
  b = read_L(addr)<<0;
  SUBL(s,a,b);
  write_L(addr,(u32)s>>0);
}

void line935(u32 reg9, u32 reg0)
{
  /* SUB.L Dn,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = reg68.d[reg9]<<0;
  addr = get_eal[5](reg0);
  b = read_L(addr)<<0;
  SUBL(s,a,b);
  write_L(addr,(u32)s>>0);
}

void line936(u32 reg9, u32 reg0)
{
  /* SUB.L Dn,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = reg68.d[reg9]<<0;
  addr = get_eal[6](reg0);
  b = read_L(addr)<<0;
  SUBL(s,a,b);
  write_L(addr,(u32)s>>0);
}

void line937(u32 reg9, u32 reg0)
{
  /* SUB.L Dn,<Ae> */
  s32 a,b,s;
  u32 addr;
  a = reg68.d[reg9]<<0;
  addr = get_eal[7](reg0);
  b = read_L(addr)<<0;
  SUBL(s,a,b);
  write_L(addr,(u32)s>>0);
}

void line938(u32 reg9, u32 reg0)
{
  /* SUB.L Dx,Ay */
  s32 a,b,s;
  a = reg68.d[reg0]<<0;
  b = reg68.a[reg9];
  SUBAL(s,a,b);
  reg68.a[reg9] = s;
}

void line939(u32 reg9, u32 reg0)
{
  /* SUB.L Ax,Ay */
  s32 a,b,s;
  a = reg68.a[reg0]<<0;
  b = reg68.a[reg9];
  SUBAL(s,a,b);
  reg68.a[reg9] = s;
}

void line93A(u32 reg9, u32 reg0)
{
  /* SUB.L <Ae>,An */
  s32 a,b,s;
  u32 addr;
  addr = get_eal[2](reg0);
  a = read_L(addr)<<0;
  b = reg68.a[reg9];
  SUBAL(s,a,b);
  reg68.a[reg9] = s;
}

void line93B(u32 reg9, u32 reg0)
{
  /* SUB.L <Ae>,An */
  s32 a,b,s;
  u32 addr;
  addr = get_eal[3](reg0);
  a = read_L(addr)<<0;
  b = reg68.a[reg9];
  SUBAL(s,a,b);
  reg68.a[reg9] = s;
}

void line93C(u32 reg9, u32 reg0)
{
  /* SUB.L <Ae>,An */
  s32 a,b,s;
  u32 addr;
  addr = get_eal[4](reg0);
  a = read_L(addr)<<0;
  b = reg68.a[reg9];
  SUBAL(s,a,b);
  reg68.a[reg9] = s;
}

void line93D(u32 reg9, u32 reg0)
{
  /* SUB.L <Ae>,An */
  s32 a,b,s;
  u32 addr;
  addr = get_eal[5](reg0);
  a = read_L(addr)<<0;
  b = reg68.a[reg9];
  SUBAL(s,a,b);
  reg68.a[reg9] = s;
}

void line93E(u32 reg9, u32 reg0)
{
  /* SUB.L <Ae>,An */
  s32 a,b,s;
  u32 addr;
  addr = get_eal[6](reg0);
  a = read_L(addr)<<0;
  b = reg68.a[reg9];
  SUBAL(s,a,b);
  reg68.a[reg9] = s;
}

void line93F(u32 reg9, u32 reg0)
{
  /* SUB.L <Ae>,An */
  s32 a,b,s;
  u32 addr;
  addr = get_eal[7](reg0);
  a = read_L(addr)<<0;
  b = reg68.a[reg9];
  SUBAL(s,a,b);
  reg68.a[reg9] = s;
}

