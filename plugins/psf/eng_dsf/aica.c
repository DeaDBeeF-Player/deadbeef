/*
    Sega/Yamaha AICA emulation
    By ElSemi, kingshriek, and R. Belmont

    This is effectively a 64-voice SCSP, with the following differences:
    - No FM mode
    - A third sample format (ADPCM) has been added
    - Some minor other tweeks
*/

#include <math.h>
#include <string.h>
#include "ao.h"
#include "cpuintrf.h"
#include "aica.h"
#include "aicadsp.h"
#include "dc_hw.h"

#define ICLIP16(x) (x<-32768)?-32768:((x>32767)?32767:x)

#define SHIFT	12
#define FIX(v)	((UINT32) ((float) (1<<SHIFT)*(v)))


#define EG_SHIFT	16

#define USEDSP

// include the LFO handling code
#include "aicalfo.c"

/*
    AICA features 64 programmable slots
    that can generate PCM and ADPCM (from ROM/RAM) sound
*/

//SLOT PARAMETERS
#define KEYONEX(slot)		((slot->udata.data[0x0]>>0x0)&0x8000)
#define KEYONB(slot)		((slot->udata.data[0x0]>>0x0)&0x4000)
#define SSCTL(slot)		((slot->udata.data[0x0]>>0xA)&0x0001)
#define LPCTL(slot)		((slot->udata.data[0x0]>>0x9)&0x0001)
#define PCMS(slot)		((slot->udata.data[0x0]>>0x7)&0x0003)

#define SA(slot)		(((slot->udata.data[0x0]&0x7F)<<16)|(slot->udata.data[0x4/2]))

#define LSA(slot)		(slot->udata.data[0x8/2])

#define LEA(slot)		(slot->udata.data[0xc/2])

#define D2R(slot)		((slot->udata.data[0x10/2]>>0xB)&0x001F)
#define D1R(slot)		((slot->udata.data[0x10/2]>>0x6)&0x001F)
#define AR(slot)		((slot->udata.data[0x10/2]>>0x0)&0x001F)

#define LPSLNK(slot)		((slot->udata.data[0x14/2]>>0x0)&0x4000)
#define KRS(slot)		((slot->udata.data[0x14/2]>>0xA)&0x000F)
#define DL(slot)		((slot->udata.data[0x14/2]>>0x5)&0x001F)
#define RR(slot)		((slot->udata.data[0x14/2]>>0x0)&0x001F)

#define TL(slot)		((slot->udata.data[0x28/2]>>0x8)&0x00FF)

#define OCT(slot)		((slot->udata.data[0x18/2]>>0xB)&0x000F)
#define FNS(slot)		((slot->udata.data[0x18/2]>>0x0)&0x03FF)

#define LFORE(slot)		((slot->udata.data[0x1c/2]>>0x0)&0x8000)
#define LFOF(slot)		((slot->udata.data[0x1c/2]>>0xA)&0x001F)
#define PLFOWS(slot)		((slot->udata.data[0x1c/2]>>0x8)&0x0003)
#define PLFOS(slot)		((slot->udata.data[0x1c/2]>>0x5)&0x0007)
#define ALFOWS(slot)		((slot->udata.data[0x1c/2]>>0x3)&0x0003)
#define ALFOS(slot)		((slot->udata.data[0x1c/2]>>0x0)&0x0007)

#define ISEL(slot)		((slot->udata.data[0x20/2]>>0x0)&0x000F)
#define IMXL(slot)		((slot->udata.data[0x20/2]>>0x4)&0x000F)

#define DISDL(slot)		((slot->udata.data[0x24/2]>>0x8)&0x000F)
#define DIPAN(slot)		((slot->udata.data[0x24/2]>>0x0)&0x001F)

#define EFSDL(slot)		((AICA->EFSPAN[slot*4]>>8)&0x000f)
#define EFPAN(slot)		((AICA->EFSPAN[slot*4]>>0)&0x001f) 

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
	UINT8 LPLINK;
};

struct _SLOT
{
	union
	{
		UINT16 data[0x40];	//only 0x1a bytes used
		UINT8 datab[0x80];
	} udata;
	UINT8 active;	//this slot is currently playing
	UINT8 *base;		//samples base address
	UINT32 prv_addr;    // previous play address (for ADPCM)
	UINT32 cur_addr;	//current play address (24.8)
	UINT32 nxt_addr;	//next play address
	UINT32 step;		//pitch step (24.8)
	UINT8 Backwards;	//the wave is playing backwards
	struct _EG EG;			//Envelope
	struct _EG FEG;			//filter envelope
	struct _LFO PLFO;		//Phase LFO
	struct _LFO ALFO;		//Amplitude LFO
	int slot;
	int cur_sample;       //current ADPCM sample
	int cur_quant;        //current ADPCM step
	int curstep;
	int cur_lpquant, cur_lpsample, cur_lpstep;
	UINT8 *adbase, *adlpbase;
	UINT8 mslc;			// monitored?
};


#define MEM4B(aica)		((aica->udata.data[0]>>0x0)&0x0200)
#define DAC18B(aica)		((aica->udata.data[0]>>0x0)&0x0100)
#define MVOL(aica)		((aica->udata.data[0]>>0x0)&0x000F)
#define RBL(aica)		((aica->udata.data[2]>>0xD)&0x0003)
#define RBP(aica)		((aica->udata.data[2]>>0x0)&0x0fff)
#define MOFULL(aica)   		((aica->udata.data[4]>>0x0)&0x1000)
#define MOEMPTY(aica)		((aica->udata.data[4]>>0x0)&0x0800)
#define MIOVF(aica)		((aica->udata.data[4]>>0x0)&0x0400)
#define MIFULL(aica)		((aica->udata.data[4]>>0x0)&0x0200)
#define MIEMPTY(aica)		((aica->udata.data[4]>>0x0)&0x0100)

#define AFSEL(aica)		((aica->udata.data[6]>>0x0)&0x4000)
#define MSLC(aica)		((aica->udata.data[6]>>0x8)&0x3F)

#define SCILV0(aica)    	((aica->udata.data[0xa8/2]>>0x0)&0xff)
#define SCILV1(aica)    	((aica->udata.data[0xac/2]>>0x0)&0xff)
#define SCILV2(aica)    	((aica->udata.data[0xb0/2]>>0x0)&0xff)

#define SCIEX0	0
#define SCIEX1	1
#define SCIEX2	2
#define SCIMID	3
#define SCIDMA	4
#define SCIIRQ	5
#define SCITMA	6
#define SCITMB	7

struct _AICA
{
	union
	{
		UINT16 data[0xc0/2];
		UINT8 datab[0xc0];
	} udata;
	UINT16 IRQL, IRQR;
	UINT16 EFSPAN[0x48];
	struct _SLOT Slots[64];
	signed short RINGBUF[64];
	unsigned char BUFPTR;
	unsigned char *AICARAM;
	UINT32 AICARAM_LENGTH;
	char Master;
	void (*IntARMCB)(struct sARM7 *cpu, int irq);

	INT32 *buffertmpl, *buffertmpr;

	UINT32 IrqTimA;
	UINT32 IrqTimBC;
	UINT32 IrqMidi;

	UINT8 MidiOutW,MidiOutR;
	UINT8 MidiStack[16];
	UINT8 MidiW,MidiR;

	int LPANTABLE[0x20000];
	int RPANTABLE[0x20000];

	int TimPris[3];
	int TimCnt[3];

	// DMA stuff
	UINT32 aica_dmea;
	UINT16 aica_drga;
	UINT16 aica_dtlg;

	int ARTABLE[64], DRTABLE[64];

	struct _AICADSP DSP;

	struct sARM7 *cpu;

    INT16 *bufferl;
    INT16 *bufferr;

    int length;

    signed short *RBUFDST;	//this points to where the sample will be stored in the RingBuf
};

//static struct _AICA *AllocedAICA;

static const float SDLT[16]={-1000000.0,-42.0,-39.0,-36.0,-33.0,-30.0,-27.0,-24.0,-21.0,-18.0,-15.0,-12.0,-9.0,-6.0,-3.0,0.0};

static unsigned char DecodeSCI(struct _AICA *AICA, unsigned char irq)
{
	unsigned char SCI=0;
	unsigned char v;
	v=(SCILV0((AICA))&(1<<irq))?1:0;
	SCI|=v;
	v=(SCILV1((AICA))&(1<<irq))?1:0;
	SCI|=v<<1;
	v=(SCILV2((AICA))&(1<<irq))?1:0;
	SCI|=v<<2;
	return SCI;
}

static void ResetInterrupts(struct _AICA *AICA)
{
#if 0
	UINT32 reset = AICA->udata.data[0xa4/2];
	if (reset & 0x40)
		AICA->IntARMCB(-AICA->IrqTimA);
	if (reset & 0x180)
		AICA->IntARMCB(-AICA->IrqTimBC);
#endif
}

static void CheckPendingIRQ(struct _AICA *AICA)
{
	UINT32 pend=AICA->udata.data[0xa0/2];
	UINT32 en=AICA->udata.data[0x9c/2];
	if(AICA->MidiW!=AICA->MidiR)
	{
		AICA->IRQL = AICA->IrqMidi;
		AICA->IntARMCB(AICA->cpu, 1);
		return;
	}
	if(!pend)
		return;
	if(pend&0x40)
		if(en&0x40)
		{
			AICA->IRQL = AICA->IrqTimA;
			AICA->IntARMCB(AICA->cpu, 1);
			return;
		}
	if(pend&0x80)
		if(en&0x80)
		{
			AICA->IRQL = AICA->IrqTimBC;
			AICA->IntARMCB(AICA->cpu, 1);
			return;
		}
	if(pend&0x100)
		if(en&0x100)
		{
			AICA->IRQL = AICA->IrqTimBC;
			AICA->IntARMCB(AICA->cpu, 1);
			return;
		}
}

static int Get_AR(struct _AICA *AICA,int base,int R)
{
	int Rate=base+(R<<1);
	if(Rate>63)	Rate=63;
	if(Rate<0) Rate=0;
	return AICA->ARTABLE[Rate];
}

static int Get_DR(struct _AICA *AICA,int base,int R)
{
	int Rate=base+(R<<1);
	if(Rate>63)	Rate=63;
	if(Rate<0) Rate=0;
	return AICA->DRTABLE[Rate];
}

static int Get_RR(struct _AICA *AICA,int base,int R)
{
	int Rate=base+(R<<1);
	if(Rate>63)	Rate=63;
	if(Rate<0) Rate=0;
	return AICA->DRTABLE[Rate];
}

static void Compute_EG(struct _AICA *AICA,struct _SLOT *slot)
{
	int octave=OCT(slot);
	int rate;
	if(octave&8) octave=octave-16;
	if(KRS(slot)!=0xf)
		rate=octave+2*KRS(slot)+((FNS(slot)>>9)&1);
	else
		rate=0; //rate=((FNS(slot)>>9)&1);

	slot->EG.volume=0x17f<<EG_SHIFT;
	slot->EG.AR=Get_AR(AICA,rate,AR(slot));
	slot->EG.D1R=Get_DR(AICA,rate,D1R(slot));
	slot->EG.D2R=Get_DR(AICA,rate,D2R(slot));
	slot->EG.RR=Get_RR(AICA,rate,RR(slot));
	slot->EG.DL=0x1f-DL(slot);
}

static void AICA_StopSlot(struct _SLOT *slot,int keyoff);

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
			break;
		case DECAY1:
			slot->EG.volume-=slot->EG.D1R;
			if(slot->EG.volume<=0)
				slot->EG.volume=0;
			if(slot->EG.volume>>(EG_SHIFT+5)<slot->EG.DL)
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
				AICA_StopSlot(slot,0);
//				slot->EG.volume=0x17f<<EG_SHIFT;
//				slot->EG.state=ATTACK;
			}
			break;
		default:
			return 1<<SHIFT;
	}
	return (slot->EG.volume>>EG_SHIFT)<<(SHIFT-10);
}

static UINT32 AICA_Step(struct _SLOT *slot)
{
	int octave=OCT(slot);
	UINT32 Fn;

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
		AICALFO_ComputeStep(&(slot->PLFO),LFOF(slot),PLFOWS(slot),PLFOS(slot),0);
	if(ALFOS(slot)!=0)
		AICALFO_ComputeStep(&(slot->ALFO),LFOF(slot),ALFOWS(slot),ALFOS(slot),1);
}

#define ADPCMSHIFT	8
#define ADFIX(f)	(int) ((float) f*(float) (1<<ADPCMSHIFT))

const int TableQuant[8]={ADFIX(0.8984375),ADFIX(0.8984375),ADFIX(0.8984375),ADFIX(0.8984375),ADFIX(1.19921875),ADFIX(1.59765625),ADFIX(2.0),ADFIX(2.3984375)};
const int quant_mul[16]= { 1, 3, 5, 7, 9, 11, 13, 15, -1, -3, -5, -7, -9, -11, -13, -15};

void InitADPCM(int *PrevSignal, int *PrevQuant)
{
	*PrevSignal=0;
	*PrevQuant=0x7f;
}

static INLINE signed short DecodeADPCM(int *PrevSignal, unsigned char Delta, int *PrevQuant)
{
	int x = *PrevQuant * quant_mul [Delta & 15];
        x = *PrevSignal + ((int)(x + ((UINT32)x >> 29)) >> 3);
	*PrevSignal=ICLIP16(x);
	*PrevQuant=(*PrevQuant*TableQuant[Delta&7])>>ADPCMSHIFT;
	*PrevQuant=(*PrevQuant<0x7f)?0x7f:((*PrevQuant>0x6000)?0x6000:*PrevQuant);
	return *PrevSignal;
}

static void AICA_StartSlot(struct _AICA *AICA, struct _SLOT *slot)
{
	UINT64 start_offset;

	slot->active=1;
	slot->Backwards=0;
	slot->cur_addr=0; slot->nxt_addr=1<<SHIFT; slot->prv_addr=-1;
	start_offset = SA(slot);	// AICA can play 16-bit samples from any boundry
	slot->base=&AICA->AICARAM[start_offset];
	slot->step=AICA_Step(slot);
	Compute_EG(AICA,slot);
	slot->EG.state=ATTACK;
	slot->EG.volume=0x17f<<EG_SHIFT;
	Compute_LFO(slot);

	if (PCMS(slot) >= 2)
	{
		UINT8 *base;
		UINT32 curstep, steps_to_go;

		slot->curstep = 0;
		slot->adbase = (unsigned char *) (AICA->AICARAM+((SA(slot))&0x7fffff));
		InitADPCM(&(slot->cur_sample), &(slot->cur_quant));
		InitADPCM(&(slot->cur_lpsample), &(slot->cur_lpquant));

		// walk to the ADPCM state at LSA
		curstep = 0;
		base = slot->adbase;
		steps_to_go = LSA(slot);

		while (curstep < steps_to_go)
		{
			int shift1, delta1;
			shift1 = 4*((curstep&1));
			delta1 = (*base>>shift1)&0xf;
			DecodeADPCM(&(slot->cur_lpsample),delta1,&(slot->cur_lpquant));
			curstep++;
			if (!(curstep & 1))
			{
				base++;
			}
		}

		slot->cur_lpstep = curstep;
		slot->adlpbase = base;

		// on real hardware this creates undefined behavior.
		if (LSA(slot) > LEA(slot))
		{
			slot->udata.data[0xc/2] = 0xffff;
		}
	}
}

static void AICA_StopSlot(struct _SLOT *slot,int keyoff)
{
	if(keyoff /*&& slot->EG.state!=RELEASE*/)
	{
		slot->EG.state=RELEASE;
	}
	else
	{
		slot->active=0;
	}
	slot->udata.data[0]&=~0x4000;
	
}

#define log_base_2(n) (log((float) n)/log((float) 2))

static void AICA_Init(struct _AICA *AICA, const struct AICAinterface *intf)
{
	int i=0;

	AICA->IrqTimA = AICA->IrqTimBC = AICA->IrqMidi = 0;
	AICA->MidiR=AICA->MidiW=0;
	AICA->MidiOutR=AICA->MidiOutW=0;

	// get AICA RAM
	{
		memset(AICA,0,sizeof(*AICA));

		if (!i)
		{
			AICA->Master=1;
		}
		else
		{
			AICA->Master=0;
		}

		if (intf->region)
		{
			AICA->AICARAM = &intf->cpu->dc_ram[0];
			AICA->AICARAM_LENGTH = 2*1024*1024;
			AICA->DSP.AICARAM = (UINT16 *)AICA->AICARAM;
			AICA->DSP.AICARAM_LENGTH =  (2*1024*1024)/2;
			AICA->cpu = intf->cpu;
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

	for(i=0;i<0x20000;++i)
	{
		int iTL =(i>>0x0)&0xff;
		int iPAN=(i>>0x8)&0x1f;
		int iSDL=(i>>0xD)&0x0F;
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

		AICA->LPANTABLE[i]=FIX((4.0*LPAN*TL*fSDL));
		AICA->RPANTABLE[i]=FIX((4.0*RPAN*TL*fSDL));
	}

	AICA->ARTABLE[0]=AICA->DRTABLE[0]=0;	//Infinite time
	AICA->ARTABLE[1]=AICA->DRTABLE[1]=0;	//Infinite time
	for(i=2;i<64;++i)
	{
		double t,step,scale;
		t=ARTimes[i];	//In ms
		if(t!=0.0)
		{
			step=(1023*1000.0)/((float) 44100.0f*t);
			scale=(double) (1<<EG_SHIFT);
			AICA->ARTABLE[i]=(int) (step*scale);
		}
		else
			AICA->ARTABLE[i]=1024<<EG_SHIFT;

		t=DRTimes[i];	//In ms
		step=(1023*1000.0)/((float) 44100.0f*t);
		scale=(double) (1<<EG_SHIFT);
		AICA->DRTABLE[i]=(int) (step*scale);
	}

	// make sure all the slots are off
	for(i=0;i<64;++i)
	{
		AICA->Slots[i].slot=i;
		AICA->Slots[i].active=0;
		AICA->Slots[i].base=NULL;
		AICA->Slots[i].EG.state=RELEASE;
		AICA->Slots[i].mslc=0;
	}

	AICALFO_Init();
	AICA->buffertmpl=(signed int*) malloc(44100*sizeof(signed int));
	AICA->buffertmpr=(signed int*) malloc(44100*sizeof(signed int));
	memset(AICA->buffertmpl,0,44100*sizeof(signed int));
	memset(AICA->buffertmpr,0,44100*sizeof(signed int));

	// no "pend"
	AICA[0].udata.data[0xa0/2] = 0;
	//AICA[1].udata.data[0x20/2] = 0;
	AICA->TimCnt[0] = 0xffff;
	AICA->TimCnt[1] = 0xffff;
	AICA->TimCnt[2] = 0xffff;
}

static void AICA_UpdateSlotReg(struct _AICA *AICA,int s,int r)
{
	struct _SLOT *slot=AICA->Slots+s;
	int sl;
	switch(r&0x7f)
	{
		case 0:
		case 1:
			if(KEYONEX(slot))
			{
				for(sl=0;sl<64;++sl)
				{
					struct _SLOT *s2=AICA->Slots+sl;
					{
						if(KEYONB(s2) && s2->EG.state==RELEASE/*&& !s2->active*/)
						{
							if(s2->mslc) AICA->udata.data[0x10] &= 0x7FFF; // reset LP at KEY_ON
							AICA_StartSlot(AICA, s2);

							#if 0
							printf("StartSlot[%02X]:   SSCTL %01X SA %06X LSA %04X LEA %04X PCMS %01X LPCTL %01X\n",sl,SSCTL(s2),SA(s2),LSA(s2),LEA(s2),PCMS(s2),LPCTL(s2));
							printf("                 AR %02X D1R %02X D2R %02X RR %02X DL %02X KRS %01X LPSLNK %01X\n",AR(s2),D1R(s2),D2R(s2),RR(s2),DL(s2),KRS(s2),LPSLNK(s2)>>14);
							printf("                 TL %02X OCT %01X FNS %03X\n",TL(s2),OCT(s2),FNS(s2));
							printf("                 LFORE %01X LFOF %02X ALFOWS %01X ALFOS %01X PLFOWS %01X PLFOS %01X\n",LFORE(s2),LFOF(s2),ALFOWS(s2),ALFOS(s2),PLFOWS(s2),PLFOS(s2));
							printf("                 IMXL %01X ISEL %01X DISDL %01X DIPAN %02X\n",IMXL(s2),ISEL(s2),DISDL(s2),DIPAN(s2));
							printf("\n");
							fflush(stdout);
							#endif
						}
						if(!KEYONB(s2) /*&& s2->active*/)
						{
							AICA_StopSlot(s2,1);
						}
					}
				}
				slot->udata.data[0]&=~0x8000;
			}
			break;
		case 0x18:
		case 0x19:
			slot->step=AICA_Step(slot);
			break;
		case 0x14:
		case 0x15:
			slot->EG.RR=Get_RR(AICA,0,RR(slot));
			slot->EG.DL=0x1f-DL(slot);
			break;
		case 0x1c:
		case 0x1d:
			Compute_LFO(slot);
			break;
		case 0x24:
//			printf("[%02d]: %x to DISDL/DIPAN (PC=%x)\n", s, slot->udata.data[0x24/2], arm7_get_register(15));
			break;
	}
}

static void AICA_UpdateReg(struct _AICA *AICA, int reg)
{
	switch(reg&0xff)
	{
		case 0x4:
		case 0x5:
			{
				unsigned int v=RBL(AICA);
				AICA->DSP.RBP=RBP(AICA);
				if(v==0)
					AICA->DSP.RBL=8*1024;
				else if(v==1)
					AICA->DSP.RBL=16*1024;
				else if(v==2)
					AICA->DSP.RBL=32*1024;
				else if(v==3)
					AICA->DSP.RBL=64*1024;
			}
			break;
		case 0x8:
		case 0x9:
			AICA_MidiIn(AICA, 0, AICA->udata.data[0x8/2]&0xff, 0);
			break;
		case 0x12:
		case 0x13:
		case 0x14:
		case 0x15:
		case 0x16:
		case 0x17:
			break;
		case 0x90:
		case 0x91:
			if(AICA->Master)
			{
				AICA->TimPris[0]=1<<((AICA->udata.data[0x90/2]>>8)&0x7);
				AICA->TimCnt[0]=(AICA->udata.data[0x90/2]&0xff)<<8;
			}
			break;
		case 0x94:
		case 0x95:
			if(AICA->Master)
			{
				AICA->TimPris[1]=1<<((AICA->udata.data[0x94/2]>>8)&0x7);
				AICA->TimCnt[1]=(AICA->udata.data[0x94/2]&0xff)<<8;
			}
			break;
		case 0x98:
		case 0x99:
			if(AICA->Master)
			{
				AICA->TimPris[2]=1<<((AICA->udata.data[0x98/2]>>8)&0x7);
				AICA->TimCnt[2]=(AICA->udata.data[0x98/2]&0xff)<<8;
			}
			break;
		case 0xa4:	//SCIRE
		case 0xa5:

			if(AICA->Master)		   
			{
				AICA->udata.data[0xa0/2] &= ~AICA->udata.data[0xa4/2];
				ResetInterrupts(AICA);

				// behavior from real hardware (SCSP, assumed to carry over): if you SCIRE a timer that's expired,
				// it'll immediately pop up again
				if (AICA->TimCnt[0] >= 0xff00)
				{
					AICA->udata.data[0xa0/2] |= 0x40;
				}
				if (AICA->TimCnt[1] >= 0xff00)
				{
					AICA->udata.data[0xa0/2] |= 0x80;
				}
				if (AICA->TimCnt[2] >= 0xff00)
				{
					AICA->udata.data[0xa0/2] |= 0x100;
				}
			}
			break;
		case 0xa8:
		case 0xa9:
		case 0xac:
		case 0xad:
		case 0xb0:
		case 0xb1:
			if(AICA->Master)
			{
				AICA->IrqTimA=DecodeSCI(AICA,SCITMA);
				AICA->IrqTimBC=DecodeSCI(AICA,SCITMB);
				AICA->IrqMidi=DecodeSCI(AICA,SCIMID);
			}
			break;
	}
}

static void AICA_UpdateSlotRegR(struct _AICA *AICA, int slot,int reg)
{

}

static void AICA_UpdateRegR(struct _AICA *AICA, int reg)
{
	switch(reg&0xff)
	{
		case 8:
		case 9:
			{
				unsigned short v=AICA->udata.data[0x8/2];
				v&=0xff00;
				v|=AICA->MidiStack[AICA->MidiR];
				AICA->IntARMCB(AICA->cpu, 0);	// cancel the IRQ
				if(AICA->MidiR!=AICA->MidiW)
				{
					++AICA->MidiR;
					AICA->MidiR&=15;
				}
				AICA->udata.data[0x8/2]=v;
			}
			break;

		case 0x10:	// LP check
		case 0x11:
			{
				//int MSLC = (AICA->udata.data[0xc/2]>>8) & 0x3f;	// which slot are we monitoring?
			}
			break;
			
		case 0x14:	// CA (slot address)
		case 0x15:
			{
				int MSLC = (AICA->udata.data[0xc/2]>>8) & 0x3f;	// which slot are we monitoring?
				unsigned int CA = AICA->Slots[MSLC].cur_addr>>(SHIFT+12);

				AICA->udata.data[0x14/2] = CA;
			}
			break;
	}
}

static void AICA_w16(struct _AICA *AICA,unsigned int addr,unsigned short val)
{
	addr&=0xffff;
	if(addr<0x2000)
	{
		int slot=addr/0x80;
		addr&=0x7f;
//		printf("%x to slot %d offset %x\n", val, slot, addr);
		*((unsigned short *) (AICA->Slots[slot].udata.datab+(addr))) = val;
		AICA_UpdateSlotReg(AICA,slot,addr&0x7f);
	}
	else if (addr < 0x2800)
	{
		if (addr <= 0x2044)
		{
//			printf("%x to EFSxx slot %d (addr %x)\n", val, (addr-0x2000)/4, addr&0x7f);
			AICA->EFSPAN[addr&0x7f] = val;
		}
	}
	else if(addr<0x3000)
	{
		if (addr < 0x28be)
		{
//			printf("%x to AICA global @ %x\n", val, addr & 0xff);
			*((unsigned short *) (AICA->udata.datab+((addr&0xff)))) = val;
			AICA_UpdateReg(AICA, addr&0xff);
		}
		else if (addr == 0x2d00)
		{
			AICA->IRQL = val;
		}
		else if (addr == 0x2d04)
		{
			AICA->IRQR = val;

			if (val)
			{
				AICA->IntARMCB(AICA->cpu, 0);
			}
		}
	}
	else
	{
		//DSP
		if(addr<0x3200)	//COEF
			*((unsigned short *) (AICA->DSP.COEF+(addr-0x3000)/2))=val;
		else if(addr<0x3400)
			*((unsigned short *) (AICA->DSP.MADRS+(addr-0x3200)/2))=val;
		else if(addr<0x3c00)
		{
			*((unsigned short *) (AICA->DSP.MPRO+(addr-0x3400)/2))=val;

			if (addr == 0x3bfe)
			{
				AICADSP_Start(&AICA->DSP);
			}
		}
	}
}

static unsigned short AICA_r16(struct _AICA *AICA, unsigned int addr)
{
	unsigned short v=0;
	addr&=0xffff;
	if(addr<0x2000)
	{
		int slot=addr/0x80;
		addr&=0x7f;
		AICA_UpdateSlotRegR(AICA, slot,addr&0x7f);
		v=*((unsigned short *) (AICA->Slots[slot].udata.datab+(addr)));
	}
	else if(addr<0x3000)
	{
		if (addr <= 0x2044)
		{
			v = AICA->EFSPAN[addr&0x7f];
		}
		else if (addr < 0x28be)
		{
			AICA_UpdateRegR(AICA, addr&0xff);
			v= *((unsigned short *) (AICA->udata.datab+((addr&0xff))));
			if((addr&0xfe)==0x10) AICA->udata.data[0x10/2] &= 0x7FFF;	// reset LP on read
		}
		else if (addr == 0x2d00)
		{
			return AICA->IRQL;
		}
		else if (addr == 0x2d04)
		{
			return AICA->IRQR;
		}
	}
//	else if (addr<0x700)
//		v=AICA->RINGBUF[(addr-0x600)/2];
	return v;
}


#define REVSIGN(v) ((~v)+1)

void AICA_TimersAddTicks(struct _AICA *AICA, int ticks)
{
	if(AICA->TimCnt[0]<=0xff00)
	{
 		AICA->TimCnt[0] += ticks << (8-((AICA->udata.data[0x90/2]>>8)&0x7));
		if (AICA->TimCnt[0] >= 0xFF00)
		{
			AICA->TimCnt[0] = 0xFFFF;
			AICA->udata.data[0xa0/2]|=0x40;
		}
		AICA->udata.data[0x90/2]&=0xff00;
		AICA->udata.data[0x90/2]|=AICA->TimCnt[0]>>8;
	}

	if(AICA->TimCnt[1]<=0xff00)
	{
		AICA->TimCnt[1] += ticks << (8-((AICA->udata.data[0x94/2]>>8)&0x7));
		if (AICA->TimCnt[1] >= 0xFF00)
		{
			AICA->TimCnt[1] = 0xFFFF;
			AICA->udata.data[0xa0/2]|=0x80;
		}
		AICA->udata.data[0x94/2]&=0xff00;
		AICA->udata.data[0x94/2]|=AICA->TimCnt[1]>>8;
	}

	if(AICA->TimCnt[2]<=0xff00)
	{
		AICA->TimCnt[2] += ticks << (8-((AICA->udata.data[0x98/2]>>8)&0x7));
		if (AICA->TimCnt[2] >= 0xFF00)
		{
			AICA->TimCnt[2] = 0xFFFF;
			AICA->udata.data[0xa0/2]|=0x100;
		}
		AICA->udata.data[0x98/2]&=0xff00;
		AICA->udata.data[0x98/2]|=AICA->TimCnt[2]>>8;
	}
}

static INLINE INT32 AICA_UpdateSlot(struct _AICA *AICA, struct _SLOT *slot)
{
	INT32 sample, fpart;
	int cur_sample;       //current sample
	int nxt_sample;       //next sample
	int step=slot->step;
	UINT32 addr1,addr2;                                   // current and next sample addresses

	if(SSCTL(slot)!=0)	//no FM or noise yet
		return 0;

	if(PLFOS(slot)!=0)
	{
		step=step*AICAPLFO_Step(&(slot->PLFO));
		step>>=SHIFT;
	}

	if(PCMS(slot) == 0) 
	{
		addr1=(slot->cur_addr>>(SHIFT-1))&0x7ffffe;
		addr2=(slot->nxt_addr>>(SHIFT-1))&0x7ffffe;
	}
	else
	{
		addr1=slot->cur_addr>>SHIFT;
		addr2=slot->nxt_addr>>SHIFT;
	}

	if(PCMS(slot) == 1)	// 8-bit signed
	{
		INT8 *p1=(signed char *) (AICA->AICARAM+(((SA(slot)+addr1))&0x7fffff));
		INT8 *p2=(signed char *) (AICA->AICARAM+(((SA(slot)+addr2))&0x7fffff));
		cur_sample = p1[0] << 8;
		nxt_sample = p2[0] << 8;
	}
	else if (PCMS(slot) == 0)	//16 bit signed
	{
		INT16 *p1=(signed short *) (AICA->AICARAM+((SA(slot)+addr1)&0x7fffff));
		INT16 *p2=(signed short *) (AICA->AICARAM+((SA(slot)+addr2)&0x7fffff));
		cur_sample = LE16(p1[0]);
		nxt_sample = LE16(p2[0]);
	}
	else	// 4-bit ADPCM
	{
		UINT8 *base= slot->adbase;
		UINT32 steps_to_go = addr2, curstep = slot->curstep;

		if (base)
		{
			cur_sample = slot->cur_sample; // may already contains current decoded sample 

			// seek to the interpolation sample
			while (curstep < steps_to_go)
			{
				int shift1, delta1;
				shift1 = 4*((curstep&1));
				delta1 = (*base>>shift1)&0xf;
				DecodeADPCM(&(slot->cur_sample),delta1,&(slot->cur_quant));
				curstep++;
				if (!(curstep & 1))
				{
					base++;
				}
				if (curstep == addr1)
					cur_sample = slot->cur_sample;
			}
			nxt_sample = slot->cur_sample;

			slot->adbase = base;
			slot->curstep = curstep;
		}
		else
		{
			cur_sample = nxt_sample = 0;
		}
	}
	fpart = slot->cur_addr & ((1<<SHIFT)-1);
	sample=cur_sample*((1<<SHIFT)-fpart)+nxt_sample*fpart;
	sample>>=SHIFT;	

	slot->prv_addr=slot->cur_addr;
	slot->cur_addr+=step;
	slot->nxt_addr=slot->cur_addr+(1<<SHIFT);

	addr1=slot->cur_addr>>SHIFT;
	addr2=slot->nxt_addr>>SHIFT;

	if(addr1>=LSA(slot))
	{
		if(LPSLNK(slot) && slot->EG.state==ATTACK)
			slot->EG.state = DECAY1;
	}

	switch(LPCTL(slot))
	{
	case 0:	//no loop
		if(addr2>=LSA(slot) && addr2>=LEA(slot)) // if next sample exceed then current must exceed too
		{
		//slot->active=0;
		if(slot->mslc) AICA->udata.data[8] |= 0x8000;
		AICA_StopSlot(slot,0);
		}
		break;
	case 1: //normal loop
		if(addr2>=LEA(slot))
		{
			INT32 rem_addr;
			if(slot->mslc) AICA->udata.data[8] |= 0x8000;
			rem_addr = slot->nxt_addr - (LEA(slot)<<SHIFT);
			slot->nxt_addr = (LSA(slot)<<SHIFT) + rem_addr;
			if(addr1>=LEA(slot))
			{
				rem_addr = slot->cur_addr - (LEA(slot)<<SHIFT);
				slot->cur_addr = (LSA(slot)<<SHIFT) + rem_addr;
			}
				
			if(PCMS(slot)>=2)
			{
				// restore the state @ LSA - the sampler will naturally walk to (LSA + remainder)
				slot->adbase = &AICA->AICARAM[SA(slot)+(LSA(slot)/2)];
				slot->curstep = LSA(slot);
				if (PCMS(slot) == 2)
				{
					slot->cur_sample = slot->cur_lpsample;
					slot->cur_quant = slot->cur_lpquant;
				}
//printf("Looping: slot_addr %x LSA %x LEA %x step %x base %x\n", slot->cur_addr>>SHIFT, LSA(slot), LEA(slot), slot->curstep, slot->adbase);
			}
		}
		break;
	}

	if(ALFOS(slot)!=0)
	{
		sample=sample*AICAALFO_Step(&(slot->ALFO));
		sample>>=SHIFT;
	}

	if(slot->EG.state==ATTACK)
		sample=(sample*EG_Update(slot))>>SHIFT;
	else
		sample=(sample*EG_TABLE[EG_Update(slot)>>(SHIFT-10)])>>SHIFT;
		
	if(slot->mslc) 
	{
		AICA->udata.data[0x14/2] = addr1;
		if (!(AFSEL(AICA)))
		{
			UINT16 res;

			AICA->udata.data[0x10/2] |= slot->EG.state<<13;

			res = 0x3FF - (slot->EG.volume>>EG_SHIFT);

			res *= 959;
			res /= 1024;

			if (res > 959) res = 959;

			AICA->udata.data[0x10/2] = res;

			//AICA->udata.data[0x10/2] |= 0x3FF - (slot->EG.volume>>EG_SHIFT);
		}
	}

	return sample;
}

static void AICA_DoMasterSamples(struct _AICA *AICA, int nsamples)
{
	INT16 *bufr,*bufl;
	int sl, s, i;

	bufr=AICA->bufferr;
	bufl=AICA->bufferl;

	for(s=0;s<nsamples;++s)
	{
 		INT32 smpl, smpr;

		smpl = smpr = 0;

		// mix slots' direct output
		for(sl=0;sl<64;++sl)
		{
			struct _SLOT *slot=AICA->Slots+sl;
			slot->mslc = (MSLC(AICA)==sl);
			AICA->RBUFDST=AICA->RINGBUF+AICA->BUFPTR;
			if(AICA->Slots[sl].active)
			{
				unsigned int Enc;
				signed int sample;

				sample=AICA_UpdateSlot(AICA, slot);

				Enc=((TL(slot))<<0x0)|((IMXL(slot))<<0xd);
				AICADSP_SetSample(&AICA->DSP,(sample*AICA->LPANTABLE[Enc])>>(SHIFT-2),ISEL(slot),IMXL(slot));
				Enc=((TL(slot))<<0x0)|((DIPAN(slot))<<0x8)|((DISDL(slot))<<0xd);
				{
					smpl+=(sample*AICA->LPANTABLE[Enc])>>SHIFT;
					smpr+=(sample*AICA->RPANTABLE[Enc])>>SHIFT;
				}
			}
			
			AICA->BUFPTR&=63;
		}

		// process the DSP
		AICADSP_Step(&AICA->DSP);

		// mix DSP output
		for(i=0;i<16;++i)
		{
			if(EFSDL(i))
			{
				unsigned int Enc=((EFPAN(i))<<0x8)|((EFSDL(i))<<0xd);
				smpl+=(AICA->DSP.EFREG[i]*AICA->LPANTABLE[Enc])>>SHIFT;
				smpr+=(AICA->DSP.EFREG[i]*AICA->RPANTABLE[Enc])>>SHIFT;
			}
		}

		*bufl++ = ICLIP16(smpl>>3);
		*bufr++ = ICLIP16(smpr>>3);

		AICA_TimersAddTicks(AICA, 1);
		CheckPendingIRQ(AICA);
	}
}

int AICA_IRQCB(void *param)
{
	CheckPendingIRQ(param);
	return -1;
}

void AICA_Update(struct _AICA *AICA, void *param, INT16 **inputs, INT16 **buf, int samples)
{
	AICA->bufferl = buf[0];
	AICA->bufferr = buf[1];
	AICA->length = samples;
	AICA_DoMasterSamples(AICA, samples);
}

void *aica_start(const void *config)
{
	const struct AICAinterface *intf;

	struct _AICA *AICA;

	AICA = malloc(sizeof(*AICA));
	memset(AICA, 0, sizeof(*AICA));

	intf = config;

	// init the emulation
	AICA_Init(AICA, intf);

	// set up the IRQ callbacks
	{
		AICA->IntARMCB = intf->irq_callback[0];

//		AICA->stream = stream_create(0, 2, 44100, AICA, AICA_Update);
	}

	return AICA;
}

void aica_stop(struct _AICA *AICA)
{
    if (!AICA) {
        return;
    }
    if (AICA->buffertmpl) {
        free (AICA->buffertmpl);
    }
    if (AICA->buffertmpr) {
        free (AICA->buffertmpr);
    }
	free(AICA);
}

void AICA_set_ram_base(struct _AICA *AICA, int which, void *base)
{
	if (AICA)
	{
		AICA->AICARAM = base;
		AICA->DSP.AICARAM = base;
	}
}

READ16_HANDLER( AICA_0_r )
{
	UINT16 res = AICA_r16(AICA, offset*2);

//	printf("Read AICA @ %x => %x (PC=%x, R5=%x)\n", offset*2, res, arm7_get_register(15), arm7_get_register(5));

	return res;
}

extern UINT32* stv_scu;

WRITE16_HANDLER( AICA_0_w )
{
	UINT16 tmp;

	tmp = AICA_r16(AICA, offset*2);
	COMBINE_DATA(&tmp);
	AICA_w16(AICA,offset*2, tmp);
}

WRITE16_HANDLER( AICA_MidiIn )
{
	AICA->MidiStack[AICA->MidiW++]=data;
	AICA->MidiW &= 15;
}

READ16_HANDLER( AICA_MidiOutR )
{
	unsigned char val;

	val=AICA->MidiStack[AICA->MidiR++];
	AICA->MidiR&=7;
	return val;
}

