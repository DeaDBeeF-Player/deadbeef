static void ASR_mem(u32 reg, u32 mode)
{
  s32 a;
  u32 addr,dec;
  dec=1;
  addr = get_eaw[mode](reg);
  a = read_W(addr)<<16;
  ASRW(a,dec);
  write_W(addr,a>>16);
}

static void LSR_mem(u32 reg, u32 mode)
{
  u32 a;
  u32 addr,dec;
  dec=1;
  addr = get_eaw[mode](reg);
  a = read_W(addr)<<16;
  LSRW(a,dec);
  write_W(addr,a>>16);
}

static void ROXR_mem(u32 reg, u32 mode)
{
  u32 a;
  u32 addr,dec;
  dec=1;
  addr = get_eaw[mode](reg);
  a = read_W(addr)<<16;
  ROXRW(a,dec);
  write_W(addr,a>>16);
}

static void ROR_mem(u32 reg, u32 mode)
{
  u32 a;
  u32 addr,dec;
  dec=1;
  addr = get_eaw[mode](reg);
  a = read_W(addr)<<16;
  RORW(a,dec);
  write_W(addr,a>>16);
}

static void ASL_mem(u32 reg, u32 mode)
{
  s32 a;
  u32 addr,dec;
  dec=1;
  addr = get_eaw[mode](reg);
  a = read_W(addr)<<16;
  ASLW(a,dec);
  write_W(addr,a>>16);
}

static void LSL_mem(u32 reg, u32 mode)
{
  u32 a;
  u32 addr,dec;
  dec=1;
  addr = get_eaw[mode](reg);
  a = read_W(addr)<<16;
  LSLW(a,dec);
  write_W(addr,a>>16);
}

static void ROXL_mem(u32 reg, u32 mode)
{
  u32 a;
  u32 addr,dec;
  dec=1;
  addr = get_eaw[mode](reg);
  a = read_W(addr)<<16;
  ROXLW(a,dec);
  write_W(addr,a>>16);
}

static void ROL_mem(u32 reg, u32 mode)
{
  u32 a;
  u32 addr,dec;
  dec=1;
  addr = get_eaw[mode](reg);
  a = read_W(addr)<<16;
  ROLW(a,dec);
  write_W(addr,a>>16);
}

static void (*lslmemR_fc[4])(u32 , u32 ) =
{
  ASR_mem,  LSR_mem,  ROXR_mem,  ROR_mem,
};

static void (*lslmemL_fc[4])(u32 , u32 ) =
{
  ASL_mem,  LSL_mem,  ROXL_mem,  ROL_mem,
};

void lineE00(u32 dec, u32 reg)
{
  s32 a;
  u32 d;
  a=reg68.d[reg]<<24;
  d = ((dec-1)&7)+1;
  ASRB(a,d);
  reg68.d[reg] = (reg68.d[reg]&0xffffff00) + ((u32)a>>24);
}

void lineE01(u32 dec, u32 reg)
{
  u32 a;
  u32 d;
  a=reg68.d[reg]<<24;
  d = ((dec-1)&7)+1;
  LSRB(a,d);
  reg68.d[reg] = (reg68.d[reg]&0xffffff00) + ((u32)a>>24);
}

void lineE02(u32 dec, u32 reg)
{
  u32 a;
  u32 d;
  a=reg68.d[reg]<<24;
  d = ((dec-1)&7)+1;
  ROXRB(a,d);
  reg68.d[reg] = (reg68.d[reg]&0xffffff00) + ((u32)a>>24);
}

void lineE03(u32 dec, u32 reg)
{
  u32 a;
  u32 d;
  a=reg68.d[reg]<<24;
  d = ((dec-1)&7)+1;
  RORB(a,d);
  reg68.d[reg] = (reg68.d[reg]&0xffffff00) + ((u32)a>>24);
}

void lineE04(u32 dec, u32 reg)
{
  s32 a;
  u32 d;
  a=reg68.d[reg]<<24;
  d=reg68.d[dec]&63;
  ASRB(a,d);
  reg68.d[reg] = (reg68.d[reg]&0xffffff00) + ((u32)a>>24);
}

void lineE05(u32 dec, u32 reg)
{
  u32 a;
  u32 d;
  a=reg68.d[reg]<<24;
  d=reg68.d[dec]&63;
  LSRB(a,d);
  reg68.d[reg] = (reg68.d[reg]&0xffffff00) + ((u32)a>>24);
}

void lineE06(u32 dec, u32 reg)
{
  u32 a;
  u32 d;
  a=reg68.d[reg]<<24;
  d=reg68.d[dec]&63;
  ROXRB(a,d);
  reg68.d[reg] = (reg68.d[reg]&0xffffff00) + ((u32)a>>24);
}

void lineE07(u32 dec, u32 reg)
{
  u32 a;
  u32 d;
  a=reg68.d[reg]<<24;
  d=reg68.d[dec]&63;
  RORB(a,d);
  reg68.d[reg] = (reg68.d[reg]&0xffffff00) + ((u32)a>>24);
}

void lineE08(u32 dec, u32 reg)
{
  s32 a;
  u32 d;
  a=reg68.d[reg]<<16;
  d = ((dec-1)&7)+1;
  ASRW(a,d);
  reg68.d[reg] = (reg68.d[reg]&0xffff0000) + ((u32)a>>16);
}

void lineE09(u32 dec, u32 reg)
{
  u32 a;
  u32 d;
  a=reg68.d[reg]<<16;
  d = ((dec-1)&7)+1;
  LSRW(a,d);
  reg68.d[reg] = (reg68.d[reg]&0xffff0000) + ((u32)a>>16);
}

void lineE0A(u32 dec, u32 reg)
{
  u32 a;
  u32 d;
  a=reg68.d[reg]<<16;
  d = ((dec-1)&7)+1;
  ROXRW(a,d);
  reg68.d[reg] = (reg68.d[reg]&0xffff0000) + ((u32)a>>16);
}

void lineE0B(u32 dec, u32 reg)
{
  u32 a;
  u32 d;
  a=reg68.d[reg]<<16;
  d = ((dec-1)&7)+1;
  RORW(a,d);
  reg68.d[reg] = (reg68.d[reg]&0xffff0000) + ((u32)a>>16);
}

void lineE0C(u32 dec, u32 reg)
{
  s32 a;
  u32 d;
  a=reg68.d[reg]<<16;
  d=reg68.d[dec]&63;
  ASRW(a,d);
  reg68.d[reg] = (reg68.d[reg]&0xffff0000) + ((u32)a>>16);
}

void lineE0D(u32 dec, u32 reg)
{
  u32 a;
  u32 d;
  a=reg68.d[reg]<<16;
  d=reg68.d[dec]&63;
  LSRW(a,d);
  reg68.d[reg] = (reg68.d[reg]&0xffff0000) + ((u32)a>>16);
}

void lineE0E(u32 dec, u32 reg)
{
  u32 a;
  u32 d;
  a=reg68.d[reg]<<16;
  d=reg68.d[dec]&63;
  ROXRW(a,d);
  reg68.d[reg] = (reg68.d[reg]&0xffff0000) + ((u32)a>>16);
}

void lineE0F(u32 dec, u32 reg)
{
  u32 a;
  u32 d;
  a=reg68.d[reg]<<16;
  d=reg68.d[dec]&63;
  RORW(a,d);
  reg68.d[reg] = (reg68.d[reg]&0xffff0000) + ((u32)a>>16);
}

void lineE10(u32 dec, u32 reg)
{
  s32 a;
  u32 d;
  a=reg68.d[reg]<<0;
  d = ((dec-1)&7)+1;
  ASRL(a,d);
  reg68.d[reg] = (reg68.d[reg]&0) + ((u32)a>>0);
}

void lineE11(u32 dec, u32 reg)
{
  u32 a;
  u32 d;
  a=reg68.d[reg]<<0;
  d = ((dec-1)&7)+1;
  LSRL(a,d);
  reg68.d[reg] = (reg68.d[reg]&0) + ((u32)a>>0);
}

void lineE12(u32 dec, u32 reg)
{
  u32 a;
  u32 d;
  a=reg68.d[reg]<<0;
  d = ((dec-1)&7)+1;
  ROXRL(a,d);
  reg68.d[reg] = (reg68.d[reg]&0) + ((u32)a>>0);
}

void lineE13(u32 dec, u32 reg)
{
  u32 a;
  u32 d;
  a=reg68.d[reg]<<0;
  d = ((dec-1)&7)+1;
  RORL(a,d);
  reg68.d[reg] = (reg68.d[reg]&0) + ((u32)a>>0);
}

void lineE14(u32 dec, u32 reg)
{
  s32 a;
  u32 d;
  a=reg68.d[reg]<<0;
  d=reg68.d[dec]&63;
  ASRL(a,d);
  reg68.d[reg] = (reg68.d[reg]&0) + ((u32)a>>0);
}

void lineE15(u32 dec, u32 reg)
{
  u32 a;
  u32 d;
  a=reg68.d[reg]<<0;
  d=reg68.d[dec]&63;
  LSRL(a,d);
  reg68.d[reg] = (reg68.d[reg]&0) + ((u32)a>>0);
}

void lineE16(u32 dec, u32 reg)
{
  u32 a;
  u32 d;
  a=reg68.d[reg]<<0;
  d=reg68.d[dec]&63;
  ROXRL(a,d);
  reg68.d[reg] = (reg68.d[reg]&0) + ((u32)a>>0);
}

void lineE17(u32 dec, u32 reg)
{
  u32 a;
  u32 d;
  a=reg68.d[reg]<<0;
  d=reg68.d[dec]&63;
  RORL(a,d);
  reg68.d[reg] = (reg68.d[reg]&0) + ((u32)a>>0);
}

void lineE18(u32 dec, u32 reg)
{
  (lslmemR_fc[dec&3])(reg,0);
}

void lineE19(u32 dec, u32 reg)
{
  (lslmemR_fc[dec&3])(reg,1);
}

void lineE1A(u32 dec, u32 reg)
{
  (lslmemR_fc[dec&3])(reg,2);
}

void lineE1B(u32 dec, u32 reg)
{
  (lslmemR_fc[dec&3])(reg,3);
}

void lineE1C(u32 dec, u32 reg)
{
  (lslmemR_fc[dec&3])(reg,4);
}

void lineE1D(u32 dec, u32 reg)
{
  (lslmemR_fc[dec&3])(reg,5);
}

void lineE1E(u32 dec, u32 reg)
{
  (lslmemR_fc[dec&3])(reg,6);
}

void lineE1F(u32 dec, u32 reg)
{
  (lslmemR_fc[dec&3])(reg,7);
}

void lineE20(u32 dec, u32 reg)
{
  s32 a;
  u32 d;
  a=reg68.d[reg]<<24;
  d = ((dec-1)&7)+1;
  ASLB(a,d);
  reg68.d[reg] = (reg68.d[reg]&0xffffff00) + ((u32)a>>24);
}

void lineE21(u32 dec, u32 reg)
{
  u32 a;
  u32 d;
  a=reg68.d[reg]<<24;
  d = ((dec-1)&7)+1;
  LSLB(a,d);
  reg68.d[reg] = (reg68.d[reg]&0xffffff00) + ((u32)a>>24);
}

void lineE22(u32 dec, u32 reg)
{
  u32 a;
  u32 d;
  a=reg68.d[reg]<<24;
  d = ((dec-1)&7)+1;
  ROXLB(a,d);
  reg68.d[reg] = (reg68.d[reg]&0xffffff00) + ((u32)a>>24);
}

void lineE23(u32 dec, u32 reg)
{
  u32 a;
  u32 d;
  a=reg68.d[reg]<<24;
  d = ((dec-1)&7)+1;
  ROLB(a,d);
  reg68.d[reg] = (reg68.d[reg]&0xffffff00) + ((u32)a>>24);
}

void lineE24(u32 dec, u32 reg)
{
  s32 a;
  u32 d;
  a=reg68.d[reg]<<24;
  d=reg68.d[dec]&63;
  ASLB(a,d);
  reg68.d[reg] = (reg68.d[reg]&0xffffff00) + ((u32)a>>24);
}

void lineE25(u32 dec, u32 reg)
{
  u32 a;
  u32 d;
  a=reg68.d[reg]<<24;
  d=reg68.d[dec]&63;
  LSLB(a,d);
  reg68.d[reg] = (reg68.d[reg]&0xffffff00) + ((u32)a>>24);
}

void lineE26(u32 dec, u32 reg)
{
  u32 a;
  u32 d;
  a=reg68.d[reg]<<24;
  d=reg68.d[dec]&63;
  ROXLB(a,d);
  reg68.d[reg] = (reg68.d[reg]&0xffffff00) + ((u32)a>>24);
}

void lineE27(u32 dec, u32 reg)
{
  u32 a;
  u32 d;
  a=reg68.d[reg]<<24;
  d=reg68.d[dec]&63;
  ROLB(a,d);
  reg68.d[reg] = (reg68.d[reg]&0xffffff00) + ((u32)a>>24);
}

void lineE28(u32 dec, u32 reg)
{
  s32 a;
  u32 d;
  a=reg68.d[reg]<<16;
  d = ((dec-1)&7)+1;
  ASLW(a,d);
  reg68.d[reg] = (reg68.d[reg]&0xffff0000) + ((u32)a>>16);
}

void lineE29(u32 dec, u32 reg)
{
  u32 a;
  u32 d;
  a=reg68.d[reg]<<16;
  d = ((dec-1)&7)+1;
  LSLW(a,d);
  reg68.d[reg] = (reg68.d[reg]&0xffff0000) + ((u32)a>>16);
}

void lineE2A(u32 dec, u32 reg)
{
  u32 a;
  u32 d;
  a=reg68.d[reg]<<16;
  d = ((dec-1)&7)+1;
  ROXLW(a,d);
  reg68.d[reg] = (reg68.d[reg]&0xffff0000) + ((u32)a>>16);
}

void lineE2B(u32 dec, u32 reg)
{
  u32 a;
  u32 d;
  a=reg68.d[reg]<<16;
  d = ((dec-1)&7)+1;
  ROLW(a,d);
  reg68.d[reg] = (reg68.d[reg]&0xffff0000) + ((u32)a>>16);
}

void lineE2C(u32 dec, u32 reg)
{
  s32 a;
  u32 d;
  a=reg68.d[reg]<<16;
  d=reg68.d[dec]&63;
  ASLW(a,d);
  reg68.d[reg] = (reg68.d[reg]&0xffff0000) + ((u32)a>>16);
}

void lineE2D(u32 dec, u32 reg)
{
  u32 a;
  u32 d;
  a=reg68.d[reg]<<16;
  d=reg68.d[dec]&63;
  LSLW(a,d);
  reg68.d[reg] = (reg68.d[reg]&0xffff0000) + ((u32)a>>16);
}

void lineE2E(u32 dec, u32 reg)
{
  u32 a;
  u32 d;
  a=reg68.d[reg]<<16;
  d=reg68.d[dec]&63;
  ROXLW(a,d);
  reg68.d[reg] = (reg68.d[reg]&0xffff0000) + ((u32)a>>16);
}

void lineE2F(u32 dec, u32 reg)
{
  u32 a;
  u32 d;
  a=reg68.d[reg]<<16;
  d=reg68.d[dec]&63;
  ROLW(a,d);
  reg68.d[reg] = (reg68.d[reg]&0xffff0000) + ((u32)a>>16);
}

void lineE30(u32 dec, u32 reg)
{
  s32 a;
  u32 d;
  a=reg68.d[reg]<<0;
  d = ((dec-1)&7)+1;
  ASLL(a,d);
  reg68.d[reg] = (reg68.d[reg]&0) + ((u32)a>>0);
}

void lineE31(u32 dec, u32 reg)
{
  u32 a;
  u32 d;
  a=reg68.d[reg]<<0;
  d = ((dec-1)&7)+1;
  LSLL(a,d);
  reg68.d[reg] = (reg68.d[reg]&0) + ((u32)a>>0);
}

void lineE32(u32 dec, u32 reg)
{
  u32 a;
  u32 d;
  a=reg68.d[reg]<<0;
  d = ((dec-1)&7)+1;
  ROXLL(a,d);
  reg68.d[reg] = (reg68.d[reg]&0) + ((u32)a>>0);
}

void lineE33(u32 dec, u32 reg)
{
  u32 a;
  u32 d;
  a=reg68.d[reg]<<0;
  d = ((dec-1)&7)+1;
  ROLL(a,d);
  reg68.d[reg] = (reg68.d[reg]&0) + ((u32)a>>0);
}

void lineE34(u32 dec, u32 reg)
{
  s32 a;
  u32 d;
  a=reg68.d[reg]<<0;
  d=reg68.d[dec]&63;
  ASLL(a,d);
  reg68.d[reg] = (reg68.d[reg]&0) + ((u32)a>>0);
}

void lineE35(u32 dec, u32 reg)
{
  u32 a;
  u32 d;
  a=reg68.d[reg]<<0;
  d=reg68.d[dec]&63;
  LSLL(a,d);
  reg68.d[reg] = (reg68.d[reg]&0) + ((u32)a>>0);
}

void lineE36(u32 dec, u32 reg)
{
  u32 a;
  u32 d;
  a=reg68.d[reg]<<0;
  d=reg68.d[dec]&63;
  ROXLL(a,d);
  reg68.d[reg] = (reg68.d[reg]&0) + ((u32)a>>0);
}

void lineE37(u32 dec, u32 reg)
{
  u32 a;
  u32 d;
  a=reg68.d[reg]<<0;
  d=reg68.d[dec]&63;
  ROLL(a,d);
  reg68.d[reg] = (reg68.d[reg]&0) + ((u32)a>>0);
}

void lineE38(u32 dec, u32 reg)
{
  (lslmemL_fc[dec&3])(reg,0);
}

void lineE39(u32 dec, u32 reg)
{
  (lslmemL_fc[dec&3])(reg,1);
}

void lineE3A(u32 dec, u32 reg)
{
  (lslmemL_fc[dec&3])(reg,2);
}

void lineE3B(u32 dec, u32 reg)
{
  (lslmemL_fc[dec&3])(reg,3);
}

void lineE3C(u32 dec, u32 reg)
{
  (lslmemL_fc[dec&3])(reg,4);
}

void lineE3D(u32 dec, u32 reg)
{
  (lslmemL_fc[dec&3])(reg,5);
}

void lineE3E(u32 dec, u32 reg)
{
  (lslmemL_fc[dec&3])(reg,6);
}

void lineE3F(u32 dec, u32 reg)
{
  (lslmemL_fc[dec&3])(reg,7);
}

