void line200(u32 reg9, u32 reg0)
{
  /* MOVE.L Dn,Dn */
  s32 a;
  a = reg68.d[reg0]<<0;
  reg68.d[reg9] = (reg68.d[reg9]&0) + ((u32)a>>0);
  MOVE(a);
}

void line201(u32 reg9, u32 reg0)
{
  /* MOVE.L An,Dn */
  s32 a;
  a = reg68.a[reg0]<<0;
  reg68.d[reg9] = (reg68.d[reg9]&0) + ((u32)a>>0);
  MOVE(a);
}

void line202(u32 reg9, u32 reg0)
{
  /* MOVE.L <Ae>,Dn */
  s32 a;
  u32 addr;
  addr = get_eal[2](reg0);
  a = read_L(addr)<<0;
  reg68.d[reg9] = (reg68.d[reg9]&0) + ((u32)a>>0);
  MOVE(a);
}

void line203(u32 reg9, u32 reg0)
{
  /* MOVE.L <Ae>,Dn */
  s32 a;
  u32 addr;
  addr = get_eal[3](reg0);
  a = read_L(addr)<<0;
  reg68.d[reg9] = (reg68.d[reg9]&0) + ((u32)a>>0);
  MOVE(a);
}

void line204(u32 reg9, u32 reg0)
{
  /* MOVE.L <Ae>,Dn */
  s32 a;
  u32 addr;
  addr = get_eal[4](reg0);
  a = read_L(addr)<<0;
  reg68.d[reg9] = (reg68.d[reg9]&0) + ((u32)a>>0);
  MOVE(a);
}

void line205(u32 reg9, u32 reg0)
{
  /* MOVE.L <Ae>,Dn */
  s32 a;
  u32 addr;
  addr = get_eal[5](reg0);
  a = read_L(addr)<<0;
  reg68.d[reg9] = (reg68.d[reg9]&0) + ((u32)a>>0);
  MOVE(a);
}

void line206(u32 reg9, u32 reg0)
{
  /* MOVE.L <Ae>,Dn */
  s32 a;
  u32 addr;
  addr = get_eal[6](reg0);
  a = read_L(addr)<<0;
  reg68.d[reg9] = (reg68.d[reg9]&0) + ((u32)a>>0);
  MOVE(a);
}

void line207(u32 reg9, u32 reg0)
{
  /* MOVE.L <Ae>,Dn */
  s32 a;
  u32 addr;
  addr = get_eal[7](reg0);
  a = read_L(addr)<<0;
  reg68.d[reg9] = (reg68.d[reg9]&0) + ((u32)a>>0);
  MOVE(a);
}

void line208(u32 reg9, u32 reg0)
{
  /* MOVE.L Dn,An */
  s32 a;
  a = reg68.d[reg0]<<0;
  reg68.a[reg9] = a>>0;
}

void line209(u32 reg9, u32 reg0)
{
  /* MOVE.L An,An */
  s32 a;
  a = reg68.a[reg0]<<0;
  reg68.a[reg9] = a>>0;
}

void line20A(u32 reg9, u32 reg0)
{
  /* MOVE.L <Ae>,An */
  s32 a;
  u32 addr;
  addr = get_eal[2](reg0);
  a = read_L(addr)<<0;
  reg68.a[reg9] = a>>0;
}

void line20B(u32 reg9, u32 reg0)
{
  /* MOVE.L <Ae>,An */
  s32 a;
  u32 addr;
  addr = get_eal[3](reg0);
  a = read_L(addr)<<0;
  reg68.a[reg9] = a>>0;
}

void line20C(u32 reg9, u32 reg0)
{
  /* MOVE.L <Ae>,An */
  s32 a;
  u32 addr;
  addr = get_eal[4](reg0);
  a = read_L(addr)<<0;
  reg68.a[reg9] = a>>0;
}

void line20D(u32 reg9, u32 reg0)
{
  /* MOVE.L <Ae>,An */
  s32 a;
  u32 addr;
  addr = get_eal[5](reg0);
  a = read_L(addr)<<0;
  reg68.a[reg9] = a>>0;
}

void line20E(u32 reg9, u32 reg0)
{
  /* MOVE.L <Ae>,An */
  s32 a;
  u32 addr;
  addr = get_eal[6](reg0);
  a = read_L(addr)<<0;
  reg68.a[reg9] = a>>0;
}

void line20F(u32 reg9, u32 reg0)
{
  /* MOVE.L <Ae>,An */
  s32 a;
  u32 addr;
  addr = get_eal[7](reg0);
  a = read_L(addr)<<0;
  reg68.a[reg9] = a>>0;
}

void line210(u32 reg9, u32 reg0)
{
  /* MOVE.L Dn,<Ae> */
  s32 a;
  u32 addr;
  a = reg68.d[reg0]<<0;
  addr = get_eal[2](reg9);
  write_L(addr,(u32)a>>0);
  MOVE(a);
}

void line211(u32 reg9, u32 reg0)
{
  /* MOVE.L An,<Ae> */
  s32 a;
  u32 addr;
  a = reg68.a[reg0]<<0;
  addr = get_eal[2](reg9);
  write_L(addr,(u32)a>>0);
  MOVE(a);
}

void line212(u32 reg9, u32 reg0)
{
  /* MOVE.L <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eal[2](reg0);
  a = read_L(addr)<<0;
  addr = get_eal[2](reg9);
  write_L(addr,(u32)a>>0);
  MOVE(a);
}

void line213(u32 reg9, u32 reg0)
{
  /* MOVE.L <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eal[3](reg0);
  a = read_L(addr)<<0;
  addr = get_eal[2](reg9);
  write_L(addr,(u32)a>>0);
  MOVE(a);
}

void line214(u32 reg9, u32 reg0)
{
  /* MOVE.L <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eal[4](reg0);
  a = read_L(addr)<<0;
  addr = get_eal[2](reg9);
  write_L(addr,(u32)a>>0);
  MOVE(a);
}

void line215(u32 reg9, u32 reg0)
{
  /* MOVE.L <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eal[5](reg0);
  a = read_L(addr)<<0;
  addr = get_eal[2](reg9);
  write_L(addr,(u32)a>>0);
  MOVE(a);
}

void line216(u32 reg9, u32 reg0)
{
  /* MOVE.L <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eal[6](reg0);
  a = read_L(addr)<<0;
  addr = get_eal[2](reg9);
  write_L(addr,(u32)a>>0);
  MOVE(a);
}

void line217(u32 reg9, u32 reg0)
{
  /* MOVE.L <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eal[7](reg0);
  a = read_L(addr)<<0;
  addr = get_eal[2](reg9);
  write_L(addr,(u32)a>>0);
  MOVE(a);
}

void line218(u32 reg9, u32 reg0)
{
  /* MOVE.L Dn,<Ae> */
  s32 a;
  u32 addr;
  a = reg68.d[reg0]<<0;
  addr = get_eal[3](reg9);
  write_L(addr,(u32)a>>0);
  MOVE(a);
}

void line219(u32 reg9, u32 reg0)
{
  /* MOVE.L An,<Ae> */
  s32 a;
  u32 addr;
  a = reg68.a[reg0]<<0;
  addr = get_eal[3](reg9);
  write_L(addr,(u32)a>>0);
  MOVE(a);
}

void line21A(u32 reg9, u32 reg0)
{
  /* MOVE.L <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eal[2](reg0);
  a = read_L(addr)<<0;
  addr = get_eal[3](reg9);
  write_L(addr,(u32)a>>0);
  MOVE(a);
}

void line21B(u32 reg9, u32 reg0)
{
  /* MOVE.L <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eal[3](reg0);
  a = read_L(addr)<<0;
  addr = get_eal[3](reg9);
  write_L(addr,(u32)a>>0);
  MOVE(a);
}

void line21C(u32 reg9, u32 reg0)
{
  /* MOVE.L <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eal[4](reg0);
  a = read_L(addr)<<0;
  addr = get_eal[3](reg9);
  write_L(addr,(u32)a>>0);
  MOVE(a);
}

void line21D(u32 reg9, u32 reg0)
{
  /* MOVE.L <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eal[5](reg0);
  a = read_L(addr)<<0;
  addr = get_eal[3](reg9);
  write_L(addr,(u32)a>>0);
  MOVE(a);
}

void line21E(u32 reg9, u32 reg0)
{
  /* MOVE.L <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eal[6](reg0);
  a = read_L(addr)<<0;
  addr = get_eal[3](reg9);
  write_L(addr,(u32)a>>0);
  MOVE(a);
}

void line21F(u32 reg9, u32 reg0)
{
  /* MOVE.L <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eal[7](reg0);
  a = read_L(addr)<<0;
  addr = get_eal[3](reg9);
  write_L(addr,(u32)a>>0);
  MOVE(a);
}

void line220(u32 reg9, u32 reg0)
{
  /* MOVE.L Dn,<Ae> */
  s32 a;
  u32 addr;
  a = reg68.d[reg0]<<0;
  addr = get_eal[4](reg9);
  write_L(addr,(u32)a>>0);
  MOVE(a);
}

void line221(u32 reg9, u32 reg0)
{
  /* MOVE.L An,<Ae> */
  s32 a;
  u32 addr;
  a = reg68.a[reg0]<<0;
  addr = get_eal[4](reg9);
  write_L(addr,(u32)a>>0);
  MOVE(a);
}

void line222(u32 reg9, u32 reg0)
{
  /* MOVE.L <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eal[2](reg0);
  a = read_L(addr)<<0;
  addr = get_eal[4](reg9);
  write_L(addr,(u32)a>>0);
  MOVE(a);
}

void line223(u32 reg9, u32 reg0)
{
  /* MOVE.L <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eal[3](reg0);
  a = read_L(addr)<<0;
  addr = get_eal[4](reg9);
  write_L(addr,(u32)a>>0);
  MOVE(a);
}

void line224(u32 reg9, u32 reg0)
{
  /* MOVE.L <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eal[4](reg0);
  a = read_L(addr)<<0;
  addr = get_eal[4](reg9);
  write_L(addr,(u32)a>>0);
  MOVE(a);
}

void line225(u32 reg9, u32 reg0)
{
  /* MOVE.L <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eal[5](reg0);
  a = read_L(addr)<<0;
  addr = get_eal[4](reg9);
  write_L(addr,(u32)a>>0);
  MOVE(a);
}

void line226(u32 reg9, u32 reg0)
{
  /* MOVE.L <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eal[6](reg0);
  a = read_L(addr)<<0;
  addr = get_eal[4](reg9);
  write_L(addr,(u32)a>>0);
  MOVE(a);
}

void line227(u32 reg9, u32 reg0)
{
  /* MOVE.L <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eal[7](reg0);
  a = read_L(addr)<<0;
  addr = get_eal[4](reg9);
  write_L(addr,(u32)a>>0);
  MOVE(a);
}

void line228(u32 reg9, u32 reg0)
{
  /* MOVE.L Dn,<Ae> */
  s32 a;
  u32 addr;
  a = reg68.d[reg0]<<0;
  addr = get_eal[5](reg9);
  write_L(addr,(u32)a>>0);
  MOVE(a);
}

void line229(u32 reg9, u32 reg0)
{
  /* MOVE.L An,<Ae> */
  s32 a;
  u32 addr;
  a = reg68.a[reg0]<<0;
  addr = get_eal[5](reg9);
  write_L(addr,(u32)a>>0);
  MOVE(a);
}

void line22A(u32 reg9, u32 reg0)
{
  /* MOVE.L <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eal[2](reg0);
  a = read_L(addr)<<0;
  addr = get_eal[5](reg9);
  write_L(addr,(u32)a>>0);
  MOVE(a);
}

void line22B(u32 reg9, u32 reg0)
{
  /* MOVE.L <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eal[3](reg0);
  a = read_L(addr)<<0;
  addr = get_eal[5](reg9);
  write_L(addr,(u32)a>>0);
  MOVE(a);
}

void line22C(u32 reg9, u32 reg0)
{
  /* MOVE.L <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eal[4](reg0);
  a = read_L(addr)<<0;
  addr = get_eal[5](reg9);
  write_L(addr,(u32)a>>0);
  MOVE(a);
}

void line22D(u32 reg9, u32 reg0)
{
  /* MOVE.L <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eal[5](reg0);
  a = read_L(addr)<<0;
  addr = get_eal[5](reg9);
  write_L(addr,(u32)a>>0);
  MOVE(a);
}

void line22E(u32 reg9, u32 reg0)
{
  /* MOVE.L <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eal[6](reg0);
  a = read_L(addr)<<0;
  addr = get_eal[5](reg9);
  write_L(addr,(u32)a>>0);
  MOVE(a);
}

void line22F(u32 reg9, u32 reg0)
{
  /* MOVE.L <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eal[7](reg0);
  a = read_L(addr)<<0;
  addr = get_eal[5](reg9);
  write_L(addr,(u32)a>>0);
  MOVE(a);
}

void line230(u32 reg9, u32 reg0)
{
  /* MOVE.L Dn,<Ae> */
  s32 a;
  u32 addr;
  a = reg68.d[reg0]<<0;
  addr = get_eal[6](reg9);
  write_L(addr,(u32)a>>0);
  MOVE(a);
}

void line231(u32 reg9, u32 reg0)
{
  /* MOVE.L An,<Ae> */
  s32 a;
  u32 addr;
  a = reg68.a[reg0]<<0;
  addr = get_eal[6](reg9);
  write_L(addr,(u32)a>>0);
  MOVE(a);
}

void line232(u32 reg9, u32 reg0)
{
  /* MOVE.L <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eal[2](reg0);
  a = read_L(addr)<<0;
  addr = get_eal[6](reg9);
  write_L(addr,(u32)a>>0);
  MOVE(a);
}

void line233(u32 reg9, u32 reg0)
{
  /* MOVE.L <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eal[3](reg0);
  a = read_L(addr)<<0;
  addr = get_eal[6](reg9);
  write_L(addr,(u32)a>>0);
  MOVE(a);
}

void line234(u32 reg9, u32 reg0)
{
  /* MOVE.L <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eal[4](reg0);
  a = read_L(addr)<<0;
  addr = get_eal[6](reg9);
  write_L(addr,(u32)a>>0);
  MOVE(a);
}

void line235(u32 reg9, u32 reg0)
{
  /* MOVE.L <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eal[5](reg0);
  a = read_L(addr)<<0;
  addr = get_eal[6](reg9);
  write_L(addr,(u32)a>>0);
  MOVE(a);
}

void line236(u32 reg9, u32 reg0)
{
  /* MOVE.L <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eal[6](reg0);
  a = read_L(addr)<<0;
  addr = get_eal[6](reg9);
  write_L(addr,(u32)a>>0);
  MOVE(a);
}

void line237(u32 reg9, u32 reg0)
{
  /* MOVE.L <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eal[7](reg0);
  a = read_L(addr)<<0;
  addr = get_eal[6](reg9);
  write_L(addr,(u32)a>>0);
  MOVE(a);
}

void line238(u32 reg9, u32 reg0)
{
  /* MOVE.L Dn,<Ae> */
  s32 a;
  u32 addr;
  a = reg68.d[reg0]<<0;
  addr = get_eal[7](reg9);
  write_L(addr,(u32)a>>0);
  MOVE(a);
}

void line239(u32 reg9, u32 reg0)
{
  /* MOVE.L An,<Ae> */
  s32 a;
  u32 addr;
  a = reg68.a[reg0]<<0;
  addr = get_eal[7](reg9);
  write_L(addr,(u32)a>>0);
  MOVE(a);
}

void line23A(u32 reg9, u32 reg0)
{
  /* MOVE.L <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eal[2](reg0);
  a = read_L(addr)<<0;
  addr = get_eal[7](reg9);
  write_L(addr,(u32)a>>0);
  MOVE(a);
}

void line23B(u32 reg9, u32 reg0)
{
  /* MOVE.L <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eal[3](reg0);
  a = read_L(addr)<<0;
  addr = get_eal[7](reg9);
  write_L(addr,(u32)a>>0);
  MOVE(a);
}

void line23C(u32 reg9, u32 reg0)
{
  /* MOVE.L <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eal[4](reg0);
  a = read_L(addr)<<0;
  addr = get_eal[7](reg9);
  write_L(addr,(u32)a>>0);
  MOVE(a);
}

void line23D(u32 reg9, u32 reg0)
{
  /* MOVE.L <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eal[5](reg0);
  a = read_L(addr)<<0;
  addr = get_eal[7](reg9);
  write_L(addr,(u32)a>>0);
  MOVE(a);
}

void line23E(u32 reg9, u32 reg0)
{
  /* MOVE.L <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eal[6](reg0);
  a = read_L(addr)<<0;
  addr = get_eal[7](reg9);
  write_L(addr,(u32)a>>0);
  MOVE(a);
}

void line23F(u32 reg9, u32 reg0)
{
  /* MOVE.L <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eal[7](reg0);
  a = read_L(addr)<<0;
  addr = get_eal[7](reg9);
  write_L(addr,(u32)a>>0);
  MOVE(a);
}

