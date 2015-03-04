void line600(u32 reg9, u32 reg0)
{
  u32 pc = reg68.pc;
  /* Bcc.w */
  if(reg0==0) pc += get_nextw();
  /* Bcc.s */
  else pc += reg0;
  /* Bcc */
  if(!(is_cc[reg9])()) reg68.pc = pc;
}

void line601(u32 reg9, u32 reg0)
{
  u32 pc = reg68.pc;
  /* Bcc.s */
  pc += reg0+8;
  /* Bcc */
  if(!(is_cc[reg9])()) reg68.pc = pc;
}

void line602(u32 reg9, u32 reg0)
{
  u32 pc = reg68.pc;
  /* Bcc.s */
  pc += reg0+16;
  /* Bcc */
  if(!(is_cc[reg9])()) reg68.pc = pc;
}

void line603(u32 reg9, u32 reg0)
{
  u32 pc = reg68.pc;
  /* Bcc.s */
  pc += reg0+24;
  /* Bcc */
  if(!(is_cc[reg9])()) reg68.pc = pc;
}

void line604(u32 reg9, u32 reg0)
{
  u32 pc = reg68.pc;
  /* Bcc.s */
  pc += reg0+32;
  /* Bcc */
  if(!(is_cc[reg9])()) reg68.pc = pc;
}

void line605(u32 reg9, u32 reg0)
{
  u32 pc = reg68.pc;
  /* Bcc.s */
  pc += reg0+40;
  /* Bcc */
  if(!(is_cc[reg9])()) reg68.pc = pc;
}

void line606(u32 reg9, u32 reg0)
{
  u32 pc = reg68.pc;
  /* Bcc.s */
  pc += reg0+48;
  /* Bcc */
  if(!(is_cc[reg9])()) reg68.pc = pc;
}

void line607(u32 reg9, u32 reg0)
{
  u32 pc = reg68.pc;
  /* Bcc.s */
  pc += reg0+56;
  /* Bcc */
  if(!(is_cc[reg9])()) reg68.pc = pc;
}

void line608(u32 reg9, u32 reg0)
{
  u32 pc = reg68.pc;
  /* Bcc.s */
  pc += reg0+64;
  /* Bcc */
  if(!(is_cc[reg9])()) reg68.pc = pc;
}

void line609(u32 reg9, u32 reg0)
{
  u32 pc = reg68.pc;
  /* Bcc.s */
  pc += reg0+72;
  /* Bcc */
  if(!(is_cc[reg9])()) reg68.pc = pc;
}

void line60A(u32 reg9, u32 reg0)
{
  u32 pc = reg68.pc;
  /* Bcc.s */
  pc += reg0+80;
  /* Bcc */
  if(!(is_cc[reg9])()) reg68.pc = pc;
}

void line60B(u32 reg9, u32 reg0)
{
  u32 pc = reg68.pc;
  /* Bcc.s */
  pc += reg0+88;
  /* Bcc */
  if(!(is_cc[reg9])()) reg68.pc = pc;
}

void line60C(u32 reg9, u32 reg0)
{
  u32 pc = reg68.pc;
  /* Bcc.s */
  pc += reg0+96;
  /* Bcc */
  if(!(is_cc[reg9])()) reg68.pc = pc;
}

void line60D(u32 reg9, u32 reg0)
{
  u32 pc = reg68.pc;
  /* Bcc.s */
  pc += reg0+104;
  /* Bcc */
  if(!(is_cc[reg9])()) reg68.pc = pc;
}

void line60E(u32 reg9, u32 reg0)
{
  u32 pc = reg68.pc;
  /* Bcc.s */
  pc += reg0+112;
  /* Bcc */
  if(!(is_cc[reg9])()) reg68.pc = pc;
}

void line60F(u32 reg9, u32 reg0)
{
  u32 pc = reg68.pc;
  /* Bcc.s */
  pc += reg0+120;
  /* Bcc */
  if(!(is_cc[reg9])()) reg68.pc = pc;
}

void line610(u32 reg9, u32 reg0)
{
  u32 pc = reg68.pc;
  /* Bcc.s */
  pc += reg0+-128;
  /* Bcc */
  if(!(is_cc[reg9])()) reg68.pc = pc;
}

void line611(u32 reg9, u32 reg0)
{
  u32 pc = reg68.pc;
  /* Bcc.s */
  pc += reg0+-120;
  /* Bcc */
  if(!(is_cc[reg9])()) reg68.pc = pc;
}

void line612(u32 reg9, u32 reg0)
{
  u32 pc = reg68.pc;
  /* Bcc.s */
  pc += reg0+-112;
  /* Bcc */
  if(!(is_cc[reg9])()) reg68.pc = pc;
}

void line613(u32 reg9, u32 reg0)
{
  u32 pc = reg68.pc;
  /* Bcc.s */
  pc += reg0+-104;
  /* Bcc */
  if(!(is_cc[reg9])()) reg68.pc = pc;
}

void line614(u32 reg9, u32 reg0)
{
  u32 pc = reg68.pc;
  /* Bcc.s */
  pc += reg0+-96;
  /* Bcc */
  if(!(is_cc[reg9])()) reg68.pc = pc;
}

void line615(u32 reg9, u32 reg0)
{
  u32 pc = reg68.pc;
  /* Bcc.s */
  pc += reg0+-88;
  /* Bcc */
  if(!(is_cc[reg9])()) reg68.pc = pc;
}

void line616(u32 reg9, u32 reg0)
{
  u32 pc = reg68.pc;
  /* Bcc.s */
  pc += reg0+-80;
  /* Bcc */
  if(!(is_cc[reg9])()) reg68.pc = pc;
}

void line617(u32 reg9, u32 reg0)
{
  u32 pc = reg68.pc;
  /* Bcc.s */
  pc += reg0+-72;
  /* Bcc */
  if(!(is_cc[reg9])()) reg68.pc = pc;
}

void line618(u32 reg9, u32 reg0)
{
  u32 pc = reg68.pc;
  /* Bcc.s */
  pc += reg0+-64;
  /* Bcc */
  if(!(is_cc[reg9])()) reg68.pc = pc;
}

void line619(u32 reg9, u32 reg0)
{
  u32 pc = reg68.pc;
  /* Bcc.s */
  pc += reg0+-56;
  /* Bcc */
  if(!(is_cc[reg9])()) reg68.pc = pc;
}

void line61A(u32 reg9, u32 reg0)
{
  u32 pc = reg68.pc;
  /* Bcc.s */
  pc += reg0+-48;
  /* Bcc */
  if(!(is_cc[reg9])()) reg68.pc = pc;
}

void line61B(u32 reg9, u32 reg0)
{
  u32 pc = reg68.pc;
  /* Bcc.s */
  pc += reg0+-40;
  /* Bcc */
  if(!(is_cc[reg9])()) reg68.pc = pc;
}

void line61C(u32 reg9, u32 reg0)
{
  u32 pc = reg68.pc;
  /* Bcc.s */
  pc += reg0+-32;
  /* Bcc */
  if(!(is_cc[reg9])()) reg68.pc = pc;
}

void line61D(u32 reg9, u32 reg0)
{
  u32 pc = reg68.pc;
  /* Bcc.s */
  pc += reg0+-24;
  /* Bcc */
  if(!(is_cc[reg9])()) reg68.pc = pc;
}

void line61E(u32 reg9, u32 reg0)
{
  u32 pc = reg68.pc;
  /* Bcc.s */
  pc += reg0+-16;
  /* Bcc */
  if(!(is_cc[reg9])()) reg68.pc = pc;
}

void line61F(u32 reg9, u32 reg0)
{
  u32 pc = reg68.pc;
  /* Bcc.s */
  pc += reg0+-8;
  /* Bcc */
  if(!(is_cc[reg9])()) reg68.pc = pc;
}

void line620(u32 reg9, u32 reg0)
{
  u32 pc = reg68.pc;
  /* Bcc.w */
  if(reg0==0) pc += get_nextw();
  /* Bcc.s */
  else pc += reg0;
  /* BSR */
  if(reg9==0)
  {
    pushl(reg68.pc);
    reg68.pc = pc;
    ADDCYCLE(12);
  }
  else  /* Bcc */
  if((is_cc[reg9])()) reg68.pc = pc;
}

void line621(u32 reg9, u32 reg0)
{
  u32 pc = reg68.pc;
  /* Bcc.s */
  pc += reg0+8;
  /* BSR */
  if(reg9==0)
  {
    pushl(reg68.pc);
    reg68.pc = pc;
    ADDCYCLE(12);
  }
  else  /* Bcc */
  if((is_cc[reg9])()) reg68.pc = pc;
}

void line622(u32 reg9, u32 reg0)
{
  u32 pc = reg68.pc;
  /* Bcc.s */
  pc += reg0+16;
  /* BSR */
  if(reg9==0)
  {
    pushl(reg68.pc);
    reg68.pc = pc;
    ADDCYCLE(12);
  }
  else  /* Bcc */
  if((is_cc[reg9])()) reg68.pc = pc;
}

void line623(u32 reg9, u32 reg0)
{
  u32 pc = reg68.pc;
  /* Bcc.s */
  pc += reg0+24;
  /* BSR */
  if(reg9==0)
  {
    pushl(reg68.pc);
    reg68.pc = pc;
    ADDCYCLE(12);
  }
  else  /* Bcc */
  if((is_cc[reg9])()) reg68.pc = pc;
}

void line624(u32 reg9, u32 reg0)
{
  u32 pc = reg68.pc;
  /* Bcc.s */
  pc += reg0+32;
  /* BSR */
  if(reg9==0)
  {
    pushl(reg68.pc);
    reg68.pc = pc;
    ADDCYCLE(12);
  }
  else  /* Bcc */
  if((is_cc[reg9])()) reg68.pc = pc;
}

void line625(u32 reg9, u32 reg0)
{
  u32 pc = reg68.pc;
  /* Bcc.s */
  pc += reg0+40;
  /* BSR */
  if(reg9==0)
  {
    pushl(reg68.pc);
    reg68.pc = pc;
    ADDCYCLE(12);
  }
  else  /* Bcc */
  if((is_cc[reg9])()) reg68.pc = pc;
}

void line626(u32 reg9, u32 reg0)
{
  u32 pc = reg68.pc;
  /* Bcc.s */
  pc += reg0+48;
  /* BSR */
  if(reg9==0)
  {
    pushl(reg68.pc);
    reg68.pc = pc;
    ADDCYCLE(12);
  }
  else  /* Bcc */
  if((is_cc[reg9])()) reg68.pc = pc;
}

void line627(u32 reg9, u32 reg0)
{
  u32 pc = reg68.pc;
  /* Bcc.s */
  pc += reg0+56;
  /* BSR */
  if(reg9==0)
  {
    pushl(reg68.pc);
    reg68.pc = pc;
    ADDCYCLE(12);
  }
  else  /* Bcc */
  if((is_cc[reg9])()) reg68.pc = pc;
}

void line628(u32 reg9, u32 reg0)
{
  u32 pc = reg68.pc;
  /* Bcc.s */
  pc += reg0+64;
  /* BSR */
  if(reg9==0)
  {
    pushl(reg68.pc);
    reg68.pc = pc;
    ADDCYCLE(12);
  }
  else  /* Bcc */
  if((is_cc[reg9])()) reg68.pc = pc;
}

void line629(u32 reg9, u32 reg0)
{
  u32 pc = reg68.pc;
  /* Bcc.s */
  pc += reg0+72;
  /* BSR */
  if(reg9==0)
  {
    pushl(reg68.pc);
    reg68.pc = pc;
    ADDCYCLE(12);
  }
  else  /* Bcc */
  if((is_cc[reg9])()) reg68.pc = pc;
}

void line62A(u32 reg9, u32 reg0)
{
  u32 pc = reg68.pc;
  /* Bcc.s */
  pc += reg0+80;
  /* BSR */
  if(reg9==0)
  {
    pushl(reg68.pc);
    reg68.pc = pc;
    ADDCYCLE(12);
  }
  else  /* Bcc */
  if((is_cc[reg9])()) reg68.pc = pc;
}

void line62B(u32 reg9, u32 reg0)
{
  u32 pc = reg68.pc;
  /* Bcc.s */
  pc += reg0+88;
  /* BSR */
  if(reg9==0)
  {
    pushl(reg68.pc);
    reg68.pc = pc;
    ADDCYCLE(12);
  }
  else  /* Bcc */
  if((is_cc[reg9])()) reg68.pc = pc;
}

void line62C(u32 reg9, u32 reg0)
{
  u32 pc = reg68.pc;
  /* Bcc.s */
  pc += reg0+96;
  /* BSR */
  if(reg9==0)
  {
    pushl(reg68.pc);
    reg68.pc = pc;
    ADDCYCLE(12);
  }
  else  /* Bcc */
  if((is_cc[reg9])()) reg68.pc = pc;
}

void line62D(u32 reg9, u32 reg0)
{
  u32 pc = reg68.pc;
  /* Bcc.s */
  pc += reg0+104;
  /* BSR */
  if(reg9==0)
  {
    pushl(reg68.pc);
    reg68.pc = pc;
    ADDCYCLE(12);
  }
  else  /* Bcc */
  if((is_cc[reg9])()) reg68.pc = pc;
}

void line62E(u32 reg9, u32 reg0)
{
  u32 pc = reg68.pc;
  /* Bcc.s */
  pc += reg0+112;
  /* BSR */
  if(reg9==0)
  {
    pushl(reg68.pc);
    reg68.pc = pc;
    ADDCYCLE(12);
  }
  else  /* Bcc */
  if((is_cc[reg9])()) reg68.pc = pc;
}

void line62F(u32 reg9, u32 reg0)
{
  u32 pc = reg68.pc;
  /* Bcc.s */
  pc += reg0+120;
  /* BSR */
  if(reg9==0)
  {
    pushl(reg68.pc);
    reg68.pc = pc;
    ADDCYCLE(12);
  }
  else  /* Bcc */
  if((is_cc[reg9])()) reg68.pc = pc;
}

void line630(u32 reg9, u32 reg0)
{
  u32 pc = reg68.pc;
  /* Bcc.s */
  pc += reg0+-128;
  /* BSR */
  if(reg9==0)
  {
    pushl(reg68.pc);
    reg68.pc = pc;
    ADDCYCLE(12);
  }
  else  /* Bcc */
  if((is_cc[reg9])()) reg68.pc = pc;
}

void line631(u32 reg9, u32 reg0)
{
  u32 pc = reg68.pc;
  /* Bcc.s */
  pc += reg0+-120;
  /* BSR */
  if(reg9==0)
  {
    pushl(reg68.pc);
    reg68.pc = pc;
    ADDCYCLE(12);
  }
  else  /* Bcc */
  if((is_cc[reg9])()) reg68.pc = pc;
}

void line632(u32 reg9, u32 reg0)
{
  u32 pc = reg68.pc;
  /* Bcc.s */
  pc += reg0+-112;
  /* BSR */
  if(reg9==0)
  {
    pushl(reg68.pc);
    reg68.pc = pc;
    ADDCYCLE(12);
  }
  else  /* Bcc */
  if((is_cc[reg9])()) reg68.pc = pc;
}

void line633(u32 reg9, u32 reg0)
{
  u32 pc = reg68.pc;
  /* Bcc.s */
  pc += reg0+-104;
  /* BSR */
  if(reg9==0)
  {
    pushl(reg68.pc);
    reg68.pc = pc;
    ADDCYCLE(12);
  }
  else  /* Bcc */
  if((is_cc[reg9])()) reg68.pc = pc;
}

void line634(u32 reg9, u32 reg0)
{
  u32 pc = reg68.pc;
  /* Bcc.s */
  pc += reg0+-96;
  /* BSR */
  if(reg9==0)
  {
    pushl(reg68.pc);
    reg68.pc = pc;
    ADDCYCLE(12);
  }
  else  /* Bcc */
  if((is_cc[reg9])()) reg68.pc = pc;
}

void line635(u32 reg9, u32 reg0)
{
  u32 pc = reg68.pc;
  /* Bcc.s */
  pc += reg0+-88;
  /* BSR */
  if(reg9==0)
  {
    pushl(reg68.pc);
    reg68.pc = pc;
    ADDCYCLE(12);
  }
  else  /* Bcc */
  if((is_cc[reg9])()) reg68.pc = pc;
}

void line636(u32 reg9, u32 reg0)
{
  u32 pc = reg68.pc;
  /* Bcc.s */
  pc += reg0+-80;
  /* BSR */
  if(reg9==0)
  {
    pushl(reg68.pc);
    reg68.pc = pc;
    ADDCYCLE(12);
  }
  else  /* Bcc */
  if((is_cc[reg9])()) reg68.pc = pc;
}

void line637(u32 reg9, u32 reg0)
{
  u32 pc = reg68.pc;
  /* Bcc.s */
  pc += reg0+-72;
  /* BSR */
  if(reg9==0)
  {
    pushl(reg68.pc);
    reg68.pc = pc;
    ADDCYCLE(12);
  }
  else  /* Bcc */
  if((is_cc[reg9])()) reg68.pc = pc;
}

void line638(u32 reg9, u32 reg0)
{
  u32 pc = reg68.pc;
  /* Bcc.s */
  pc += reg0+-64;
  /* BSR */
  if(reg9==0)
  {
    pushl(reg68.pc);
    reg68.pc = pc;
    ADDCYCLE(12);
  }
  else  /* Bcc */
  if((is_cc[reg9])()) reg68.pc = pc;
}

void line639(u32 reg9, u32 reg0)
{
  u32 pc = reg68.pc;
  /* Bcc.s */
  pc += reg0+-56;
  /* BSR */
  if(reg9==0)
  {
    pushl(reg68.pc);
    reg68.pc = pc;
    ADDCYCLE(12);
  }
  else  /* Bcc */
  if((is_cc[reg9])()) reg68.pc = pc;
}

void line63A(u32 reg9, u32 reg0)
{
  u32 pc = reg68.pc;
  /* Bcc.s */
  pc += reg0+-48;
  /* BSR */
  if(reg9==0)
  {
    pushl(reg68.pc);
    reg68.pc = pc;
    ADDCYCLE(12);
  }
  else  /* Bcc */
  if((is_cc[reg9])()) reg68.pc = pc;
}

void line63B(u32 reg9, u32 reg0)
{
  u32 pc = reg68.pc;
  /* Bcc.s */
  pc += reg0+-40;
  /* BSR */
  if(reg9==0)
  {
    pushl(reg68.pc);
    reg68.pc = pc;
    ADDCYCLE(12);
  }
  else  /* Bcc */
  if((is_cc[reg9])()) reg68.pc = pc;
}

void line63C(u32 reg9, u32 reg0)
{
  u32 pc = reg68.pc;
  /* Bcc.s */
  pc += reg0+-32;
  /* BSR */
  if(reg9==0)
  {
    pushl(reg68.pc);
    reg68.pc = pc;
    ADDCYCLE(12);
  }
  else  /* Bcc */
  if((is_cc[reg9])()) reg68.pc = pc;
}

void line63D(u32 reg9, u32 reg0)
{
  u32 pc = reg68.pc;
  /* Bcc.s */
  pc += reg0+-24;
  /* BSR */
  if(reg9==0)
  {
    pushl(reg68.pc);
    reg68.pc = pc;
    ADDCYCLE(12);
  }
  else  /* Bcc */
  if((is_cc[reg9])()) reg68.pc = pc;
}

void line63E(u32 reg9, u32 reg0)
{
  u32 pc = reg68.pc;
  /* Bcc.s */
  pc += reg0+-16;
  /* BSR */
  if(reg9==0)
  {
    pushl(reg68.pc);
    reg68.pc = pc;
    ADDCYCLE(12);
  }
  else  /* Bcc */
  if((is_cc[reg9])()) reg68.pc = pc;
}

void line63F(u32 reg9, u32 reg0)
{
  u32 pc = reg68.pc;
  /* Bcc.s */
  pc += reg0+-8;
  /* BSR */
  if(reg9==0)
  {
    pushl(reg68.pc);
    reg68.pc = pc;
    ADDCYCLE(12);
  }
  else  /* Bcc */
  if((is_cc[reg9])()) reg68.pc = pc;
}

