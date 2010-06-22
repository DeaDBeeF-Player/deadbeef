#ifndef _SAT_HW_H_
#define _SAT_HW_H_

extern uint8 sat_ram[512*1024];

void sat_hw_init(void);

#if !LSB_FIRST
static unsigned short INLINE mem_readword_swap(unsigned short *addr)
{
	return ((*addr&0x00ff)<<8)|((*addr&0xff00)>>8);
}

static void INLINE mem_writeword_swap(unsigned short *addr, unsigned short value)
{
	*addr = ((value&0x00ff)<<8)|((value&0xff00)>>8);
}
#else	// big endian
static unsigned short INLINE mem_readword_swap(unsigned short *addr)
{
	unsigned long retval;

	retval = *addr;

	return retval;
}

static void INLINE mem_writeword_swap(unsigned short *addr, unsigned short value)
{
	*addr = value;
}
#endif


#endif
