void line300(u32 reg9, u32 reg0)
{
  /* MOVE.W Dn,Dn */
  s32 a;
  a = reg68.d[reg0]<<16;
  reg68.d[reg9] = (reg68.d[reg9]&0xffff0000) + ((u32)a>>16);
  MOVE(a);
}

void line301(u32 reg9, u32 reg0)
{
  /* MOVE.W An,Dn */
  s32 a;
  a = reg68.a[reg0]<<16;
  reg68.d[reg9] = (reg68.d[reg9]&0xffff0000) + ((u32)a>>16);
  MOVE(a);
}

void line302(u32 reg9, u32 reg0)
{
  /* MOVE.W <Ae>,Dn */
  s32 a;
  u32 addr;
  addr = get_eaw[2](reg0);
  a = read_W(addr)<<16;
  reg68.d[reg9] = (reg68.d[reg9]&0xffff0000) + ((u32)a>>16);
  MOVE(a);
}

void line303(u32 reg9, u32 reg0)
{
  /* MOVE.W <Ae>,Dn */
  s32 a;
  u32 addr;
  addr = get_eaw[3](reg0);
  a = read_W(addr)<<16;
  reg68.d[reg9] = (reg68.d[reg9]&0xffff0000) + ((u32)a>>16);
  MOVE(a);
}

void line304(u32 reg9, u32 reg0)
{
  /* MOVE.W <Ae>,Dn */
  s32 a;
  u32 addr;
  addr = get_eaw[4](reg0);
  a = read_W(addr)<<16;
  reg68.d[reg9] = (reg68.d[reg9]&0xffff0000) + ((u32)a>>16);
  MOVE(a);
}

void line305(u32 reg9, u32 reg0)
{
  /* MOVE.W <Ae>,Dn */
  s32 a;
  u32 addr;
  addr = get_eaw[5](reg0);
  a = read_W(addr)<<16;
  reg68.d[reg9] = (reg68.d[reg9]&0xffff0000) + ((u32)a>>16);
  MOVE(a);
}

void line306(u32 reg9, u32 reg0)
{
  /* MOVE.W <Ae>,Dn */
  s32 a;
  u32 addr;
  addr = get_eaw[6](reg0);
  a = read_W(addr)<<16;
  reg68.d[reg9] = (reg68.d[reg9]&0xffff0000) + ((u32)a>>16);
  MOVE(a);
}

void line307(u32 reg9, u32 reg0)
{
  /* MOVE.W <Ae>,Dn */
  s32 a;
  u32 addr;
  addr = get_eaw[7](reg0);
  a = read_W(addr)<<16;
  reg68.d[reg9] = (reg68.d[reg9]&0xffff0000) + ((u32)a>>16);
  MOVE(a);
}

void line308(u32 reg9, u32 reg0)
{
  /* MOVE.W Dn,An */
  s32 a;
  a = reg68.d[reg0]<<16;
  reg68.a[reg9] = a>>16;
}

void line309(u32 reg9, u32 reg0)
{
  /* MOVE.W An,An */
  s32 a;
  a = reg68.a[reg0]<<16;
  reg68.a[reg9] = a>>16;
}

void line30A(u32 reg9, u32 reg0)
{
  /* MOVE.W <Ae>,An */
  s32 a;
  u32 addr;
  addr = get_eaw[2](reg0);
  a = read_W(addr)<<16;
  reg68.a[reg9] = a>>16;
}

void line30B(u32 reg9, u32 reg0)
{
  /* MOVE.W <Ae>,An */
  s32 a;
  u32 addr;
  addr = get_eaw[3](reg0);
  a = read_W(addr)<<16;
  reg68.a[reg9] = a>>16;
}

void line30C(u32 reg9, u32 reg0)
{
  /* MOVE.W <Ae>,An */
  s32 a;
  u32 addr;
  addr = get_eaw[4](reg0);
  a = read_W(addr)<<16;
  reg68.a[reg9] = a>>16;
}

void line30D(u32 reg9, u32 reg0)
{
  /* MOVE.W <Ae>,An */
  s32 a;
  u32 addr;
  addr = get_eaw[5](reg0);
  a = read_W(addr)<<16;
  reg68.a[reg9] = a>>16;
}

void line30E(u32 reg9, u32 reg0)
{
  /* MOVE.W <Ae>,An */
  s32 a;
  u32 addr;
  addr = get_eaw[6](reg0);
  a = read_W(addr)<<16;
  reg68.a[reg9] = a>>16;
}

void line30F(u32 reg9, u32 reg0)
{
  /* MOVE.W <Ae>,An */
  s32 a;
  u32 addr;
  addr = get_eaw[7](reg0);
  a = read_W(addr)<<16;
  reg68.a[reg9] = a>>16;
}

void line310(u32 reg9, u32 reg0)
{
  /* MOVE.W Dn,<Ae> */
  s32 a;
  u32 addr;
  a = reg68.d[reg0]<<16;
  addr = get_eaw[2](reg9);
  write_W(addr,(u32)a>>16);
  MOVE(a);
}

void line311(u32 reg9, u32 reg0)
{
  /* MOVE.W An,<Ae> */
  s32 a;
  u32 addr;
  a = reg68.a[reg0]<<16;
  addr = get_eaw[2](reg9);
  write_W(addr,(u32)a>>16);
  MOVE(a);
}

void line312(u32 reg9, u32 reg0)
{
  /* MOVE.W <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eaw[2](reg0);
  a = read_W(addr)<<16;
  addr = get_eaw[2](reg9);
  write_W(addr,(u32)a>>16);
  MOVE(a);
}

void line313(u32 reg9, u32 reg0)
{
  /* MOVE.W <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eaw[3](reg0);
  a = read_W(addr)<<16;
  addr = get_eaw[2](reg9);
  write_W(addr,(u32)a>>16);
  MOVE(a);
}

void line314(u32 reg9, u32 reg0)
{
  /* MOVE.W <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eaw[4](reg0);
  a = read_W(addr)<<16;
  addr = get_eaw[2](reg9);
  write_W(addr,(u32)a>>16);
  MOVE(a);
}

void line315(u32 reg9, u32 reg0)
{
  /* MOVE.W <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eaw[5](reg0);
  a = read_W(addr)<<16;
  addr = get_eaw[2](reg9);
  write_W(addr,(u32)a>>16);
  MOVE(a);
}

void line316(u32 reg9, u32 reg0)
{
  /* MOVE.W <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eaw[6](reg0);
  a = read_W(addr)<<16;
  addr = get_eaw[2](reg9);
  write_W(addr,(u32)a>>16);
  MOVE(a);
}

void line317(u32 reg9, u32 reg0)
{
  /* MOVE.W <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eaw[7](reg0);
  a = read_W(addr)<<16;
  addr = get_eaw[2](reg9);
  write_W(addr,(u32)a>>16);
  MOVE(a);
}

void line318(u32 reg9, u32 reg0)
{
  /* MOVE.W Dn,<Ae> */
  s32 a;
  u32 addr;
  a = reg68.d[reg0]<<16;
  addr = get_eaw[3](reg9);
  write_W(addr,(u32)a>>16);
  MOVE(a);
}

void line319(u32 reg9, u32 reg0)
{
  /* MOVE.W An,<Ae> */
  s32 a;
  u32 addr;
  a = reg68.a[reg0]<<16;
  addr = get_eaw[3](reg9);
  write_W(addr,(u32)a>>16);
  MOVE(a);
}

void line31A(u32 reg9, u32 reg0)
{
  /* MOVE.W <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eaw[2](reg0);
  a = read_W(addr)<<16;
  addr = get_eaw[3](reg9);
  write_W(addr,(u32)a>>16);
  MOVE(a);
}

void line31B(u32 reg9, u32 reg0)
{
  /* MOVE.W <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eaw[3](reg0);
  a = read_W(addr)<<16;
  addr = get_eaw[3](reg9);
  write_W(addr,(u32)a>>16);
  MOVE(a);
}

void line31C(u32 reg9, u32 reg0)
{
  /* MOVE.W <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eaw[4](reg0);
  a = read_W(addr)<<16;
  addr = get_eaw[3](reg9);
  write_W(addr,(u32)a>>16);
  MOVE(a);
}

void line31D(u32 reg9, u32 reg0)
{
  /* MOVE.W <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eaw[5](reg0);
  a = read_W(addr)<<16;
  addr = get_eaw[3](reg9);
  write_W(addr,(u32)a>>16);
  MOVE(a);
}

void line31E(u32 reg9, u32 reg0)
{
  /* MOVE.W <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eaw[6](reg0);
  a = read_W(addr)<<16;
  addr = get_eaw[3](reg9);
  write_W(addr,(u32)a>>16);
  MOVE(a);
}

void line31F(u32 reg9, u32 reg0)
{
  /* MOVE.W <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eaw[7](reg0);
  a = read_W(addr)<<16;
  addr = get_eaw[3](reg9);
  write_W(addr,(u32)a>>16);
  MOVE(a);
}

void line320(u32 reg9, u32 reg0)
{
  /* MOVE.W Dn,<Ae> */
  s32 a;
  u32 addr;
  a = reg68.d[reg0]<<16;
  addr = get_eaw[4](reg9);
  write_W(addr,(u32)a>>16);
  MOVE(a);
}

void line321(u32 reg9, u32 reg0)
{
  /* MOVE.W An,<Ae> */
  s32 a;
  u32 addr;
  a = reg68.a[reg0]<<16;
  addr = get_eaw[4](reg9);
  write_W(addr,(u32)a>>16);
  MOVE(a);
}

void line322(u32 reg9, u32 reg0)
{
  /* MOVE.W <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eaw[2](reg0);
  a = read_W(addr)<<16;
  addr = get_eaw[4](reg9);
  write_W(addr,(u32)a>>16);
  MOVE(a);
}

void line323(u32 reg9, u32 reg0)
{
  /* MOVE.W <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eaw[3](reg0);
  a = read_W(addr)<<16;
  addr = get_eaw[4](reg9);
  write_W(addr,(u32)a>>16);
  MOVE(a);
}

void line324(u32 reg9, u32 reg0)
{
  /* MOVE.W <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eaw[4](reg0);
  a = read_W(addr)<<16;
  addr = get_eaw[4](reg9);
  write_W(addr,(u32)a>>16);
  MOVE(a);
}

void line325(u32 reg9, u32 reg0)
{
  /* MOVE.W <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eaw[5](reg0);
  a = read_W(addr)<<16;
  addr = get_eaw[4](reg9);
  write_W(addr,(u32)a>>16);
  MOVE(a);
}

void line326(u32 reg9, u32 reg0)
{
  /* MOVE.W <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eaw[6](reg0);
  a = read_W(addr)<<16;
  addr = get_eaw[4](reg9);
  write_W(addr,(u32)a>>16);
  MOVE(a);
}

void line327(u32 reg9, u32 reg0)
{
  /* MOVE.W <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eaw[7](reg0);
  a = read_W(addr)<<16;
  addr = get_eaw[4](reg9);
  write_W(addr,(u32)a>>16);
  MOVE(a);
}

void line328(u32 reg9, u32 reg0)
{
  /* MOVE.W Dn,<Ae> */
  s32 a;
  u32 addr;
  a = reg68.d[reg0]<<16;
  addr = get_eaw[5](reg9);
  write_W(addr,(u32)a>>16);
  MOVE(a);
}

void line329(u32 reg9, u32 reg0)
{
  /* MOVE.W An,<Ae> */
  s32 a;
  u32 addr;
  a = reg68.a[reg0]<<16;
  addr = get_eaw[5](reg9);
  write_W(addr,(u32)a>>16);
  MOVE(a);
}

void line32A(u32 reg9, u32 reg0)
{
  /* MOVE.W <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eaw[2](reg0);
  a = read_W(addr)<<16;
  addr = get_eaw[5](reg9);
  write_W(addr,(u32)a>>16);
  MOVE(a);
}

void line32B(u32 reg9, u32 reg0)
{
  /* MOVE.W <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eaw[3](reg0);
  a = read_W(addr)<<16;
  addr = get_eaw[5](reg9);
  write_W(addr,(u32)a>>16);
  MOVE(a);
}

void line32C(u32 reg9, u32 reg0)
{
  /* MOVE.W <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eaw[4](reg0);
  a = read_W(addr)<<16;
  addr = get_eaw[5](reg9);
  write_W(addr,(u32)a>>16);
  MOVE(a);
}

void line32D(u32 reg9, u32 reg0)
{
  /* MOVE.W <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eaw[5](reg0);
  a = read_W(addr)<<16;
  addr = get_eaw[5](reg9);
  write_W(addr,(u32)a>>16);
  MOVE(a);
}

void line32E(u32 reg9, u32 reg0)
{
  /* MOVE.W <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eaw[6](reg0);
  a = read_W(addr)<<16;
  addr = get_eaw[5](reg9);
  write_W(addr,(u32)a>>16);
  MOVE(a);
}

void line32F(u32 reg9, u32 reg0)
{
  /* MOVE.W <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eaw[7](reg0);
  a = read_W(addr)<<16;
  addr = get_eaw[5](reg9);
  write_W(addr,(u32)a>>16);
  MOVE(a);
}

void line330(u32 reg9, u32 reg0)
{
  /* MOVE.W Dn,<Ae> */
  s32 a;
  u32 addr;
  a = reg68.d[reg0]<<16;
  addr = get_eaw[6](reg9);
  write_W(addr,(u32)a>>16);
  MOVE(a);
}

void line331(u32 reg9, u32 reg0)
{
  /* MOVE.W An,<Ae> */
  s32 a;
  u32 addr;
  a = reg68.a[reg0]<<16;
  addr = get_eaw[6](reg9);
  write_W(addr,(u32)a>>16);
  MOVE(a);
}

void line332(u32 reg9, u32 reg0)
{
  /* MOVE.W <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eaw[2](reg0);
  a = read_W(addr)<<16;
  addr = get_eaw[6](reg9);
  write_W(addr,(u32)a>>16);
  MOVE(a);
}

void line333(u32 reg9, u32 reg0)
{
  /* MOVE.W <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eaw[3](reg0);
  a = read_W(addr)<<16;
  addr = get_eaw[6](reg9);
  write_W(addr,(u32)a>>16);
  MOVE(a);
}

void line334(u32 reg9, u32 reg0)
{
  /* MOVE.W <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eaw[4](reg0);
  a = read_W(addr)<<16;
  addr = get_eaw[6](reg9);
  write_W(addr,(u32)a>>16);
  MOVE(a);
}

void line335(u32 reg9, u32 reg0)
{
  /* MOVE.W <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eaw[5](reg0);
  a = read_W(addr)<<16;
  addr = get_eaw[6](reg9);
  write_W(addr,(u32)a>>16);
  MOVE(a);
}

void line336(u32 reg9, u32 reg0)
{
  /* MOVE.W <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eaw[6](reg0);
  a = read_W(addr)<<16;
  addr = get_eaw[6](reg9);
  write_W(addr,(u32)a>>16);
  MOVE(a);
}

void line337(u32 reg9, u32 reg0)
{
  /* MOVE.W <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eaw[7](reg0);
  a = read_W(addr)<<16;
  addr = get_eaw[6](reg9);
  write_W(addr,(u32)a>>16);
  MOVE(a);
}

void line338(u32 reg9, u32 reg0)
{
  /* MOVE.W Dn,<Ae> */
  s32 a;
  u32 addr;
  a = reg68.d[reg0]<<16;
  addr = get_eaw[7](reg9);
  write_W(addr,(u32)a>>16);
  MOVE(a);
}

void line339(u32 reg9, u32 reg0)
{
  /* MOVE.W An,<Ae> */
  s32 a;
  u32 addr;
  a = reg68.a[reg0]<<16;
  addr = get_eaw[7](reg9);
  write_W(addr,(u32)a>>16);
  MOVE(a);
}

void line33A(u32 reg9, u32 reg0)
{
  /* MOVE.W <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eaw[2](reg0);
  a = read_W(addr)<<16;
  addr = get_eaw[7](reg9);
  write_W(addr,(u32)a>>16);
  MOVE(a);
}

void line33B(u32 reg9, u32 reg0)
{
  /* MOVE.W <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eaw[3](reg0);
  a = read_W(addr)<<16;
  addr = get_eaw[7](reg9);
  write_W(addr,(u32)a>>16);
  MOVE(a);
}

void line33C(u32 reg9, u32 reg0)
{
  /* MOVE.W <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eaw[4](reg0);
  a = read_W(addr)<<16;
  addr = get_eaw[7](reg9);
  write_W(addr,(u32)a>>16);
  MOVE(a);
}

void line33D(u32 reg9, u32 reg0)
{
  /* MOVE.W <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eaw[5](reg0);
  a = read_W(addr)<<16;
  addr = get_eaw[7](reg9);
  write_W(addr,(u32)a>>16);
  MOVE(a);
}

void line33E(u32 reg9, u32 reg0)
{
  /* MOVE.W <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eaw[6](reg0);
  a = read_W(addr)<<16;
  addr = get_eaw[7](reg9);
  write_W(addr,(u32)a>>16);
  MOVE(a);
}

void line33F(u32 reg9, u32 reg0)
{
  /* MOVE.W <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eaw[7](reg0);
  a = read_W(addr)<<16;
  addr = get_eaw[7](reg9);
  write_W(addr,(u32)a>>16);
  MOVE(a);
}

