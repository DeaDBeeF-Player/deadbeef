static void funky4_m6_0(void)
{
  RESET;
}

static void funky4_m6_1(void)
{
  NOP;
}

static void funky4_m6_2(void)
{
  STOP;
}

static void funky4_m6_3(void)
{
  RTE;
}

static void funky4_m6_4(void)
{
  ILLEGAL;
}

static void funky4_m6_5(void)
{
  RTS;
}

static void funky4_m6_6(void)
{
  TRAPV;
}

static void funky4_m6_7(void)
{
  RTR;
}


static void (*funky4_m6_func[8])(void) =
{ funky4_m6_0,funky4_m6_1,funky4_m6_2,funky4_m6_3,funky4_m6_4,funky4_m6_5,funky4_m6_6,funky4_m6_7, };

static void funky4_m0(u32 reg0)
{
  s32 a = (0<<3)+reg0;
  TRAP(a);
}

static void funky4_m1(u32 reg0)
{
  s32 a = (1<<3)+reg0;
  TRAP(a);
}

static void funky4_m2(u32 reg0)
{
  LINK(reg0);
}

static void funky4_m3(u32 reg0)
{
  UNLK(reg0);
}

static void funky4_m4(u32 reg0)
{
  reg68.usp = reg68.a[reg0];
}

static void funky4_m5(u32 reg0)
{
  reg68.a[reg0] = reg68.usp;
}

static void funky4_m6(u32 reg0)
{
  (funky4_m6_func[reg0])();
}

static void funky4_m7(u32 reg0)
{
  ILLEGAL;
}

static void (*funky4_func[8])(u32) =
{ funky4_m0,funky4_m1,funky4_m2,funky4_m3,funky4_m4,funky4_m5,funky4_m6,funky4_m7,
};

static void movemregw(u32 mode, u32 reg0)
{
  u32 m = (u16)get_nextw(), addr;
  s32 *r = reg68.d;
  addr = get_eaw[mode](reg0);
  for(; m; r++, m>>=1)
    if(m&1){ *r=(s32)read_W(addr); addr+=2; }
  if(mode==3) reg68.a[reg0] = addr;
}

static void movemregl(u32 mode, u32 reg0)
{
  u32 m = (u16)get_nextw(), addr;
  s32 *r = reg68.d;
  addr = get_eal[mode](reg0);
  for(; m; r++, m>>=1)
    if(m&1){ *r=(s32)read_L(addr); addr+=4; }
  if(mode==3) reg68.a[reg0] = addr;
}

static void movemmemw(u32 mode, u32 reg0)
{
  u32 m = (u16)get_nextw(), addr;
  if(mode==4)
  {
    s32 *r = reg68.a+7;
    addr = get_eaw[3](reg0);
    for(; m; r--, m>>=1)
      if(m&1) write_W(addr-=2,*r);
    reg68.a[reg0] = addr;
  }
  else
  {
    s32 *r = reg68.d;
    addr = get_eaw[mode](reg0);
    for(; m; r++, m>>=1)
      if(m&1){ write_W(addr,*r); addr+=2; }
  }
}

static void movemmeml(u32 mode, u32 reg0)
{
  u32 m = (u16)get_nextw(), addr;
  if(mode==4)
  {
    s32 *r = reg68.a+7;
    addr = get_eal[3](reg0);
    for(; m; r--, m>>=1)
      if(m&1) write_L(addr-=4,*r);
    reg68.a[reg0] = addr;
  }
  else
  {
    s32 *r = reg68.d;
    addr = get_eal[mode](reg0);
    for(; m; r++, m>>=1)
      if(m&1){ write_L(addr,*r); addr+=4; }
  }
}

void line4_r0_s0(u32 mode, u32 reg0)
{
  /* NEGX.B <Ae> */
  if(mode<2)
  {
    s32 s;
    s32 a=reg68.d[reg0]<<24;
    NEGXB(s,a);
    reg68.d[reg0] = (reg68.d[reg0]&0xffffff00) | (u32)s>>24;
  }
  else
  {
    u32 addr=get_eab[mode](reg0);
    s32 s,a;
    a=read_B(addr)<<24;
    NEGXB(s,a);
    write_B(addr,(u32)s>>24);
  }
}

void line4_r0_s1(u32 mode, u32 reg0)
{
  /* NEGX.W <Ae> */
  if(mode<2)
  {
    s32 s;
    s32 a=reg68.d[reg0]<<16;
    NEGXW(s,a);
    reg68.d[reg0] = (reg68.d[reg0]&0xffff0000) | (u32)s>>16;
  }
  else
  {
    u32 addr=get_eaw[mode](reg0);
    s32 s,a;
    a=read_W(addr)<<16;
    NEGXW(s,a);
    write_W(addr,(u32)s>>16);
  }
}

void line4_r0_s2(u32 mode, u32 reg0)
{
  /* NEGX.L <Ae> */
  if(mode<2)
  {
    s32 s;
    s32 a=reg68.d[reg0]<<0;
    NEGXL(s,a);
    reg68.d[reg0] = (reg68.d[reg0]&0) | (u32)s>>0;
  }
  else
  {
    u32 addr=get_eal[mode](reg0);
    s32 s,a;
    a=read_L(addr)<<0;
    NEGXL(s,a);
    write_L(addr,(u32)s>>0);
  }
}

void line4_r0_s3(u32 mode, u32 reg0)
{
  /* MOVE FROM SR */
  if(mode)
    write_W(get_eaw[mode](reg0),reg68.sr);
  else
    reg68.d[reg0]=(reg68.d[reg0]&0xFFFF0000)+(u16)reg68.sr;
}

void line4_r1_s0(u32 mode, u32 reg0)
{
  /* CLR.B <Ae> */
  if(mode<2)
  {
    s32 s;
    s32 a=0;
    CLRB(s,a);
    reg68.d[reg0] = (reg68.d[reg0]&0xffffff00) | (u32)s>>24;
  }
  else
  {
    u32 addr=get_eab[mode](reg0);
    s32 s,a;
    a=0;
    CLRB(s,a);
    write_B(addr,(u32)s>>24);
  }
}

void line4_r1_s1(u32 mode, u32 reg0)
{
  /* CLR.W <Ae> */
  if(mode<2)
  {
    s32 s;
    s32 a=0;
    CLRW(s,a);
    reg68.d[reg0] = (reg68.d[reg0]&0xffff0000) | (u32)s>>16;
  }
  else
  {
    u32 addr=get_eaw[mode](reg0);
    s32 s,a;
    a=0;
    CLRW(s,a);
    write_W(addr,(u32)s>>16);
  }
}

void line4_r1_s2(u32 mode, u32 reg0)
{
  /* CLR.L <Ae> */
  if(mode<2)
  {
    s32 s;
    s32 a=0;
    CLRL(s,a);
    reg68.d[reg0] = (reg68.d[reg0]&0) | (u32)s>>0;
  }
  else
  {
    u32 addr=get_eal[mode](reg0);
    s32 s,a;
    a=0;
    CLRL(s,a);
    write_L(addr,(u32)s>>0);
  }
}

void line4_r1_s3(u32 mode, u32 reg0)
{
  ILLEGAL;
}

void line4_r2_s0(u32 mode, u32 reg0)
{
  /* NEG.B <Ae> */
  if(mode<2)
  {
    s32 s;
    s32 a=reg68.d[reg0]<<24;
    NEGB(s,a);
    reg68.d[reg0] = (reg68.d[reg0]&0xffffff00) | (u32)s>>24;
  }
  else
  {
    u32 addr=get_eab[mode](reg0);
    s32 s,a;
    a=read_B(addr)<<24;
    NEGB(s,a);
    write_B(addr,(u32)s>>24);
  }
}

void line4_r2_s1(u32 mode, u32 reg0)
{
  /* NEG.W <Ae> */
  if(mode<2)
  {
    s32 s;
    s32 a=reg68.d[reg0]<<16;
    NEGW(s,a);
    reg68.d[reg0] = (reg68.d[reg0]&0xffff0000) | (u32)s>>16;
  }
  else
  {
    u32 addr=get_eaw[mode](reg0);
    s32 s,a;
    a=read_W(addr)<<16;
    NEGW(s,a);
    write_W(addr,(u32)s>>16);
  }
}

void line4_r2_s2(u32 mode, u32 reg0)
{
  /* NEG.L <Ae> */
  if(mode<2)
  {
    s32 s;
    s32 a=reg68.d[reg0]<<0;
    NEGL(s,a);
    reg68.d[reg0] = (reg68.d[reg0]&0) | (u32)s>>0;
  }
  else
  {
    u32 addr=get_eal[mode](reg0);
    s32 s,a;
    a=read_L(addr)<<0;
    NEGL(s,a);
    write_L(addr,(u32)s>>0);
  }
}

void line4_r2_s3(u32 mode, u32 reg0)
{
  /* MOVE TO CCR */
  if(mode)
    SET_CCR(reg68.sr,read_W(get_eaw[mode](reg0)));
  else
    SET_CCR(reg68.sr,reg68.d[reg0]);
}

void line4_r3_s0(u32 mode, u32 reg0)
{
  /* NOT.B <Ae> */
  if(mode<2)
  {
    s32 s;
    s32 a=reg68.d[reg0]<<24;
    NOTB(s,a);
    reg68.d[reg0] = (reg68.d[reg0]&0xffffff00) | (u32)s>>24;
  }
  else
  {
    u32 addr=get_eab[mode](reg0);
    s32 s,a;
    a=read_B(addr)<<24;
    NOTB(s,a);
    write_B(addr,(u32)s>>24);
  }
}

void line4_r3_s1(u32 mode, u32 reg0)
{
  /* NOT.W <Ae> */
  if(mode<2)
  {
    s32 s;
    s32 a=reg68.d[reg0]<<16;
    NOTW(s,a);
    reg68.d[reg0] = (reg68.d[reg0]&0xffff0000) | (u32)s>>16;
  }
  else
  {
    u32 addr=get_eaw[mode](reg0);
    s32 s,a;
    a=read_W(addr)<<16;
    NOTW(s,a);
    write_W(addr,(u32)s>>16);
  }
}

void line4_r3_s2(u32 mode, u32 reg0)
{
  /* NOT.L <Ae> */
  if(mode<2)
  {
    s32 s;
    s32 a=reg68.d[reg0]<<0;
    NOTL(s,a);
    reg68.d[reg0] = (reg68.d[reg0]&0) | (u32)s>>0;
  }
  else
  {
    u32 addr=get_eal[mode](reg0);
    s32 s,a;
    a=read_L(addr)<<0;
    NOTL(s,a);
    write_L(addr,(u32)s>>0);
  }
}

void line4_r3_s3(u32 mode, u32 reg0)
{
  /* MOVE TO SR */
  if(mode)
    reg68.sr = read_W(get_eaw[mode](reg0));
  else
    reg68.sr = (u16)reg68.d[reg0];
}

void line4_r4_s0(u32 mode, u32 reg0)
{
  /* NBCD.B <Ae> */
  if(mode<2)
  {
    s32 s;
    s32 a=reg68.d[reg0]<<24;
    NBCDB(s,a);
    reg68.d[reg0] = (reg68.d[reg0]&0xffffff00) | (u32)s>>24;
  }
  else
  {
    u32 addr=get_eab[mode](reg0);
    s32 s,a;
    a=read_B(addr)<<24;
    NBCDB(s,a);
    write_B(addr,(u32)s>>24);
  }
}

void line4_r4_s1(u32 mode, u32 reg0)
{
  /* SWP or PEA */
  if(!mode){ SWAP(reg68.d[reg0]); }
  else pushl(get_eal[mode](reg0));
}

void line4_r4_s2(u32 mode, u32 reg0)
{
  if(!mode){ EXTW(reg68.d[reg0]); }
  else movemmemw(mode,reg0);
}

void line4_r4_s3(u32 mode, u32 reg0)
{
  if(!mode){ EXTL(reg68.d[reg0]); }
  else movemmeml(mode,reg0);
}

void line4_r5_s0(u32 mode, u32 reg0)
{
  /* TST.B <Ae> */
  if(mode<2)
  {
    s32 s;
    s32 a=reg68.d[reg0]<<24;
    TSTB(s,a);
  }
  else
  {
    u32 addr=get_eab[mode](reg0);
    s32 s,a;
    a=read_B(addr)<<24;
    TSTB(s,a);
  }
}

void line4_r5_s1(u32 mode, u32 reg0)
{
  /* TST.W <Ae> */
  if(mode<2)
  {
    s32 s;
    s32 a=reg68.d[reg0]<<16;
    TSTW(s,a);
  }
  else
  {
    u32 addr=get_eaw[mode](reg0);
    s32 s,a;
    a=read_W(addr)<<16;
    TSTW(s,a);
  }
}

void line4_r5_s2(u32 mode, u32 reg0)
{
  /* TST.L <Ae> */
  if(mode<2)
  {
    s32 s;
    s32 a=reg68.d[reg0]<<0;
    TSTL(s,a);
  }
  else
  {
    u32 addr=get_eal[mode](reg0);
    s32 s,a;
    a=read_L(addr)<<0;
    TSTL(s,a);
  }
}

void line4_r5_s3(u32 mode, u32 reg0)
{
  if(mode<2)
  {
    s32 a = reg68.d[reg0]<<24;
    TAS(a);
    reg68.d[reg0] = (reg68.d[reg0]&0xFFFFFF00) | ((u32)a>>24);
  }
  else
  {
    s32 a;
    if(mode==7 && reg0>1){ ILLEGAL; }
    else
    {
      u32 addr = get_eab[mode](reg0);
      a = read_B(addr)<<24;
      TAS(a);
      write_B(addr,a>>24);
    }
  }
}

void line4_r6_s0(u32 mode, u32 reg0)
{
  ILLEGAL;
}

void line4_r6_s1(u32 mode, u32 reg0)
{
  ILLEGAL;
}

void line4_r6_s2(u32 mode, u32 reg0)
{
  movemregw(mode,reg0);
}

void line4_r6_s3(u32 mode, u32 reg0)
{
  movemregl(mode,reg0);
}

void line4_r7_s0(u32 mode, u32 reg0)
{
  ILLEGAL;
}

void line4_r7_s1(u32 mode, u32 reg0)
{
  (funky4_func[mode])(reg0);
}

void line4_r7_s2(u32 mode, u32 reg0)
{
  /* JSR <Ae> */
  u32 addr = get_eal[mode](reg0);
  pushl(reg68.pc);
  reg68.pc = addr;
  ADDCYCLE(4);
}

void line4_r7_s3(u32 mode, u32 reg0)
{
  /* JMP <Ae> */
  u32 addr = get_eal[mode](reg0);
  reg68.pc = addr;
  ADDCYCLE(4);
}

static void (*line4_0_func[8])(u32,u32) =
{
  line4_r0_s0,line4_r1_s0,line4_r2_s0,line4_r3_s0,line4_r4_s0,line4_r5_s0,line4_r6_s0,line4_r7_s0,
};

static void (*line4_1_func[8])(u32,u32) =
{
  line4_r0_s1,line4_r1_s1,line4_r2_s1,line4_r3_s1,line4_r4_s1,line4_r5_s1,line4_r6_s1,line4_r7_s1,
};

static void (*line4_2_func[8])(u32,u32) =
{
  line4_r0_s2,line4_r1_s2,line4_r2_s2,line4_r3_s2,line4_r4_s2,line4_r5_s2,line4_r6_s2,line4_r7_s2,
};

static void (*line4_3_func[8])(u32,u32) =
{
  line4_r0_s3,line4_r1_s3,line4_r2_s3,line4_r3_s3,line4_r4_s3,line4_r5_s3,line4_r6_s3,line4_r7_s3,
};

void line400(u32 reg9, u32 reg0)
{
  (line4_0_func[reg9])(0,reg0);
}

void line401(u32 reg9, u32 reg0)
{
  (line4_0_func[reg9])(1,reg0);
}

void line402(u32 reg9, u32 reg0)
{
  (line4_0_func[reg9])(2,reg0);
}

void line403(u32 reg9, u32 reg0)
{
  (line4_0_func[reg9])(3,reg0);
}

void line404(u32 reg9, u32 reg0)
{
  (line4_0_func[reg9])(4,reg0);
}

void line405(u32 reg9, u32 reg0)
{
  (line4_0_func[reg9])(5,reg0);
}

void line406(u32 reg9, u32 reg0)
{
  (line4_0_func[reg9])(6,reg0);
}

void line407(u32 reg9, u32 reg0)
{
  (line4_0_func[reg9])(7,reg0);
}

void line408(u32 reg9, u32 reg0)
{
  (line4_1_func[reg9])(0,reg0);
}

void line409(u32 reg9, u32 reg0)
{
  (line4_1_func[reg9])(1,reg0);
}

void line40A(u32 reg9, u32 reg0)
{
  (line4_1_func[reg9])(2,reg0);
}

void line40B(u32 reg9, u32 reg0)
{
  (line4_1_func[reg9])(3,reg0);
}

void line40C(u32 reg9, u32 reg0)
{
  (line4_1_func[reg9])(4,reg0);
}

void line40D(u32 reg9, u32 reg0)
{
  (line4_1_func[reg9])(5,reg0);
}

void line40E(u32 reg9, u32 reg0)
{
  (line4_1_func[reg9])(6,reg0);
}

void line40F(u32 reg9, u32 reg0)
{
  (line4_1_func[reg9])(7,reg0);
}

void line410(u32 reg9, u32 reg0)
{
  (line4_2_func[reg9])(0,reg0);
}

void line411(u32 reg9, u32 reg0)
{
  (line4_2_func[reg9])(1,reg0);
}

void line412(u32 reg9, u32 reg0)
{
  (line4_2_func[reg9])(2,reg0);
}

void line413(u32 reg9, u32 reg0)
{
  (line4_2_func[reg9])(3,reg0);
}

void line414(u32 reg9, u32 reg0)
{
  (line4_2_func[reg9])(4,reg0);
}

void line415(u32 reg9, u32 reg0)
{
  (line4_2_func[reg9])(5,reg0);
}

void line416(u32 reg9, u32 reg0)
{
  (line4_2_func[reg9])(6,reg0);
}

void line417(u32 reg9, u32 reg0)
{
  (line4_2_func[reg9])(7,reg0);
}

void line418(u32 reg9, u32 reg0)
{
  (line4_3_func[reg9])(0,reg0);
}

void line419(u32 reg9, u32 reg0)
{
  (line4_3_func[reg9])(1,reg0);
}

void line41A(u32 reg9, u32 reg0)
{
  (line4_3_func[reg9])(2,reg0);
}

void line41B(u32 reg9, u32 reg0)
{
  (line4_3_func[reg9])(3,reg0);
}

void line41C(u32 reg9, u32 reg0)
{
  (line4_3_func[reg9])(4,reg0);
}

void line41D(u32 reg9, u32 reg0)
{
  (line4_3_func[reg9])(5,reg0);
}

void line41E(u32 reg9, u32 reg0)
{
  (line4_3_func[reg9])(6,reg0);
}

void line41F(u32 reg9, u32 reg0)
{
  (line4_3_func[reg9])(7,reg0);
}

void line420(u32 reg9, u32 reg0)
{
  ILLEGAL;
}

void line421(u32 reg9, u32 reg0)
{
  ILLEGAL;
}

void line422(u32 reg9, u32 reg0)
{
  ILLEGAL;
}

void line423(u32 reg9, u32 reg0)
{
  ILLEGAL;
}

void line424(u32 reg9, u32 reg0)
{
  ILLEGAL;
}

void line425(u32 reg9, u32 reg0)
{
  ILLEGAL;
}

void line426(u32 reg9, u32 reg0)
{
  ILLEGAL;
}

void line427(u32 reg9, u32 reg0)
{
  ILLEGAL;
}

void line428(u32 reg9, u32 reg0)
{
  ILLEGAL;
}

void line429(u32 reg9, u32 reg0)
{
  ILLEGAL;
}

void line42A(u32 reg9, u32 reg0)
{
  ILLEGAL;
}

void line42B(u32 reg9, u32 reg0)
{
  ILLEGAL;
}

void line42C(u32 reg9, u32 reg0)
{
  ILLEGAL;
}

void line42D(u32 reg9, u32 reg0)
{
  ILLEGAL;
}

void line42E(u32 reg9, u32 reg0)
{
  ILLEGAL;
}

void line42F(u32 reg9, u32 reg0)
{
  ILLEGAL;
}

void line430(u32 reg9, u32 reg0)
{
  /* CHK.W Dx,Dy */
  s32 a,b;
  a = reg68.d[reg0]<<16;
  b = reg68.d[reg9]<<16;
  CHKW(a,b);
}

void line431(u32 reg9, u32 reg0)
{
  /* CHK <AE>,Dn */;
  ILLEGAL;
}

void line432(u32 reg9, u32 reg0)
{
  /* CHK.W <Ae>,Dn */
  s32 a,b;
  u32 addr;
  addr = get_eaw[2](reg0);
  a = read_W(addr)<<16;
  b = reg68.d[reg9]<<16;
  CHKW(a,b);
}

void line433(u32 reg9, u32 reg0)
{
  /* CHK.W <Ae>,Dn */
  s32 a,b;
  u32 addr;
  addr = get_eaw[3](reg0);
  a = read_W(addr)<<16;
  b = reg68.d[reg9]<<16;
  CHKW(a,b);
}

void line434(u32 reg9, u32 reg0)
{
  /* CHK.W <Ae>,Dn */
  s32 a,b;
  u32 addr;
  addr = get_eaw[4](reg0);
  a = read_W(addr)<<16;
  b = reg68.d[reg9]<<16;
  CHKW(a,b);
}

void line435(u32 reg9, u32 reg0)
{
  /* CHK.W <Ae>,Dn */
  s32 a,b;
  u32 addr;
  addr = get_eaw[5](reg0);
  a = read_W(addr)<<16;
  b = reg68.d[reg9]<<16;
  CHKW(a,b);
}

void line436(u32 reg9, u32 reg0)
{
  /* CHK.W <Ae>,Dn */
  s32 a,b;
  u32 addr;
  addr = get_eaw[6](reg0);
  a = read_W(addr)<<16;
  b = reg68.d[reg9]<<16;
  CHKW(a,b);
}

void line437(u32 reg9, u32 reg0)
{
  /* CHK.W <Ae>,Dn */
  s32 a,b;
  u32 addr;
  addr = get_eaw[7](reg0);
  a = read_W(addr)<<16;
  b = reg68.d[reg9]<<16;
  CHKW(a,b);
}

void line438(u32 reg9, u32 reg0)
{
  /* LEA <AE>,an */
  ILLEGAL;
}

void line439(u32 reg9, u32 reg0)
{
  /* LEA <AE>,an */
  ILLEGAL;
}

void line43A(u32 reg9, u32 reg0)
{
  /* LEA <AE>,an */
  reg68.a[reg9] = reg68.a[reg0];
}

void line43B(u32 reg9, u32 reg0)
{
  /* LEA <AE>,an */
  ILLEGAL;
}

void line43C(u32 reg9, u32 reg0)
{
  /* LEA <AE>,an */
  ILLEGAL;
}

void line43D(u32 reg9, u32 reg0)
{
  /* LEA <AE>,an */
  reg68.a[reg9] = reg68.a[reg0] + get_nextw();
}

void line43E(u32 reg9, u32 reg0)
{
  /* LEA <AE>,an */
  reg68.a[reg9] = (get_eal[6])(reg0);
}

void line43F(u32 reg9, u32 reg0)
{
  /* LEA <AE>,an */
  reg68.a[reg9] = (get_eal[7])(reg0);
}

