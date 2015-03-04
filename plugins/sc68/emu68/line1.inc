void line100(u32 reg9, u32 reg0)
{
  /* MOVE.B Dn,Dn */
  s32 a;
  a = reg68.d[reg0]<<24;
  reg68.d[reg9] = (reg68.d[reg9]&0xffffff00) + ((u32)a>>24);
  MOVE(a);
}

void line101(u32 reg9, u32 reg0)
{
  /* MOVE.B An,Dn */
  s32 a;
  a = reg68.a[reg0]<<24;
  reg68.d[reg9] = (reg68.d[reg9]&0xffffff00) + ((u32)a>>24);
  MOVE(a);
}

void line102(u32 reg9, u32 reg0)
{
  /* MOVE.B <Ae>,Dn */
  s32 a;
  u32 addr;
  addr = get_eab[2](reg0);
  a = read_B(addr)<<24;
  reg68.d[reg9] = (reg68.d[reg9]&0xffffff00) + ((u32)a>>24);
  MOVE(a);
}

void line103(u32 reg9, u32 reg0)
{
  /* MOVE.B <Ae>,Dn */
  s32 a;
  u32 addr;
  addr = get_eab[3](reg0);
  a = read_B(addr)<<24;
  reg68.d[reg9] = (reg68.d[reg9]&0xffffff00) + ((u32)a>>24);
  MOVE(a);
}

void line104(u32 reg9, u32 reg0)
{
  /* MOVE.B <Ae>,Dn */
  s32 a;
  u32 addr;
  addr = get_eab[4](reg0);
  a = read_B(addr)<<24;
  reg68.d[reg9] = (reg68.d[reg9]&0xffffff00) + ((u32)a>>24);
  MOVE(a);
}

void line105(u32 reg9, u32 reg0)
{
  /* MOVE.B <Ae>,Dn */
  s32 a;
  u32 addr;
  addr = get_eab[5](reg0);
  a = read_B(addr)<<24;
  reg68.d[reg9] = (reg68.d[reg9]&0xffffff00) + ((u32)a>>24);
  MOVE(a);
}

void line106(u32 reg9, u32 reg0)
{
  /* MOVE.B <Ae>,Dn */
  s32 a;
  u32 addr;
  addr = get_eab[6](reg0);
  a = read_B(addr)<<24;
  reg68.d[reg9] = (reg68.d[reg9]&0xffffff00) + ((u32)a>>24);
  MOVE(a);
}

void line107(u32 reg9, u32 reg0)
{
  /* MOVE.B <Ae>,Dn */
  s32 a;
  u32 addr;
  addr = get_eab[7](reg0);
  a = read_B(addr)<<24;
  reg68.d[reg9] = (reg68.d[reg9]&0xffffff00) + ((u32)a>>24);
  MOVE(a);
}

void line108(u32 reg9, u32 reg0)
{
  /* MOVE.B Dn,An */
  s32 a;
  a = reg68.d[reg0]<<24;
  reg68.a[reg9] = a>>24;
}

void line109(u32 reg9, u32 reg0)
{
  /* MOVE.B An,An */
  s32 a;
  a = reg68.a[reg0]<<24;
  reg68.a[reg9] = a>>24;
}

void line10A(u32 reg9, u32 reg0)
{
  /* MOVE.B <Ae>,An */
  s32 a;
  u32 addr;
  addr = get_eab[2](reg0);
  a = read_B(addr)<<24;
  reg68.a[reg9] = a>>24;
}

void line10B(u32 reg9, u32 reg0)
{
  /* MOVE.B <Ae>,An */
  s32 a;
  u32 addr;
  addr = get_eab[3](reg0);
  a = read_B(addr)<<24;
  reg68.a[reg9] = a>>24;
}

void line10C(u32 reg9, u32 reg0)
{
  /* MOVE.B <Ae>,An */
  s32 a;
  u32 addr;
  addr = get_eab[4](reg0);
  a = read_B(addr)<<24;
  reg68.a[reg9] = a>>24;
}

void line10D(u32 reg9, u32 reg0)
{
  /* MOVE.B <Ae>,An */
  s32 a;
  u32 addr;
  addr = get_eab[5](reg0);
  a = read_B(addr)<<24;
  reg68.a[reg9] = a>>24;
}

void line10E(u32 reg9, u32 reg0)
{
  /* MOVE.B <Ae>,An */
  s32 a;
  u32 addr;
  addr = get_eab[6](reg0);
  a = read_B(addr)<<24;
  reg68.a[reg9] = a>>24;
}

void line10F(u32 reg9, u32 reg0)
{
  /* MOVE.B <Ae>,An */
  s32 a;
  u32 addr;
  addr = get_eab[7](reg0);
  a = read_B(addr)<<24;
  reg68.a[reg9] = a>>24;
}

void line110(u32 reg9, u32 reg0)
{
  /* MOVE.B Dn,<Ae> */
  s32 a;
  u32 addr;
  a = reg68.d[reg0]<<24;
  addr = get_eab[2](reg9);
  write_B(addr,(u32)a>>24);
  MOVE(a);
}

void line111(u32 reg9, u32 reg0)
{
  /* MOVE.B An,<Ae> */
  s32 a;
  u32 addr;
  a = reg68.a[reg0]<<24;
  addr = get_eab[2](reg9);
  write_B(addr,(u32)a>>24);
  MOVE(a);
}

void line112(u32 reg9, u32 reg0)
{
  /* MOVE.B <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eab[2](reg0);
  a = read_B(addr)<<24;
  addr = get_eab[2](reg9);
  write_B(addr,(u32)a>>24);
  MOVE(a);
}

void line113(u32 reg9, u32 reg0)
{
  /* MOVE.B <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eab[3](reg0);
  a = read_B(addr)<<24;
  addr = get_eab[2](reg9);
  write_B(addr,(u32)a>>24);
  MOVE(a);
}

void line114(u32 reg9, u32 reg0)
{
  /* MOVE.B <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eab[4](reg0);
  a = read_B(addr)<<24;
  addr = get_eab[2](reg9);
  write_B(addr,(u32)a>>24);
  MOVE(a);
}

void line115(u32 reg9, u32 reg0)
{
  /* MOVE.B <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eab[5](reg0);
  a = read_B(addr)<<24;
  addr = get_eab[2](reg9);
  write_B(addr,(u32)a>>24);
  MOVE(a);
}

void line116(u32 reg9, u32 reg0)
{
  /* MOVE.B <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eab[6](reg0);
  a = read_B(addr)<<24;
  addr = get_eab[2](reg9);
  write_B(addr,(u32)a>>24);
  MOVE(a);
}

void line117(u32 reg9, u32 reg0)
{
  /* MOVE.B <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eab[7](reg0);
  a = read_B(addr)<<24;
  addr = get_eab[2](reg9);
  write_B(addr,(u32)a>>24);
  MOVE(a);
}

void line118(u32 reg9, u32 reg0)
{
  /* MOVE.B Dn,<Ae> */
  s32 a;
  u32 addr;
  a = reg68.d[reg0]<<24;
  addr = get_eab[3](reg9);
  write_B(addr,(u32)a>>24);
  MOVE(a);
}

void line119(u32 reg9, u32 reg0)
{
  /* MOVE.B An,<Ae> */
  s32 a;
  u32 addr;
  a = reg68.a[reg0]<<24;
  addr = get_eab[3](reg9);
  write_B(addr,(u32)a>>24);
  MOVE(a);
}

void line11A(u32 reg9, u32 reg0)
{
  /* MOVE.B <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eab[2](reg0);
  a = read_B(addr)<<24;
  addr = get_eab[3](reg9);
  write_B(addr,(u32)a>>24);
  MOVE(a);
}

void line11B(u32 reg9, u32 reg0)
{
  /* MOVE.B <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eab[3](reg0);
  a = read_B(addr)<<24;
  addr = get_eab[3](reg9);
  write_B(addr,(u32)a>>24);
  MOVE(a);
}

void line11C(u32 reg9, u32 reg0)
{
  /* MOVE.B <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eab[4](reg0);
  a = read_B(addr)<<24;
  addr = get_eab[3](reg9);
  write_B(addr,(u32)a>>24);
  MOVE(a);
}

void line11D(u32 reg9, u32 reg0)
{
  /* MOVE.B <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eab[5](reg0);
  a = read_B(addr)<<24;
  addr = get_eab[3](reg9);
  write_B(addr,(u32)a>>24);
  MOVE(a);
}

void line11E(u32 reg9, u32 reg0)
{
  /* MOVE.B <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eab[6](reg0);
  a = read_B(addr)<<24;
  addr = get_eab[3](reg9);
  write_B(addr,(u32)a>>24);
  MOVE(a);
}

void line11F(u32 reg9, u32 reg0)
{
  /* MOVE.B <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eab[7](reg0);
  a = read_B(addr)<<24;
  addr = get_eab[3](reg9);
  write_B(addr,(u32)a>>24);
  MOVE(a);
}

void line120(u32 reg9, u32 reg0)
{
  /* MOVE.B Dn,<Ae> */
  s32 a;
  u32 addr;
  a = reg68.d[reg0]<<24;
  addr = get_eab[4](reg9);
  write_B(addr,(u32)a>>24);
  MOVE(a);
}

void line121(u32 reg9, u32 reg0)
{
  /* MOVE.B An,<Ae> */
  s32 a;
  u32 addr;
  a = reg68.a[reg0]<<24;
  addr = get_eab[4](reg9);
  write_B(addr,(u32)a>>24);
  MOVE(a);
}

void line122(u32 reg9, u32 reg0)
{
  /* MOVE.B <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eab[2](reg0);
  a = read_B(addr)<<24;
  addr = get_eab[4](reg9);
  write_B(addr,(u32)a>>24);
  MOVE(a);
}

void line123(u32 reg9, u32 reg0)
{
  /* MOVE.B <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eab[3](reg0);
  a = read_B(addr)<<24;
  addr = get_eab[4](reg9);
  write_B(addr,(u32)a>>24);
  MOVE(a);
}

void line124(u32 reg9, u32 reg0)
{
  /* MOVE.B <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eab[4](reg0);
  a = read_B(addr)<<24;
  addr = get_eab[4](reg9);
  write_B(addr,(u32)a>>24);
  MOVE(a);
}

void line125(u32 reg9, u32 reg0)
{
  /* MOVE.B <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eab[5](reg0);
  a = read_B(addr)<<24;
  addr = get_eab[4](reg9);
  write_B(addr,(u32)a>>24);
  MOVE(a);
}

void line126(u32 reg9, u32 reg0)
{
  /* MOVE.B <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eab[6](reg0);
  a = read_B(addr)<<24;
  addr = get_eab[4](reg9);
  write_B(addr,(u32)a>>24);
  MOVE(a);
}

void line127(u32 reg9, u32 reg0)
{
  /* MOVE.B <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eab[7](reg0);
  a = read_B(addr)<<24;
  addr = get_eab[4](reg9);
  write_B(addr,(u32)a>>24);
  MOVE(a);
}

void line128(u32 reg9, u32 reg0)
{
  /* MOVE.B Dn,<Ae> */
  s32 a;
  u32 addr;
  a = reg68.d[reg0]<<24;
  addr = get_eab[5](reg9);
  write_B(addr,(u32)a>>24);
  MOVE(a);
}

void line129(u32 reg9, u32 reg0)
{
  /* MOVE.B An,<Ae> */
  s32 a;
  u32 addr;
  a = reg68.a[reg0]<<24;
  addr = get_eab[5](reg9);
  write_B(addr,(u32)a>>24);
  MOVE(a);
}

void line12A(u32 reg9, u32 reg0)
{
  /* MOVE.B <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eab[2](reg0);
  a = read_B(addr)<<24;
  addr = get_eab[5](reg9);
  write_B(addr,(u32)a>>24);
  MOVE(a);
}

void line12B(u32 reg9, u32 reg0)
{
  /* MOVE.B <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eab[3](reg0);
  a = read_B(addr)<<24;
  addr = get_eab[5](reg9);
  write_B(addr,(u32)a>>24);
  MOVE(a);
}

void line12C(u32 reg9, u32 reg0)
{
  /* MOVE.B <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eab[4](reg0);
  a = read_B(addr)<<24;
  addr = get_eab[5](reg9);
  write_B(addr,(u32)a>>24);
  MOVE(a);
}

void line12D(u32 reg9, u32 reg0)
{
  /* MOVE.B <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eab[5](reg0);
  a = read_B(addr)<<24;
  addr = get_eab[5](reg9);
  write_B(addr,(u32)a>>24);
  MOVE(a);
}

void line12E(u32 reg9, u32 reg0)
{
  /* MOVE.B <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eab[6](reg0);
  a = read_B(addr)<<24;
  addr = get_eab[5](reg9);
  write_B(addr,(u32)a>>24);
  MOVE(a);
}

void line12F(u32 reg9, u32 reg0)
{
  /* MOVE.B <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eab[7](reg0);
  a = read_B(addr)<<24;
  addr = get_eab[5](reg9);
  write_B(addr,(u32)a>>24);
  MOVE(a);
}

void line130(u32 reg9, u32 reg0)
{
  /* MOVE.B Dn,<Ae> */
  s32 a;
  u32 addr;
  a = reg68.d[reg0]<<24;
  addr = get_eab[6](reg9);
  write_B(addr,(u32)a>>24);
  MOVE(a);
}

void line131(u32 reg9, u32 reg0)
{
  /* MOVE.B An,<Ae> */
  s32 a;
  u32 addr;
  a = reg68.a[reg0]<<24;
  addr = get_eab[6](reg9);
  write_B(addr,(u32)a>>24);
  MOVE(a);
}

void line132(u32 reg9, u32 reg0)
{
  /* MOVE.B <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eab[2](reg0);
  a = read_B(addr)<<24;
  addr = get_eab[6](reg9);
  write_B(addr,(u32)a>>24);
  MOVE(a);
}

void line133(u32 reg9, u32 reg0)
{
  /* MOVE.B <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eab[3](reg0);
  a = read_B(addr)<<24;
  addr = get_eab[6](reg9);
  write_B(addr,(u32)a>>24);
  MOVE(a);
}

void line134(u32 reg9, u32 reg0)
{
  /* MOVE.B <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eab[4](reg0);
  a = read_B(addr)<<24;
  addr = get_eab[6](reg9);
  write_B(addr,(u32)a>>24);
  MOVE(a);
}

void line135(u32 reg9, u32 reg0)
{
  /* MOVE.B <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eab[5](reg0);
  a = read_B(addr)<<24;
  addr = get_eab[6](reg9);
  write_B(addr,(u32)a>>24);
  MOVE(a);
}

void line136(u32 reg9, u32 reg0)
{
  /* MOVE.B <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eab[6](reg0);
  a = read_B(addr)<<24;
  addr = get_eab[6](reg9);
  write_B(addr,(u32)a>>24);
  MOVE(a);
}

void line137(u32 reg9, u32 reg0)
{
  /* MOVE.B <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eab[7](reg0);
  a = read_B(addr)<<24;
  addr = get_eab[6](reg9);
  write_B(addr,(u32)a>>24);
  MOVE(a);
}

void line138(u32 reg9, u32 reg0)
{
  /* MOVE.B Dn,<Ae> */
  s32 a;
  u32 addr;
  a = reg68.d[reg0]<<24;
  addr = get_eab[7](reg9);
  write_B(addr,(u32)a>>24);
  MOVE(a);
}

void line139(u32 reg9, u32 reg0)
{
  /* MOVE.B An,<Ae> */
  s32 a;
  u32 addr;
  a = reg68.a[reg0]<<24;
  addr = get_eab[7](reg9);
  write_B(addr,(u32)a>>24);
  MOVE(a);
}

void line13A(u32 reg9, u32 reg0)
{
  /* MOVE.B <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eab[2](reg0);
  a = read_B(addr)<<24;
  addr = get_eab[7](reg9);
  write_B(addr,(u32)a>>24);
  MOVE(a);
}

void line13B(u32 reg9, u32 reg0)
{
  /* MOVE.B <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eab[3](reg0);
  a = read_B(addr)<<24;
  addr = get_eab[7](reg9);
  write_B(addr,(u32)a>>24);
  MOVE(a);
}

void line13C(u32 reg9, u32 reg0)
{
  /* MOVE.B <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eab[4](reg0);
  a = read_B(addr)<<24;
  addr = get_eab[7](reg9);
  write_B(addr,(u32)a>>24);
  MOVE(a);
}

void line13D(u32 reg9, u32 reg0)
{
  /* MOVE.B <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eab[5](reg0);
  a = read_B(addr)<<24;
  addr = get_eab[7](reg9);
  write_B(addr,(u32)a>>24);
  MOVE(a);
}

void line13E(u32 reg9, u32 reg0)
{
  /* MOVE.B <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eab[6](reg0);
  a = read_B(addr)<<24;
  addr = get_eab[7](reg9);
  write_B(addr,(u32)a>>24);
  MOVE(a);
}

void line13F(u32 reg9, u32 reg0)
{
  /* MOVE.B <Ae>,<Ae> */
  s32 a;
  u32 addr;
  addr = get_eab[7](reg0);
  a = read_B(addr)<<24;
  addr = get_eab[7](reg9);
  write_B(addr,(u32)a>>24);
  MOVE(a);
}

