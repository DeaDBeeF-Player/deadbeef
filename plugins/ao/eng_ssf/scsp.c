/*
    Sega/Yamaha YMF292-F (SCSP = Saturn Custom Sound Processor) emulation
    By ElSemi
    MAME/M1 conversion and cleanup by R. Belmont
    Additional code and bugfixes by kingshriek

    This chip has 32 voices.  Each voice can play a sample or be part of
    an FM construct.  Unlike traditional Yamaha FM chips, the base waveform
    for the FM still comes from the wavetable RAM.

    ChangeLog:
    * November 25, 2003 (ES) Fixed buggy timers and envelope overflows.
                        (RB) Improved sample rates other than 44100, multiple
                             chips now works properly.
    * December 02, 2003 (ES) Added DISDL register support, improves mix.
    * April 28, 2004    (ES) Corrected envelope rates, added key-rate scaling,
                             added ringbuffer support.
    * January 8, 2005   (RB) Added ability to specify region offset for RAM.
    * January 26, 2007  (ES) Added on-board DSP capability
    * December 16, 2007 (kingshriek) Many EG bug fixes, implemented effects mixer,
                             implemented FM.
*/

#include <math.h>
#include <string.h>
#include "ao.h"
#include "cpuintrf.h"
#include "scsp.h"
#include "scspdsp.h"
#include "sat_hw.h"

#define ICLIP16(x) (x<-32768)?-32768:((x>32767)?32767:x)

#define SHIFT	12
#define FIX(v)	((UINT32) ((float) (1<<SHIFT)*(v)))


#define EG_SHIFT	16
#define FM_DELAY    0   // delay in number of slots processed before samples are written to the FM ring buffer

// include the LFO handling code
#include "scsplfo.c"

/*
    SCSP features 32 programmable slots
    that can generate FM and PCM (from ROM/RAM) sound
*/

//SLOT PARAMETERS
#define KEYONEX(slot)		((slot->udata.data[0x0]>>0x0)&0x1000)
#define KEYONB(slot)		((slot->udata.data[0x0]>>0x0)&0x0800)
#define SBCTL(slot)		((slot->udata.data[0x0]>>0x9)&0x0003)
#define SSCTL(slot)		((slot->udata.data[0x0]>>0x7)&0x0003)
#define LPCTL(slot)		((slot->udata.data[0x0]>>0x5)&0x0003)
#define PCM8B(slot)		((slot->udata.data[0x0]>>0x0)&0x0010)

#define SA(slot)		(((slot->udata.data[0x0]&0xF)<<16)|(slot->udata.data[0x1]))

#define LSA(slot)		(slot->udata.data[0x2])

#define LEA(slot)		(slot->udata.data[0x3])

#define D2R(slot)		((slot->udata.data[0x4]>>0xB)&0x001F)
#define D1R(slot)		((slot->udata.data[0x4]>>0x6)&0x001F)
#define EGHOLD(slot)		((slot->udata.data[0x4]>>0x0)&0x0020)
#define AR(slot)		((slot->udata.data[0x4]>>0x0)&0x001F)

#define LPSLNK(slot)		((slot->udata.data[0x5]>>0x0)&0x4000)
#define KRS(slot)		((slot->udata.data[0x5]>>0xA)&0x000F)
#define DL(slot)		((slot->udata.data[0x5]>>0x5)&0x001F)
#define RR(slot)		((slot->udata.data[0x5]>>0x0)&0x001F)

#define STWINH(slot)		((slot->udata.data[0x6]>>0x0)&0x0200)
#define SDIR(slot)		((slot->udata.data[0x6]>>0x0)&0x0100)
#define TL(slot)		((slot->udata.data[0x6]>>0x0)&0x00FF)

#define MDL(slot)		((slot->udata.data[0x7]>>0xC)&0x000F)
#define MDXSL(slot)		((slot->udata.data[0x7]>>0x6)&0x003F)
#define MDYSL(slot)		((slot->udata.data[0x7]>>0x0)&0x003F)

#define OCT(slot)		((slot->udata.data[0x8]>>0xB)&0x000F)
#define FNS(slot)		((slot->udata.data[0x8]>>0x0)&0x03FF)

#define LFORE(slot)		((slot->udata.data[0x9]>>0x0)&0x8000)
#define LFOF(slot)		((slot->udata.data[0x9]>>0xA)&0x001F)
#define PLFOWS(slot)		((slot->udata.data[0x9]>>0x8)&0x0003)
#define PLFOS(slot)		((slot->udata.data[0x9]>>0x5)&0x0007)
#define ALFOWS(slot)		((slot->udata.data[0x9]>>0x3)&0x0003)
#define ALFOS(slot)		((slot->udata.data[0x9]>>0x0)&0x0007)

#define ISEL(slot)		((slot->udata.data[0xA]>>0x3)&0x000F)
#define IMXL(slot)		((slot->udata.data[0xA]>>0x0)&0x0007)

#define DISDL(slot)		((slot->udata.data[0xB]>>0xD)&0x0007)
#define DIPAN(slot)		((slot->udata.data[0xB]>>0x8)&0x001F)
#define EFSDL(slot)		((slot->udata.data[0xB]>>0x5)&0x0007)
#define EFPAN(slot)		((slot->udata.data[0xB]>>0x0)&0x001F)

//Envelope times in ms
static const double ARTimes[64]={100000/*infinity*/,100000/*infinity*/,8100.0,6900.0,6000.0,4800.0,4000.0,3400.0,3000.0,2400.0,2000.0,1700.0,1500.0,
					1200.0,1000.0,860.0,760.0,600.0,500.0,430.0,380.0,300.0,250.0,220.0,190.0,150.0,130.0,110.0,95.0,
					76.0,63.0,55.0,47.0,38.0,31.0,27.0,24.0,19.0,15.0,13.0,12.0,9.4,7.9,6.8,6.0,4.7,3.8,3.4,3.0,2.4,
					2.0,1.8,1.6,1.3,1.1,0.93,0.85,0.65,0.53,0.44,0.40,0.35,0.0,0.0};
static const double DRTimes[64]={100000/*infinity*/,100000/*infinity*/,118200.0,101300.0,88600.0,70900.0,59100.0,50700.0,44300.0,35500.0,29600.0,25300.0,22200.0,17700.0,
					14800.0,12700.0,11100.0,8900.0,7400.0,6300.0,5500.0,4400.0,3700.0,3200.0,2800.0,2200.0,1800.0,1600.0,1400.0,1100.0,
					920.0,790.0,690.0,550.0,460.0,390.0,340.0,270.0,230.0,200.0,170.0,140.0,110.0,98.0,85.0,68.0,57.0,49.0,43.0,34.0,
					28.0,25.0,22.0,18.0,14.0,12.0,11.0,8.5,7.1,6.1,5.4,4.3,3.6,3.1};
static UINT32 FNS_Table[0x400];
static INT32 EG_TABLE[0x400];

typedef enum {ATTACK,DECAY1,DECAY2,RELEASE} _STATE;
struct _EG
{
	int volume;	//
	_STATE state;
	int step;
	//step vals
	int AR;		//Attack
	int D1R;	//Decay1
	int D2R;	//Decay2
	int RR;		//Release

	int DL;		//Decay level
	UINT8 EGHOLD;
	UINT8 LPLINK;
};

struct _SLOT
{
	union
	{
		UINT16 data[0x10];	//only 0x1a bytes used
		UINT8 datab[0x20];
	} udata;
	UINT8 active;	//this slot is currently playing
	UINT8 *base;		//samples base address
	UINT32 cur_addr;	//current play address (24.8)
	UINT32 nxt_addr;	//next play address
	UINT32 step;		//pitch step (24.8)
	UINT8 Backwards;	//the wave is playing backwards
	struct _EG EG;			//Envelope
	struct _LFO PLFO;		//Phase LFO
	struct _LFO ALFO;		//Amplitude LFO
	int slot;
	signed short Prev;	//Previous sample (for interpolation)
};

#define MEM4B(scsp)		((scsp->udata.data[0]>>0x0)&0x0200)
#define DAC18B(scsp)		((scsp->udata.data[0]>>0x0)&0x0100)
#define MVOL(scsp)		((scsp->udata.data[0]>>0x0)&0x000F)
#define RBL(scsp)		((scsp->udata.data[1]>>0x7)&0x0003)
#define RBP(scsp)		((scsp->udata.data[1]>>0x0)&0x003F)
#define MOFULL(scsp)   		((scsp->udata.data[2]>>0x0)&0x1000)
#define MOEMPTY(scsp)		((scsp->udata.data[2]>>0x0)&0x0800)
#define MIOVF(scsp)		((scsp->udata.data[2]>>0x0)&0x0400)
#define MIFULL(scsp)		((scsp->udata.data[2]>>0x0)&0x0200)
#define MIEMPTY(scsp)		((scsp->udata.data[2]>>0x0)&0x0100)

#define SCILV0(scsp)    	((scsp->udata.data[0x24/2]>>0x0)&0xff)
#define SCILV1(scsp)    	((scsp->udata.data[0x26/2]>>0x0)&0xff)
#define SCILV2(scsp)    	((scsp->udata.data[0x28/2]>>0x0)&0xff)

#define SCIEX0	0
#define SCIEX1	1
#define SCIEX2	2
#define SCIMID	3
#define SCIDMA	4
#define SCIIRQ	5
#define SCITMA	6
#define SCITMB	7

#define USEDSP

struct m68ki_cpu_core_s;

struct _SCSP
{
	union
	{
		UINT16 data[0x30/2];
		UINT8 datab[0x30];
	} udata;
	struct _SLOT Slots[32];
	signed short RINGBUF[64];
	unsigned char BUFPTR;
#if FM_DELAY
	signed short DELAYBUF[FM_DELAY];
	unsigned char DELAYPTR;
#endif
	unsigned char *SCSPRAM;
	UINT32 SCSPRAM_LENGTH;
	char Master;
	void (*Int68kCB)(struct m68ki_cpu_core_s *cpu, int irq);

	INT32 *buffertmpl, *buffertmpr;

	UINT32 IrqTimA;
	UINT32 IrqTimBC;
	UINT32 IrqMidi;

	UINT8 MidiOutW,MidiOutR;
	UINT8 MidiStack[16];
	UINT8 MidiW,MidiR;

	int LPANTABLE[0x10000];
	int RPANTABLE[0x10000];

	int TimPris[3];
	int TimCnt[3];

	// DMA stuff
	UINT32 scsp_dmea;
	UINT16 scsp_drga;
	UINT16 scsp_dtlg;

	int ARTABLE[64], DRTABLE[64];

    struct m68ki_cpu_core_s *cpu;
	struct _SCSPDSP DSP;

    INT16 *bufferl;
    INT16 *bufferr;

    int length;

    signed short *RBUFDST;	//this points to where the sample will be stored in the RingBuf
};

static void dma_scsp(struct _SCSP *SCSP); 		/*SCSP DMA transfer function*/
#define	scsp_dgate		scsp_regs[0x16/2] & 0x4000
#define	scsp_ddir		scsp_regs[0x16/2] & 0x2000
#define scsp_dexe 		scsp_regs[0x16/2] & 0x1000
#define dma_transfer_end	((scsp_regs[0x24/2] & 0x10)>>4)|(((scsp_regs[0x26/2] & 0x10)>>4)<<1)|(((scsp_regs[0x28/2] & 0x10)>>4)<<2)

static const float SDLT[8]={-1000000.0,-36.0,-30.0,-24.0,-18.0,-12.0,-6.0,0.0};

static unsigned char DecodeSCI(struct _SCSP *SCSP,unsigned char irq)
{
	unsigned char SCI=0;
	unsigned char v;
	v=(SCILV0((SCSP))&(1<<irq))?1:0;
	SCI|=v;
	v=(SCILV1((SCSP))&(1<<irq))?1:0;
	SCI|=v<<1;
	v=(SCILV2((SCSP))&(1<<irq))?1:0;
	SCI|=v<<2;
	return SCI;
}

static void ResetInterrupts(struct _SCSP *SCSP)
{
	UINT32 reset = SCSP->udata.data[0x22/2];
	if (reset & 0x40)
		SCSP->Int68kCB(SCSP->cpu, -SCSP->IrqTimA);
	if (reset & 0x180)
		SCSP->Int68kCB(SCSP->cpu, -SCSP->IrqTimBC);
}

static void CheckPendingIRQ(struct _SCSP *SCSP)
{
	UINT32 pend=SCSP->udata.data[0x20/2];
	UINT32 en=SCSP->udata.data[0x1e/2];
	if(SCSP->MidiW!=SCSP->MidiR)
	{
		SCSP->Int68kCB(SCSP->cpu, SCSP->IrqMidi);
		return;
	}
	if(!pend)
		return;
	if(pend&0x40)
		if(en&0x40)
		{
			SCSP->Int68kCB(SCSP->cpu, SCSP->IrqTimA);
			return;
		}
	if(pend&0x80)
		if(en&0x80)
		{
			SCSP->Int68kCB(SCSP->cpu, SCSP->IrqTimBC);
			return;
		}
	if(pend&0x100)
		if(en&0x100)
		{
			SCSP->Int68kCB(SCSP->cpu, SCSP->IrqTimBC);
			return;
		}

	SCSP->Int68kCB(SCSP->cpu, 0);
}

static int Get_AR(struct _SCSP *SCSP,int base,int R)
{
	int Rate=base+(R<<1);
	if(Rate>63)	Rate=63;
	if(Rate<0) Rate=0;
	return SCSP->ARTABLE[Rate];
}

static int Get_DR(struct _SCSP *SCSP,int base,int R)
{
	int Rate=base+(R<<1);
	if(Rate>63)	Rate=63;
	if(Rate<0) Rate=0;
	return SCSP->DRTABLE[Rate];
}

static int Get_RR(struct _SCSP *SCSP,int base,int R)
{
	int Rate=base+(R<<1);
	if(Rate>63)	Rate=63;
	if(Rate<0) Rate=0;
	return SCSP->DRTABLE[Rate];
}

static void Compute_EG(struct _SCSP *SCSP,struct _SLOT *slot)
{
	int octave=OCT(slot);
	int rate;
	if(octave&8) octave=octave-16;
	if(KRS(slot)!=0xf)
		rate=octave+2*KRS(slot)+((FNS(slot)>>9)&1);
	else
		rate=0; //rate=((FNS(slot)>>9)&1);

	slot->EG.volume=0x17F<<EG_SHIFT;
	slot->EG.AR=Get_AR(SCSP,rate,AR(slot));
	slot->EG.D1R=Get_DR(SCSP,rate,D1R(slot));
	slot->EG.D2R=Get_DR(SCSP,rate,D2R(slot));
	slot->EG.RR=Get_RR(SCSP,rate,RR(slot));
	slot->EG.DL=0x1f-DL(slot);
	slot->EG.EGHOLD=EGHOLD(slot);
}

static void SCSP_StopSlot(struct _SLOT *slot,int keyoff);

static int EG_Update(struct _SLOT *slot)
{
	switch(slot->EG.state)
	{
		case ATTACK:
			slot->EG.volume+=slot->EG.AR;
			if(slot->EG.volume>=(0x3ff<<EG_SHIFT))
			{
				if (!LPSLNK(slot))
				{
					slot->EG.state=DECAY1;
					if(slot->EG.D1R>=(1024<<EG_SHIFT)) //Skip DECAY1, go directly to DECAY2
						slot->EG.state=DECAY2;
				}
				slot->EG.volume=0x3ff<<EG_SHIFT;
			}
			if(slot->EG.EGHOLD)
				return 0x3ff<<(SHIFT-10);
			break;
		case DECAY1:
			slot->EG.volume-=slot->EG.D1R;
			if(slot->EG.volume<=0)
				slot->EG.volume=0;
			if(slot->EG.volume>>(EG_SHIFT+5)<=slot->EG.DL)
				slot->EG.state=DECAY2;
			break;
		case DECAY2:
			if(D2R(slot)==0)
				return (slot->EG.volume>>EG_SHIFT)<<(SHIFT-10);
			slot->EG.volume-=slot->EG.D2R;
			if(slot->EG.volume<=0)
				slot->EG.volume=0;

			break;
		case RELEASE:
			slot->EG.volume-=slot->EG.RR;
			if(slot->EG.volume<=0)
			{
				slot->EG.volume=0;
				SCSP_StopSlot(slot,0);
				//slot->EG.volume=0x17F<<EG_SHIFT;
				//slot->EG.state=ATTACK;
			}
			break;
		default:
			return 1<<SHIFT;
	}
	return (slot->EG.volume>>EG_SHIFT)<<(SHIFT-10);
}

static UINT32 SCSP_Step(struct _SLOT *slot)
{
	int octave=OCT(slot);
	UINT64 Fn;

	Fn=(FNS_Table[FNS(slot)]);	//24.8
	if(octave&8)
		Fn>>=(16-octave);
	else
		Fn<<=octave;

	return Fn/(44100);
}


static void Compute_LFO(struct _SLOT *slot)
{
	if(PLFOS(slot)!=0)
		LFO_ComputeStep(&(slot->PLFO),LFOF(slot),PLFOWS(slot),PLFOS(slot),0);
	if(ALFOS(slot)!=0)
		LFO_ComputeStep(&(slot->ALFO),LFOF(slot),ALFOWS(slot),ALFOS(slot),1);
}

static void SCSP_StartSlot(struct _SCSP *SCSP, struct _SLOT *slot)
{
	UINT32 start_offset;
	slot->active=1;
	slot->Backwards=0;
	slot->cur_addr=0; slot->nxt_addr=1<<SHIFT;
	start_offset = PCM8B(slot) ? SA(slot) : SA(slot) & 0x7FFFE;
	slot->base=SCSP->SCSPRAM + start_offset;
	slot->step=SCSP_Step(slot);
	Compute_EG(SCSP,slot);
	slot->EG.state=ATTACK;
	slot->EG.volume=0x17F<<EG_SHIFT;
	slot->Prev=0;
	Compute_LFO(slot);

//	printf("StartSlot: SA %x PCM8B %x LPCTL %x ALFOS %x STWINH %x TL %x EFSDL %x\n", SA(slot), PCM8B(slot), LPCTL(slot), ALFOS(slot), STWINH(slot), TL(slot), EFSDL(slot));
//	printf("           AR %x D1R %x D2R %x RR %x DL %x KRS %x EGHOLD %x LPSLNK %x\n", AR(slot), D1R(slot), D2R(slot), RR(slot), DL(slot), KRS(slot), EGHOLD(slot), LPSLNK(slot));
}

static void SCSP_StopSlot(struct _SLOT *slot,int keyoff)
{
	if(keyoff /*&& slot->EG.state!=RELEASE*/)
	{
		slot->EG.state=RELEASE;
	}
	else
	{
		slot->active=0;
	}
	slot->udata.data[0]&=~0x800;
}

#define log_base_2(n) (log((float) n)/log((float) 2))

static void SCSP_Init(struct _SCSP *SCSP, const struct SCSPinterface *intf)
{
	int i=0;

	SCSP->IrqTimA = SCSP->IrqTimBC = SCSP->IrqMidi = 0;
	SCSP->MidiR=SCSP->MidiW=0;
	SCSP->MidiOutR=SCSP->MidiOutW=0;

	// get SCSP RAM
	{
		memset(SCSP,0,sizeof(*SCSP));
        SCSP->cpu = intf->cpu;

		if (!i)
		{
			SCSP->Master=1;
		}
		else
		{
			SCSP->Master=0;
		}

		if (intf->region)
		{
			SCSP->SCSPRAM = (unsigned char *)intf->region[0];
			SCSP->SCSPRAM_LENGTH = 512*1024;
			SCSP->DSP.SCSPRAM = (UINT16 *)SCSP->SCSPRAM;
			SCSP->DSP.SCSPRAM_LENGTH =  (512*1024)/2;
//			SCSP->SCSPRAM += intf->roffset;
		}
	}

	for(i=0;i<0x400;++i)
	{
		float fcent=(double) 1200.0*log_base_2((double)(((double) 1024.0+(double)i)/(double)1024.0));
		fcent=(double) 44100.0*pow(2.0,fcent/1200.0);
		FNS_Table[i]=(float) (1<<SHIFT) *fcent;
	}

	for(i=0;i<0x400;++i)
	{
		float envDB=((float)(3*(i-0x3ff)))/32.0;
		float scale=(float)(1<<SHIFT);
		EG_TABLE[i]=(INT32)(pow(10.0,envDB/20.0)*scale);
	}

	for(i=0;i<0x10000;++i)
	{
		int iTL =(i>>0x0)&0xff;
		int iPAN=(i>>0x8)&0x1f;
		int iSDL=(i>>0xD)&0x07;
		float TL=1.0;
		float SegaDB=0;
		float fSDL=1.0;
		float PAN=1.0;
		float LPAN,RPAN;

		if(iTL&0x01) SegaDB-=0.4;
		if(iTL&0x02) SegaDB-=0.8;
		if(iTL&0x04) SegaDB-=1.5;
		if(iTL&0x08) SegaDB-=3;
		if(iTL&0x10) SegaDB-=6;
		if(iTL&0x20) SegaDB-=12;
		if(iTL&0x40) SegaDB-=24;
		if(iTL&0x80) SegaDB-=48;

		TL=pow(10.0,SegaDB/20.0);

		SegaDB=0;
		if(iPAN&0x1) SegaDB-=3;
		if(iPAN&0x2) SegaDB-=6;
		if(iPAN&0x4) SegaDB-=12;
		if(iPAN&0x8) SegaDB-=24;

		if((iPAN&0xf)==0xf) PAN=0.0;
		else PAN=pow(10.0,SegaDB/20.0);

		if(iPAN<0x10)
		{
			LPAN=PAN;
			RPAN=1.0;
		}
		else
		{
			RPAN=PAN;
			LPAN=1.0;
		}

		if(iSDL)
			fSDL=pow(10.0,(SDLT[iSDL])/20.0);
		else
			fSDL=0.0;

		SCSP->LPANTABLE[i]=FIX((4.0*LPAN*TL*fSDL));
		SCSP->RPANTABLE[i]=FIX((4.0*RPAN*TL*fSDL));
	}

	SCSP->ARTABLE[0]=SCSP->DRTABLE[0]=0;	//Infinite time
	SCSP->ARTABLE[1]=SCSP->DRTABLE[1]=0;	//Infinite time
	for(i=2;i<64;++i)
	{
		double t,step,scale;
		t=ARTimes[i];	//In ms
		if(t!=0.0)
		{
			step=(1023*1000.0)/((float) 44100.0f*t);
			scale=(double) (1<<EG_SHIFT);
			SCSP->ARTABLE[i]=(int) (step*scale);
		}
		else
			SCSP->ARTABLE[i]=1024<<EG_SHIFT;

		t=DRTimes[i];	//In ms
		step=(1023*1000.0)/((float) 44100.0f*t);
		scale=(double) (1<<EG_SHIFT);
		SCSP->DRTABLE[i]=(int) (step*scale);
	}

	// make sure all the slots are off
	for(i=0;i<32;++i)
	{
		SCSP->Slots[i].slot=i;
		SCSP->Slots[i].active=0;
		SCSP->Slots[i].base=NULL;
	}

	LFO_Init();
	SCSP->buffertmpl=(signed int*) malloc(44100*sizeof(signed int));
	SCSP->buffertmpr=(signed int*) malloc(44100*sizeof(signed int));
	memset(SCSP->buffertmpl,0,44100*sizeof(signed int));
	memset(SCSP->buffertmpr,0,44100*sizeof(signed int));

	// no "pend"
	SCSP[0].udata.data[0x20/2] = 0;
	//SCSP[1].udata.data[0x20/2] = 0;
	SCSP->TimCnt[0] = 0xffff;
	SCSP->TimCnt[1] = 0xffff;
	SCSP->TimCnt[2] = 0xffff;
}

static void SCSP_UpdateSlotReg(struct _SCSP *SCSP,int s,int r)
{
	struct _SLOT *slot=SCSP->Slots+s;
	int sl;
	switch(r&0x3f)
	{
		case 0:
		case 1:
			if(KEYONEX(slot))
			{
				for(sl=0;sl<32;++sl)
				{
					struct _SLOT *s2=SCSP->Slots+sl;
					{
						if(KEYONB(s2) && s2->EG.state==RELEASE/*&& !s2->active*/)
						{
							SCSP_StartSlot(SCSP, s2);
						}
						if(!KEYONB(s2) /*&& s2->active*/)
						{
							SCSP_StopSlot(s2,1);
						}
					}
				}
				slot->udata.data[0]&=~0x1000;
			}
			break;
		case 0x10:
		case 0x11:
			slot->step=SCSP_Step(slot);
			break;
		case 0xA:
		case 0xB:
			slot->EG.RR=Get_RR(SCSP,0,RR(slot));
			slot->EG.DL=0x1f-DL(slot);
			break;
		case 0x12:
		case 0x13:
			Compute_LFO(slot);
			break;
	}
}

static void SCSP_UpdateReg(struct _SCSP *SCSP, int reg)
{
	switch(reg&0x3f)
	{
		case 0x2:
		case 0x3:
			{
				unsigned int v=RBL(SCSP);
				SCSP->DSP.RBP=RBP(SCSP);
				if(v==0)
					SCSP->DSP.RBL=8*1024;
				else if(v==1)
					SCSP->DSP.RBL=16*1024;
				if(v==2)
					SCSP->DSP.RBL=32*1024;
				if(v==3)
					SCSP->DSP.RBL=64*1024;
			}
			break;
		case 0x6:
		case 0x7:
			SCSP_MidiIn(SCSP, 0, SCSP->udata.data[0x6/2]&0xff, 0);
			break;
		case 0x12:
		case 0x13:
		case 0x14:
		case 0x15:
		case 0x16:
		case 0x17:
			break;
		case 0x18:
		case 0x19:
			if(SCSP->Master)
			{
				SCSP->TimPris[0]=1<<((SCSP->udata.data[0x18/2]>>8)&0x7);
				SCSP->TimCnt[0]=(SCSP->udata.data[0x18/2]&0xff)<<8;
			}
			break;
		case 0x1a:
		case 0x1b:
			if(SCSP->Master)
			{
				SCSP->TimPris[1]=1<<((SCSP->udata.data[0x1A/2]>>8)&0x7);
				SCSP->TimCnt[1]=(SCSP->udata.data[0x1A/2]&0xff)<<8;
			}
			break;
		case 0x1C:
		case 0x1D:
			if(SCSP->Master)
			{
				SCSP->TimPris[2]=1<<((SCSP->udata.data[0x1C/2]>>8)&0x7);
				SCSP->TimCnt[2]=(SCSP->udata.data[0x1C/2]&0xff)<<8;
			}
			break;
		case 0x22:	//SCIRE
		case 0x23:

			if(SCSP->Master)
			{
				SCSP->udata.data[0x20/2]&=~SCSP->udata.data[0x22/2];
				ResetInterrupts(SCSP);

				// behavior from real hardware: if you SCIRE a timer that's expired,
				// it'll immediately pop up again.  ask Sakura Taisen on the Saturn...
				if (SCSP->TimCnt[0] >= 0xff00)
				{
					SCSP->udata.data[0x20/2] |= 0x40;
				}
				if (SCSP->TimCnt[1] >= 0xff00)
				{
					SCSP->udata.data[0x20/2] |= 0x80;
				}
				if (SCSP->TimCnt[2] >= 0xff00)
				{
					SCSP->udata.data[0x20/2] |= 0x100;
				}
			}
			break;
		case 0x24:
		case 0x25:
		case 0x26:
		case 0x27:
		case 0x28:
		case 0x29:
			if(SCSP->Master)
			{
				SCSP->IrqTimA=DecodeSCI(SCSP,SCITMA);
				SCSP->IrqTimBC=DecodeSCI(SCSP,SCITMB);
				SCSP->IrqMidi=DecodeSCI(SCSP,SCIMID);
			}
			break;
	}
}

static void SCSP_UpdateSlotRegR(struct _SCSP *SCSP, int slot,int reg)
{

}

static void SCSP_UpdateRegR(struct _SCSP *SCSP, int reg)
{
	switch(reg&0x3f)
	{
		case 4:
		case 5:
			{
				unsigned short v=SCSP->udata.data[0x5/2];
				v&=0xff00;
				v|=SCSP->MidiStack[SCSP->MidiR];
				SCSP->Int68kCB(SCSP->cpu, 0);	// cancel the IRQ
				if(SCSP->MidiR!=SCSP->MidiW)
				{
					++SCSP->MidiR;
					SCSP->MidiR&=15;
				}
				SCSP->udata.data[0x5/2]=v;
			}
			break;
		case 8:
		case 9:
			{
				unsigned char slot=SCSP->udata.data[0x8/2]>>11;
				unsigned int CA=SCSP->Slots[slot&0x1f].cur_addr>>(SHIFT+12);
				SCSP->udata.data[0x8/2]&=~(0x780);
				SCSP->udata.data[0x8/2]|=CA<<7;
			}
			break;
	}
}

static void SCSP_w16(struct _SCSP *SCSP,unsigned int addr,unsigned short val)
{
	addr&=0xffff;
	if(addr<0x400)
	{
		int slot=addr/0x20;
		addr&=0x1f;
		*((unsigned short *) (SCSP->Slots[slot].udata.datab+(addr))) = val;
		SCSP_UpdateSlotReg(SCSP,slot,addr&0x1f);
	}
	else if(addr<0x600)
	{
		if (addr < 0x430)
		{
			*((unsigned short *) (SCSP->udata.datab+((addr&0x3f)))) = val;
			SCSP_UpdateReg(SCSP, addr&0x3f);
		}
	}
	else if(addr<0x700)
		SCSP->RINGBUF[(addr-0x600)/2]=val;
	else
	{
		//DSP
		if(addr<0x780)	//COEF
			*((unsigned short *) (SCSP->DSP.COEF+(addr-0x700)/2))=val;
		else if(addr<0x800)
			*((unsigned short *) (SCSP->DSP.MADRS+(addr-0x780)/2))=val;
		else if(addr<0xC00)
			*((unsigned short *) (SCSP->DSP.MPRO+(addr-0x800)/2))=val;

		if(addr==0xBF0)
		{
//			printf("DSP start\n");
			SCSPDSP_Start(&SCSP->DSP);
		}
	}
}

static unsigned short SCSP_r16(struct _SCSP *SCSP, unsigned int addr)
{
	unsigned short v=0;
	addr&=0xffff;
	if(addr<0x400)
	{
		int slot=addr/0x20;
		addr&=0x1f;
		SCSP_UpdateSlotRegR(SCSP, slot,addr&0x1f);
		v=*((unsigned short *) (SCSP->Slots[slot].udata.datab+(addr)));
	}
	else if(addr<0x600)
	{
		if (addr < 0x430)
		{
			SCSP_UpdateRegR(SCSP, addr&0x3f);
			v= *((unsigned short *) (SCSP->udata.datab+((addr&0x3f))));
		}
	}
	else if(addr<0x700)
		v=SCSP->RINGBUF[(addr-0x600)/2];
	return v;
}


#define REVSIGN(v) ((~v)+1)

void SCSP_TimersAddTicks(struct _SCSP *SCSP, int ticks)
{
	if(SCSP->TimCnt[0]<=0xff00)
	{
 		SCSP->TimCnt[0] += ticks << (8-((SCSP->udata.data[0x18/2]>>8)&0x7));
		if (SCSP->TimCnt[0] > 0xFF00)
		{
			SCSP->TimCnt[0] = 0xFFFF;
			SCSP->udata.data[0x20/2]|=0x40;
		}
		SCSP->udata.data[0x18/2]&=0xff00;
		SCSP->udata.data[0x18/2]|=SCSP->TimCnt[0]>>8;
	}

	if(SCSP->TimCnt[1]<=0xff00)
	{
		SCSP->TimCnt[1] += ticks << (8-((SCSP->udata.data[0x1a/2]>>8)&0x7));
		if (SCSP->TimCnt[1] > 0xFF00)
		{
			SCSP->TimCnt[1] = 0xFFFF;
			SCSP->udata.data[0x20/2]|=0x80;
		}
		SCSP->udata.data[0x1a/2]&=0xff00;
		SCSP->udata.data[0x1a/2]|=SCSP->TimCnt[1]>>8;
	}

	if(SCSP->TimCnt[2]<=0xff00)
	{
		SCSP->TimCnt[2] += ticks << (8-((SCSP->udata.data[0x1c/2]>>8)&0x7));
		if (SCSP->TimCnt[2] > 0xFF00)
		{
			SCSP->TimCnt[2] = 0xFFFF;
			SCSP->udata.data[0x20/2]|=0x100;
		}
		SCSP->udata.data[0x1c/2]&=0xff00;
		SCSP->udata.data[0x1c/2]|=SCSP->TimCnt[2]>>8;
	}
}

static INLINE INT32 SCSP_UpdateSlot(struct _SCSP *SCSP, struct _SLOT *slot)
{
	INT32 sample;
	int step=slot->step;
	UINT32 addr1,addr2,addr_select;                                   // current and next sample addresses
	UINT32 *addr[2]      = {&addr1, &addr2};                          // used for linear interpolation
	UINT32 *slot_addr[2] = {&(slot->cur_addr), &(slot->nxt_addr)};    //

	if(SSCTL(slot)!=0)	//no FM or noise yet
		return 0;

	if(PLFOS(slot)!=0)
	{
		step=step*PLFO_Step(&(slot->PLFO));
		step>>=SHIFT;
	}

	if(PCM8B(slot))
	{
		addr1=slot->cur_addr>>SHIFT;
		addr2=slot->nxt_addr>>SHIFT;
	}
	else
	{
		addr1=(slot->cur_addr>>(SHIFT-1))&0x7fffe;
		addr2=(slot->nxt_addr>>(SHIFT-1))&0x7fffe;
	}

	if(MDL(slot)!=0 || MDXSL(slot)!=0 || MDYSL(slot)!=0)
	{
		INT32 smp=(SCSP->RINGBUF[(SCSP->BUFPTR+MDXSL(slot))&63]+SCSP->RINGBUF[(SCSP->BUFPTR+MDYSL(slot))&63])/2;

		smp<<=0xA; // associate cycle with 1024
		smp>>=0x1A-MDL(slot); // ex. for MDL=0xF, sample range corresponds to +/- 64 pi (32=2^5 cycles) so shift by 11 (16-5 == 0x1A-0xF)
		if(!PCM8B(slot)) smp<<=1;

		addr1+=smp; addr2+=smp;
	}

	if(PCM8B(slot))	//8 bit signed
	{
		INT8 *p1=(signed char *) (SCSP->SCSPRAM+(((SA(slot)+addr1)^1)&0x7FFFF));
		INT8 *p2=(signed char *) (SCSP->SCSPRAM+(((SA(slot)+addr2)^1)&0x7FFFF));
		//sample=(p[0])<<8;
		INT32 s;
		INT32 fpart=slot->cur_addr&((1<<SHIFT)-1);
		s=(int) (p1[0]<<8)*((1<<SHIFT)-fpart)+(int) (p2[0]<<8)*fpart;
		sample=(s>>SHIFT);
	}
	else	//16 bit signed (endianness?)
	{
		INT16 *p1=(signed short *) (SCSP->SCSPRAM+((SA(slot)+addr1)&0x7FFFE));
		INT16 *p2=(signed short *) (SCSP->SCSPRAM+((SA(slot)+addr2)&0x7FFFE));
		//sample=LE16(p[0]);
		INT32 s;
		INT32 fpart=slot->cur_addr&((1<<SHIFT)-1);
		s=(int) LE16(p1[0])*((1<<SHIFT)-fpart)+(int) LE16(p2[0])*fpart;
		sample=(s>>SHIFT);
	}

	if(SBCTL(slot)&0x1)
		sample ^= 0x7FFF;
	if(SBCTL(slot)&0x2)
		sample = (INT16)(sample^0x8000);

	if(slot->Backwards)
		slot->cur_addr-=step;
	else
		slot->cur_addr+=step;
	slot->nxt_addr=slot->cur_addr+(1<<SHIFT);

	addr1=slot->cur_addr>>SHIFT;
	addr2=slot->nxt_addr>>SHIFT;

	if(addr1>=LSA(slot) && !(slot->Backwards))
	{
		if(LPSLNK(slot) && slot->EG.state==ATTACK)
			slot->EG.state = DECAY1;
	}

	for (addr_select=0;addr_select<2;addr_select++)
	{
		INT32 rem_addr;
		switch(LPCTL(slot))
		{
		case 0:	//no loop
			if(*addr[addr_select]>=LSA(slot) && *addr[addr_select]>=LEA(slot))
			{
			//slot->active=0;
			SCSP_StopSlot(slot,0);
			}
			break;
		case 1: //normal loop
			if(*addr[addr_select]>=LEA(slot))
			{
				rem_addr = *slot_addr[addr_select] - (LEA(slot)<<SHIFT);
				*slot_addr[addr_select]=(LSA(slot)<<SHIFT) + rem_addr;
			}
			break;
		case 2:	//reverse loop
			if((*addr[addr_select]>=LSA(slot)) && !(slot->Backwards))
			{
				rem_addr = *slot_addr[addr_select] - (LSA(slot)<<SHIFT);
				*slot_addr[addr_select]=(LEA(slot)<<SHIFT) - rem_addr;
				slot->Backwards=1;
			}
			else if((*addr[addr_select]<LSA(slot) || (*slot_addr[addr_select]&0x80000000)) && slot->Backwards)
			{
				rem_addr = (LSA(slot)<<SHIFT) - *slot_addr[addr_select];
				*slot_addr[addr_select]=(LEA(slot)<<SHIFT) - rem_addr;
			}
			break;
		case 3: //ping-pong
			if(*addr[addr_select]>=LEA(slot)) //reached end, reverse till start
			{
				rem_addr = *slot_addr[addr_select] - (LEA(slot)<<SHIFT);
				*slot_addr[addr_select]=(LEA(slot)<<SHIFT) - rem_addr;
				slot->Backwards=1;
			}
			else if((*addr[addr_select]<LSA(slot) || (*slot_addr[addr_select]&0x80000000)) && slot->Backwards)//reached start or negative
			{
				rem_addr = (LSA(slot)<<SHIFT) - *slot_addr[addr_select];
				*slot_addr[addr_select]=(LSA(slot)<<SHIFT) + rem_addr;
				slot->Backwards=0;
			}
			break;
		}
	}

	if(ALFOS(slot)!=0)
	{
		sample=sample*ALFO_Step(&(slot->ALFO));
		sample>>=SHIFT;
	}

	if(slot->EG.state==ATTACK)
		sample=(sample*EG_Update(slot))>>SHIFT;
	else
		sample=(sample*EG_TABLE[EG_Update(slot)>>(SHIFT-10)])>>SHIFT;

	if(!STWINH(slot))
	{
		unsigned short Enc=((TL(slot))<<0x0)|(0x7<<0xd);
		*SCSP->RBUFDST=(sample*SCSP->LPANTABLE[Enc])>>(SHIFT+1);
	}

	return sample;
}

static void SCSP_DoMasterSamples(struct _SCSP *SCSP, int nsamples)
{
	INT16 *bufr,*bufl;
	int sl, s, i;

	bufr=SCSP->bufferr;
	bufl=SCSP->bufferl;

	for(s=0;s<nsamples;++s)
	{
		INT32 smpl, smpr;

		smpl = smpr = 0;

		for(sl=0;sl<32;++sl)
		{
#if FM_DELAY
			SCSP->RBUFDST=SCSP->DELAYBUF+SCSP->DELAYPTR;
#else
			SCSP->RBUFDST=SCSP->RINGBUF+SCSP->BUFPTR;
#endif
			if(SCSP->Slots[sl].active)
			{
				struct _SLOT *slot=SCSP->Slots+sl;
				unsigned short Enc;
				signed int sample;

				sample=SCSP_UpdateSlot(SCSP, slot);

				Enc=((TL(slot))<<0x0)|((IMXL(slot))<<0xd);
				SCSPDSP_SetSample(&SCSP->DSP,(sample*SCSP->LPANTABLE[Enc])>>(SHIFT-2),ISEL(slot),IMXL(slot));
				Enc=((TL(slot))<<0x0)|((DIPAN(slot))<<0x8)|((DISDL(slot))<<0xd);
				{
					smpl+=(sample*SCSP->LPANTABLE[Enc])>>SHIFT;
					smpr+=(sample*SCSP->RPANTABLE[Enc])>>SHIFT;
				}
			}

#if FM_DELAY
			SCSP->RINGBUF[(SCSP->BUFPTR+64-(FM_DELAY-1))&63] = SCSP->DELAYBUF[(SCSP->DELAYPTR+FM_DELAY-(FM_DELAY-1))%FM_DELAY];
#endif
			++SCSP->BUFPTR;
			SCSP->BUFPTR&=63;
#if FM_DELAY
			++SCSP->DELAYPTR;
			if(SCSP->DELAYPTR>FM_DELAY-1) SCSP->DELAYPTR=0;
#endif
		}

		SCSPDSP_Step(&SCSP->DSP);

		for(i=0;i<16;++i)
		{
			struct _SLOT *slot=SCSP->Slots+i;
			if(EFSDL(slot))
			{
				unsigned short Enc=((EFPAN(slot))<<0x8)|((EFSDL(slot))<<0xd);
				smpl+=(SCSP->DSP.EFREG[i]*SCSP->LPANTABLE[Enc])>>SHIFT;
				smpr+=(SCSP->DSP.EFREG[i]*SCSP->RPANTABLE[Enc])>>SHIFT;
			}
		}

		*bufl++ = ICLIP16(smpl>>2);
		*bufr++ = ICLIP16(smpr>>2);

		SCSP_TimersAddTicks(SCSP, 1);
		CheckPendingIRQ(SCSP);
	}

}

static void dma_scsp(struct _SCSP *SCSP)
{
	static UINT16 tmp_dma[3], *scsp_regs;

	scsp_regs = (UINT16 *)SCSP->udata.datab;

	printf("SCSP: DMA transfer START\n"
			 "DMEA: %04x DRGA: %04x DTLG: %04x\n"
			 "DGATE: %d  DDIR: %d\n",SCSP->scsp_dmea,SCSP->scsp_drga,SCSP->scsp_dtlg,scsp_dgate ? 1 : 0,scsp_ddir ? 1 : 0);

	/* Copy the dma values in a temp storage for resuming later *
     * (DMA *can't* overwrite his parameters).                  */
	if(!(scsp_ddir))
	{
		tmp_dma[0] = scsp_regs[0x12/2];
		tmp_dma[1] = scsp_regs[0x14/2];
		tmp_dma[2] = scsp_regs[0x16/2];
	}

	if(scsp_ddir)
	{
		for(;SCSP->scsp_dtlg > 0;SCSP->scsp_dtlg-=2)
		{
//			program_write_word(SCSP->scsp_dmea, program_read_word(0x100000|SCSP->scsp_drga));
			SCSP->scsp_dmea+=2;
			SCSP->scsp_drga+=2;
		}
	}
	else
	{
		for(;SCSP->scsp_dtlg > 0;SCSP->scsp_dtlg-=2)
		{
//  			program_write_word(0x100000|SCSP->scsp_drga,program_read_word(SCSP->scsp_dmea));
			SCSP->scsp_dmea+=2;
			SCSP->scsp_drga+=2;
		}
	}

	/*Resume the values*/
	if(!(scsp_ddir))
	{
	 	scsp_regs[0x12/2] = tmp_dma[0];
		scsp_regs[0x14/2] = tmp_dma[1];
		scsp_regs[0x16/2] = tmp_dma[2];
	}

	/*Job done,request a dma end irq*/
//	if(scsp_regs[0x1e/2] & 0x10)
//		cpunum_set_input_line(2,dma_transfer_end,HOLD_LINE);
}

int SCSP_IRQCB(void *param)
{
	CheckPendingIRQ(param);
	return -1;
}

void SCSP_Update(void *param, INT16 **inputs, INT16 **buf, int samples)
{
	struct _SCSP *SCSP = param;
	SCSP->bufferl = buf[0];
	SCSP->bufferr = buf[1];
	SCSP->length = samples;
	SCSP_DoMasterSamples(SCSP, samples);
}

void *SCSP_Start(const void *config)
{
	const struct SCSPinterface *intf;

	struct _SCSP *SCSP;

	SCSP = malloc(sizeof(*SCSP));
	memset(SCSP, 0, sizeof(*SCSP));

	intf = config;

	// init the emulation
	SCSP_Init(SCSP, intf);

	// set up the IRQ callbacks
	{
		SCSP->Int68kCB = intf->irq_callback[0];

//		SCSP->stream = stream_create(0, 2, 44100, SCSP, SCSP_Update);
	}

	return SCSP;
}

void SCSP_Exit (void *param) {
    if (param) {
        struct _SCSP *SCSP = param;
        if (SCSP->buffertmpl) {
            free (SCSP->buffertmpl);
        }
        if (SCSP->buffertmpr) {
            free (SCSP->buffertmpr);
        }
        free (param);
    }
}


void SCSP_set_ram_base(struct _SCSP *SCSP, int which, void *base)
{
	if (SCSP)
	{
		SCSP->SCSPRAM = base;
		SCSP->DSP.SCSPRAM = base;
	}
}


READ16_HANDLER( SCSP_0_r )
{
	return SCSP_r16(param, offset*2);
}

WRITE16_HANDLER( SCSP_0_w )
{
	struct _SCSP *SCSP = param;
	UINT16 tmp, *scsp_regs;

	tmp = SCSP_r16(SCSP, offset*2);
	COMBINE_DATA(&tmp);
	SCSP_w16(SCSP,offset*2, tmp);

	scsp_regs = (UINT16 *)SCSP->udata.datab;

	switch(offset*2)
	{
		// check DMA
		case 0x412:
		/*DMEA [15:1]*/
		/*Sound memory address*/
		SCSP->scsp_dmea = (((scsp_regs[0x14/2] & 0xf000)>>12)*0x10000) | (scsp_regs[0x12/2] & 0xfffe);
		break;
		case 0x414:
		/*DMEA [19:16]*/
		SCSP->scsp_dmea = (((scsp_regs[0x14/2] & 0xf000)>>12)*0x10000) | (scsp_regs[0x12/2] & 0xfffe);
		/*DRGA [11:1]*/
		/*Register memory address*/
		SCSP->scsp_drga = scsp_regs[0x14/2] & 0x0ffe;
		break;
		case 0x416:
		/*DGATE[14]*/
		/*DDIR[13]*/
		/*if 0 sound_mem -> reg*/
		/*if 1 sound_mem <- reg*/
		/*DEXE[12]*/
		/*starting bit*/
		/*DTLG[11:1]*/
		/*size of transfer*/
		SCSP->scsp_dtlg = scsp_regs[0x16/2] & 0x0ffe;
		if(scsp_dexe)
		{
			dma_scsp(SCSP);
			scsp_regs[0x16/2]^=0x1000;//disable starting bit
		}
		break;
		//check main cpu IRQ
		case 0x42a:
#if 0
        extern UINT32* stv_scu;

			if(stv_scu && !(stv_scu[40] & 0x40) /*&& scsp_regs[0x42c/2] & 0x20*/)/*Main CPU allow sound irq*/
			{
				cpunum_set_input_line_and_vector(0, 9, HOLD_LINE , 0x46);
			    logerror("SCSP: Main CPU interrupt\n");
			}
#endif
		break;
		case 0x42c:
		break;
		case 0x42e:
		break;
	}
}

WRITE16_HANDLER( SCSP_MidiIn )
{
	struct _SCSP *SCSP = param;
	SCSP->MidiStack[SCSP->MidiW++]=data;
	SCSP->MidiW &= 15;
}

READ16_HANDLER( SCSP_MidiOutR )
{
	struct _SCSP *SCSP = param;
	unsigned char val;

	val=SCSP->MidiStack[SCSP->MidiR++];
	SCSP->MidiR&=7;
	return val;
}

