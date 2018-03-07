/*
 * Sega System 32 Multi/Model 1/Model 2 custom PCM chip (315-5560) emulation.
 *
 * by Miguel Angel Horna (ElSemi) for Model 2 Emulator and MAME.
 * Information by R.Belmont and the YMF278B (OPL4) manual.
 *
 * voice registers:
 * 0: Pan
 * 1: Index of sample
 * 2: LSB of pitch (low 2 bits seem unused so)
 * 3: MSB of pitch (ooooppppppppppxx) (o=octave (4 bit signed), p=pitch (10 bits), x=unused?
 * 4: voice control: top bit = 1 for key on, 0 for key off
 * 5: bit 0: 0: interpolate volume changes, 1: direct set volume,
      bits 1-7 = volume attenuate (0=max, 7f=min)
 * 6: LFO frequency + Phase LFO depth
 * 7: Amplitude LFO size
 *
 * The first sample ROM contains a variable length table with 12
 * bytes per instrument/sample. This is very similar to the YMF278B.
 *
 * The first 3 bytes are the offset into the file (big endian).
 * The next 2 are the loop start offset into the file (big endian)
 * The next 2 are the 2's complement of the total sample size (big endian)
 * The next byte is LFO freq + depth (copied to reg 6 ?)
 * The next 3 are envelope params (Attack, Decay1 and 2, sustain level, release, Key Rate Scaling)
 * The next byte is Amplitude LFO size (copied to reg 7 ?)
 *
 * TODO
 * - The YM278B manual states that the chip supports 512 instruments. The MultiPCM probably supports them
 * too but the high bit position is unknown (probably reg 2 low bit). Any game use more than 256?
 *
 */

//#include "emu.h"
//#include "streams.h"
#include "mamedef.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "multipcm.h"

#ifndef NULL
#define NULL	((void *)0)
#endif

//????
#define MULTIPCM_CLOCKDIV   	(180.0)

struct _Sample
{
	unsigned int Start;
	unsigned int Loop;
	unsigned int End;
	unsigned char AR,DR1,DR2,DL,RR;
	unsigned char KRS;
	unsigned char LFOVIB;
	unsigned char AM;
};

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
};

struct _LFO
{
	unsigned short phase;
	UINT32 phase_step;
	int *table;
	int *scale;
};


struct _SLOT
{
	unsigned char Num;
	unsigned char Regs[8];
	int Playing;
	struct _Sample *Sample;
	unsigned int Base;
	unsigned int offset;
	unsigned int step;
	unsigned int Pan,TL;
	unsigned int DstTL;
	int TLStep;
	signed int Prev;
	struct _EG EG;
	struct _LFO PLFO;	//Phase lfo
	struct _LFO ALFO;	//AM lfo
	
	UINT8 Muted;
};

typedef struct _MultiPCM MultiPCM;
struct _MultiPCM
{
	//sound_stream * stream;
	struct _Sample Samples[0x200];		//Max 512 samples
	struct _SLOT Slots[28];
	unsigned int CurSlot;
	unsigned int Address;
	unsigned int BankR,BankL;
	float Rate;
	UINT32 ROMMask;
	UINT32 ROMSize;
	INT8 *ROM;
	//I include these in the chip because they depend on the chip clock
	unsigned int ARStep[0x40],DRStep[0x40];	//Envelope step table
	unsigned int FNS_Table[0x400];		//Frequency step table
};


static UINT8 IsInit = 0x00;
static signed int LPANTABLE[0x800],RPANTABLE[0x800];

#define FIX(v)	((UINT32) ((float) (1<<SHIFT)*(v)))

static const int val2chan[] =
{
	0, 1, 2, 3, 4, 5, 6 , -1,
	7, 8, 9, 10,11,12,13, -1,
	14,15,16,17,18,19,20, -1,
	21,22,23,24,25,26,27, -1,
};


#define SHIFT		12


#define MULTIPCM_RATE	44100.0


/*INLINE MultiPCM *get_safe_token(running_device *device)
{
	assert(device != NULL);
	assert(device->type() == MULTIPCM);
	return (MultiPCM *)downcast<legacy_device_base *>(device)->token();
}*/


/*******************************
        ENVELOPE SECTION
*******************************/

//Times are based on a 44100Hz timebase. It's adjusted to the actual sampling rate on startup

static const double BaseTimes[64]={0,0,0,0,6222.95,4978.37,4148.66,3556.01,3111.47,2489.21,2074.33,1778.00,1555.74,1244.63,1037.19,889.02,
777.87,622.31,518.59,444.54,388.93,311.16,259.32,222.27,194.47,155.60,129.66,111.16,97.23,77.82,64.85,55.60,
48.62,38.91,32.43,27.80,24.31,19.46,16.24,13.92,12.15,9.75,8.12,6.98,6.08,4.90,4.08,3.49,
3.04,2.49,2.13,1.90,1.72,1.41,1.18,1.04,0.91,0.73,0.59,0.50,0.45,0.45,0.45,0.45};
#define AR2DR	14.32833
static signed int lin2expvol[0x400];
static int TLSteps[2];

#define EG_SHIFT	16

static int EG_Update(struct _SLOT *slot)
{
	switch(slot->EG.state)
	{
		case ATTACK:
			slot->EG.volume+=slot->EG.AR;
			if(slot->EG.volume>=(0x3ff<<EG_SHIFT))
			{
				slot->EG.state=DECAY1;
				if(slot->EG.D1R>=(0x400<<EG_SHIFT)) //Skip DECAY1, go directly to DECAY2
					slot->EG.state=DECAY2;
				slot->EG.volume=0x3ff<<EG_SHIFT;
			}
			break;
		case DECAY1:
			slot->EG.volume-=slot->EG.D1R;
			if(slot->EG.volume<=0)
				slot->EG.volume=0;
			if(slot->EG.volume>>EG_SHIFT<=(slot->EG.DL<<(10-4)))
				slot->EG.state=DECAY2;
			break;
		case DECAY2:
			slot->EG.volume-=slot->EG.D2R;
			if(slot->EG.volume<=0)
				slot->EG.volume=0;
			break;
		case RELEASE:
			slot->EG.volume-=slot->EG.RR;
			if(slot->EG.volume<=0)
			{
				slot->EG.volume=0;
				slot->Playing=0;
			}
			break;
		default:
			return 1<<SHIFT;
	}
	return lin2expvol[slot->EG.volume>>EG_SHIFT];
}

static unsigned int Get_RATE(unsigned int *Steps,unsigned int rate,unsigned int val)
{
	int r=4*val+rate;
	if(val==0)
		return Steps[0];
	if(val==0xf)
		return Steps[0x3f];
	if(r>0x3f)
		r=0x3f;
	return Steps[r];
}

static void EG_Calc(MultiPCM *ptChip,struct _SLOT *slot)
{
	int octave=((slot->Regs[3]>>4)-1)&0xf;
	int rate;
	if(octave&8) octave=octave-16;
	if(slot->Sample->KRS!=0xf)
		rate=(octave+slot->Sample->KRS)*2+((slot->Regs[3]>>3)&1);
	else
		rate=0;

	slot->EG.AR=Get_RATE(ptChip->ARStep,rate,slot->Sample->AR);
	slot->EG.D1R=Get_RATE(ptChip->DRStep,rate,slot->Sample->DR1);
	slot->EG.D2R=Get_RATE(ptChip->DRStep,rate,slot->Sample->DR2);
	slot->EG.RR=Get_RATE(ptChip->DRStep,rate,slot->Sample->RR);
	slot->EG.DL=0xf-slot->Sample->DL;

}

/*****************************
        LFO  SECTION
*****************************/

#define LFO_SHIFT	8


#define LFIX(v)	((unsigned int) ((float) (1<<LFO_SHIFT)*(v)))

//Convert DB to multiply amplitude
#define DB(v)	LFIX(pow(10.0,v/20.0))

//Convert cents to step increment
#define CENTS(v) LFIX(pow(2.0,v/1200.0))

static int PLFO_TRI[256];
static int ALFO_TRI[256];

static const float LFOFreq[8]={0.168f,2.019f,3.196f,4.206f,5.215f,5.888f,6.224f,7.066f};	//Hz;
static const float PSCALE[8]={0.0f,3.378f,5.065f,6.750f,10.114f,20.170f,40.180f,79.307f};	//cents
static const float ASCALE[8]={0.0f,0.4f,0.8f,1.5f,3.0f,6.0f,12.0f,24.0f};					//DB
static int PSCALES[8][256];
static int ASCALES[8][256];

static void LFO_Init(void)
{
	int i,s;
	for(i=0;i<256;++i)
	{
		int a;	//amplitude
		int p;	//phase

		//Tri
		if(i<128)
			a=255-(i*2);
		else
			a=(i*2)-256;
		if(i<64)
			p=i*2;
		else if(i<128)
			p=255-i*2;
		else if(i<192)
			p=256-i*2;
		else
			p=i*2-511;
		ALFO_TRI[i]=a;
		PLFO_TRI[i]=p;
	}

	for(s=0;s<8;++s)
	{
		float limit=PSCALE[s];
		for(i=-128;i<128;++i)
		{
			PSCALES[s][i+128]=CENTS(((limit*(float) i)/128.0));
		}
		limit=-ASCALE[s];
		for(i=0;i<256;++i)
		{
			ASCALES[s][i]=DB(((limit*(float) i)/256.0));
		}
	}
}

INLINE signed int PLFO_Step(struct _LFO *LFO)
{
	int p;
	LFO->phase+=LFO->phase_step;
	p=LFO->table[(LFO->phase>>LFO_SHIFT)&0xff];
	p=LFO->scale[p+128];
	return p<<(SHIFT-LFO_SHIFT);
}

INLINE signed int ALFO_Step(struct _LFO *LFO)
{
	int p;
	LFO->phase+=LFO->phase_step;
	p=LFO->table[(LFO->phase>>LFO_SHIFT)&0xff];
	p=LFO->scale[p];
	return p<<(SHIFT-LFO_SHIFT);
}

static void LFO_ComputeStep(MultiPCM *ptChip,struct _LFO *LFO,UINT32 LFOF,UINT32 LFOS,int ALFO)
{
	float step=(float) LFOFreq[LFOF]*256.0/(float) ptChip->Rate;
	LFO->phase_step=(unsigned int) ((float) (1<<LFO_SHIFT)*step);
	if(ALFO)
	{
		LFO->table=ALFO_TRI;
		LFO->scale=ASCALES[LFOS];
	}
	else
	{
		LFO->table=PLFO_TRI;
		LFO->scale=PSCALES[LFOS];
	}
}



static void WriteSlot(MultiPCM *ptChip,struct _SLOT *slot,int reg,unsigned char data)
{
	slot->Regs[reg]=data;

	switch(reg)
	{
		case 0:	//PANPOT
			slot->Pan=(data>>4)&0xf;
			break;
		case 1:	//Sample
			//according to YMF278 sample write causes some base params written to the regs (envelope+lfos)
			//the game should never change the sample while playing.
			{
				struct _Sample *Sample=ptChip->Samples+slot->Regs[1];
				WriteSlot(ptChip,slot,6,Sample->LFOVIB);
				WriteSlot(ptChip,slot,7,Sample->AM);
			}
			break;
		case 2:	//Pitch
		case 3:
			{
				unsigned int oct=((slot->Regs[3]>>4)-1)&0xf;
				unsigned int pitch=((slot->Regs[3]&0xf)<<6)|(slot->Regs[2]>>2);
				pitch=ptChip->FNS_Table[pitch];
				if(oct&0x8)
					pitch>>=(16-oct);
				else
					pitch<<=oct;
				slot->step=pitch/ptChip->Rate;
			}
			break;
		case 4:		//KeyOn/Off (and more?)
			{
				if(data&0x80)		//KeyOn
				{
					slot->Sample=ptChip->Samples+slot->Regs[1];
					slot->Playing=1;
					slot->Base=slot->Sample->Start;
					slot->offset=0;
					slot->Prev=0;
					slot->TL=slot->DstTL<<SHIFT;

					EG_Calc(ptChip,slot);
					slot->EG.state=ATTACK;
					slot->EG.volume=0;

					if(slot->Base>=0x100000)
					{
						if(slot->Pan&8)
							slot->Base=(slot->Base&0xfffff)|(ptChip->BankL);
						else
							slot->Base=(slot->Base&0xfffff)|(ptChip->BankR);
					}

				}
				else
				{
					if(slot->Playing)
					{
						if(slot->Sample->RR!=0xf)
							slot->EG.state=RELEASE;
						else
							slot->Playing=0;
					}
				}
			}
			break;
		case 5:	//TL+Interpolation
			{
				slot->DstTL=(data>>1)&0x7f;
				if(!(data&1))	//Interpolate TL
				{
					if((slot->TL>>SHIFT)>slot->DstTL)
						slot->TLStep=TLSteps[0];		//decrease
					else
						slot->TLStep=TLSteps[1];		//increase
				}
				else
					slot->TL=slot->DstTL<<SHIFT;
			}
			break;
		case 6:	//LFO freq+PLFO
			{
				if(data)
				{
					LFO_ComputeStep(ptChip,&(slot->PLFO),(slot->Regs[6]>>3)&7,slot->Regs[6]&7,0);
					LFO_ComputeStep(ptChip,&(slot->ALFO),(slot->Regs[6]>>3)&7,slot->Regs[7]&7,1);
				}
			}
			break;
		case 7:	//ALFO
			{
				if(data)
				{
					LFO_ComputeStep(ptChip,&(slot->PLFO),(slot->Regs[6]>>3)&7,slot->Regs[6]&7,0);
					LFO_ComputeStep(ptChip,&(slot->ALFO),(slot->Regs[6]>>3)&7,slot->Regs[7]&7,1);
				}
			}
			break;
	}
}

//static STREAM_UPDATE( MultiPCM_update )
void MultiPCM_update(void *param, stream_sample_t **outputs, int samples)
{
	MultiPCM *ptChip = (MultiPCM *)param;
	stream_sample_t  *datap[2];
	int i,sl;

	datap[0] = outputs[0];
	datap[1] = outputs[1];

	memset(datap[0], 0, sizeof(*datap[0])*samples);
	memset(datap[1], 0, sizeof(*datap[1])*samples);


	for(i=0;i<samples;++i)
	{
		signed int smpl=0;
		signed int smpr=0;
		for(sl=0;sl<28;++sl)
		{
			struct _SLOT *slot=ptChip->Slots+sl;
			if(slot->Playing && ! slot->Muted)
			{
				unsigned int vol=(slot->TL>>SHIFT)|(slot->Pan<<7);
				unsigned int adr=slot->offset>>SHIFT;
				signed int sample;
				unsigned int step=slot->step;
				signed int csample=(signed short) (ptChip->ROM[(slot->Base+adr) & ptChip->ROMMask]<<8);
				signed int fpart=slot->offset&((1<<SHIFT)-1);
				sample=(csample*fpart+slot->Prev*((1<<SHIFT)-fpart))>>SHIFT;

				if(slot->Regs[6]&7)	//Vibrato enabled
				{
					step=step*PLFO_Step(&(slot->PLFO));
					step>>=SHIFT;
				}

				slot->offset+=step;
				if(slot->offset>=(slot->Sample->End<<SHIFT))
				{
					slot->offset=slot->Sample->Loop<<SHIFT;
				}
				if(adr^(slot->offset>>SHIFT))
				{
					slot->Prev=csample;
				}

				if((slot->TL>>SHIFT)!=slot->DstTL)
					slot->TL+=slot->TLStep;

				if(slot->Regs[7]&7)	//Tremolo enabled
				{
					sample=sample*ALFO_Step(&(slot->ALFO));
					sample>>=SHIFT;
				}

				sample=(sample*EG_Update(slot))>>10;

				smpl+=(LPANTABLE[vol]*sample)>>SHIFT;
				smpr+=(RPANTABLE[vol]*sample)>>SHIFT;
			}
		}
/*#define ICLIP16(x) (x<-32768)?-32768:((x>32767)?32767:x)
		datap[0][i]=ICLIP16(smpl);
		datap[1][i]=ICLIP16(smpr);*/
		datap[0][i] = smpl;
		datap[1][i] = smpr;
	}
}

//READ8_DEVICE_HANDLER( multipcm_r )
UINT8 multipcm_r(void *_info, offs_t offset)
{
//  MultiPCM *ptChip = get_safe_token(device);
//	MultiPCM *ptChip = &MultiPCMData[ChipID];
	return 0;
}

//static DEVICE_START( multipcm )
int device_start_multipcm(void **_info, int clock)
{
	//MultiPCM *ptChip = get_safe_token(device);
	MultiPCM *ptChip;
	int i;

	ptChip = (MultiPCM *) calloc(1, sizeof(MultiPCM));
	*_info = (void *) ptChip;
	
	//ptChip->ROM=*device->region();
	ptChip->ROMMask = 0x00;
	ptChip->ROMSize = 0x00;
	ptChip->ROM = NULL;
	//ptChip->Rate=(float) device->clock() / MULTIPCM_CLOCKDIV;
	ptChip->Rate=(float) clock / MULTIPCM_CLOCKDIV;

	//ptChip->stream = stream_create(device, 0, 2, ptChip->Rate, ptChip, MultiPCM_update);

	if (! IsInit)
	{
		//Volume+pan table
		for(i=0;i<0x800;++i)
		{
			float SegaDB=0;
			float TL;
			float LPAN,RPAN;

			unsigned char iTL=i&0x7f;
			unsigned char iPAN=(i>>7)&0xf;

			SegaDB=(float) iTL*(-24.0)/(float) 0x40;

			TL=pow(10.0,SegaDB/20.0);


			if(iPAN==0x8)
			{
				LPAN=RPAN=0.0;
			}
			else if(iPAN==0x0)
			{
				LPAN=RPAN=1.0;
			}
			else if(iPAN&0x8)
			{
				LPAN=1.0;

				iPAN=0x10-iPAN;

				SegaDB=(float) iPAN*(-12.0)/(float) 0x4;

				RPAN=pow(10.0,SegaDB/20.0);

				if((iPAN&0x7)==7)
					RPAN=0.0;
			}
			else
			{
				RPAN=1.0;

				SegaDB=(float) iPAN*(-12.0)/(float) 0x4;

				LPAN=pow(10.0,SegaDB/20.0);
				if((iPAN&0x7)==7)
					LPAN=0.0;
			}

			TL/=4.0;

			LPANTABLE[i]=FIX((LPAN*TL));
			RPANTABLE[i]=FIX((RPAN*TL));
		}
		IsInit = 0x01;
	}

	//Pitch steps
	for(i=0;i<0x400;++i)
	{
		float fcent=ptChip->Rate*(1024.0+(float) i)/1024.0;
		ptChip->FNS_Table[i]=(unsigned int ) ((float) (1<<SHIFT) *fcent);
	}

	//Envelope steps
	for(i=0;i<0x40;++i)
	{
		//Times are based on 44100 clock, adjust to real chip clock
		ptChip->ARStep[i]=(float) (0x400<<EG_SHIFT)/(BaseTimes[i]*44100.0/(1000.0));
		ptChip->DRStep[i]=(float) (0x400<<EG_SHIFT)/(BaseTimes[i]*AR2DR*44100.0/(1000.0));
	}
	ptChip->ARStep[0]=ptChip->ARStep[1]=ptChip->ARStep[2]=ptChip->ARStep[3]=0;
	ptChip->ARStep[0x3f]=0x400<<EG_SHIFT;
	ptChip->DRStep[0]=ptChip->DRStep[1]=ptChip->DRStep[2]=ptChip->DRStep[3]=0;

	//TL Interpolation steps
	//lower
	TLSteps[0]=-(float) (0x80<<SHIFT)/(78.2*44100.0/1000.0);
	//raise
	TLSteps[1]=(float) (0x80<<SHIFT)/(78.2*2*44100.0/1000.0);

	//build the linear->exponential ramps
	for(i=0;i<0x400;++i)
	{
		float db=-(96.0-(96.0*(float) i/(float) 0x400));
		lin2expvol[i]=pow(10.0,db/20.0)*(float) (1<<SHIFT);
	}


	/*for(i=0;i<512;++i)
	{
		UINT8 *ptSample=(UINT8 *) ptChip->ROM+i*12;
		ptChip->Samples[i].Start=(ptSample[0]<<16)|(ptSample[1]<<8)|(ptSample[2]<<0);
		ptChip->Samples[i].Loop=(ptSample[3]<<8)|(ptSample[4]<<0);
		ptChip->Samples[i].End=0xffff-((ptSample[5]<<8)|(ptSample[6]<<0));
		ptChip->Samples[i].LFOVIB=ptSample[7];
		ptChip->Samples[i].DR1=ptSample[8]&0xf;
		ptChip->Samples[i].AR=(ptSample[8]>>4)&0xf;
		ptChip->Samples[i].DR2=ptSample[9]&0xf;
		ptChip->Samples[i].DL=(ptSample[9]>>4)&0xf;
		ptChip->Samples[i].RR=ptSample[10]&0xf;
		ptChip->Samples[i].KRS=(ptSample[10]>>4)&0xf;
		ptChip->Samples[i].AM=ptSample[11];
	}*/

	/*state_save_register_device_item(device, 0, ptChip->CurSlot);
	state_save_register_device_item(device, 0, ptChip->Address);
	state_save_register_device_item(device, 0, ptChip->BankL);
	state_save_register_device_item(device, 0, ptChip->BankR);*/

	// reset is done via DEVICE_RESET
	/*for(i=0;i<28;++i)
	{
		ptChip->Slots[i].Num=i;
		ptChip->Slots[i].Playing=0;

		state_save_register_device_item(device, i, ptChip->Slots[i].Num);
		state_save_register_device_item_array(device, i, ptChip->Slots[i].Regs);
		state_save_register_device_item(device, i, ptChip->Slots[i].Playing);
		state_save_register_device_item(device, i, ptChip->Slots[i].Base);
		state_save_register_device_item(device, i, ptChip->Slots[i].offset);
		state_save_register_device_item(device, i, ptChip->Slots[i].step);
		state_save_register_device_item(device, i, ptChip->Slots[i].Pan);
		state_save_register_device_item(device, i, ptChip->Slots[i].TL);
		state_save_register_device_item(device, i, ptChip->Slots[i].DstTL);
		state_save_register_device_item(device, i, ptChip->Slots[i].TLStep);
		state_save_register_device_item(device, i, ptChip->Slots[i].Prev);
		state_save_register_device_item(device, i, ptChip->Slots[i].EG.volume);
		state_save_register_device_item(device, i, ptChip->Slots[i].EG.state);
		state_save_register_device_item(device, i, ptChip->Slots[i].EG.step);
		state_save_register_device_item(device, i, ptChip->Slots[i].EG.AR);
		state_save_register_device_item(device, i, ptChip->Slots[i].EG.D1R);
		state_save_register_device_item(device, i, ptChip->Slots[i].EG.D2R);
		state_save_register_device_item(device, i, ptChip->Slots[i].EG.RR);
		state_save_register_device_item(device, i, ptChip->Slots[i].EG.DL);
		state_save_register_device_item(device, i, ptChip->Slots[i].PLFO.phase);
		state_save_register_device_item(device, i, ptChip->Slots[i].PLFO.phase_step);
		state_save_register_device_item(device, i, ptChip->Slots[i].ALFO.phase);
		state_save_register_device_item(device, i, ptChip->Slots[i].ALFO.phase_step);
	}*/

	LFO_Init();
	
	multipcm_set_bank(ptChip, 0x00, 0x00);

	return (int)(ptChip->Rate + 0.5);
}


void device_stop_multipcm(void *_info)
{
	MultiPCM *ptChip = (MultiPCM *)_info;
	
	free(ptChip->ROM);	ptChip->ROM = NULL;

	free(ptChip);	

	return;
}

void device_reset_multipcm(void *_info)
{
	MultiPCM *ptChip = (MultiPCM *)_info;
	int i;
	
	for(i=0;i<28;++i)
	{
		ptChip->Slots[i].Num=i;
		ptChip->Slots[i].Playing=0;
	}
	
	return;
}


//WRITE8_DEVICE_HANDLER( multipcm_w )
void multipcm_w(void *_info, offs_t offset, UINT8 data)
{
	//MultiPCM *ptChip = get_safe_token(device);
	MultiPCM *ptChip = (MultiPCM *)_info;
	switch(offset)
	{
		case 0:		//Data write
			WriteSlot(ptChip,ptChip->Slots+ptChip->CurSlot,ptChip->Address,data);
			break;
		case 1:
			ptChip->CurSlot=val2chan[data&0x1f];
			break;
		case 2:
			ptChip->Address=(data>7)?7:data;
			break;
	}
	/*ptChip->CurSlot = val2chan[(offset >> 3) & 0x1F];
	ptChip->Address = offset & 0x07;
	WriteSlot(ptChip, ptChip->Slots + ptChip->CurSlot, ptChip->Address, data);*/
}

/* MAME/M1 access functions */

//void multipcm_set_bank(running_device *device, UINT32 leftoffs, UINT32 rightoffs)
void multipcm_set_bank(void *_info, UINT32 leftoffs, UINT32 rightoffs)
{
	//MultiPCM *ptChip = get_safe_token(device);
	MultiPCM *ptChip = (MultiPCM *)_info;
	ptChip->BankL = leftoffs;
	ptChip->BankR = rightoffs;
}

void multipcm_bank_write(void *_info, UINT8 offset, UINT16 data)
{
	MultiPCM *ptChip = (MultiPCM *)_info;
	
	if (offset & 0x01)
		ptChip->BankL = data << 16;
	if (offset & 0x02)
		ptChip->BankR = data << 16;
	
	return;
}

void multipcm_write_rom(void *_info, offs_t ROMSize, offs_t DataStart, offs_t DataLength,
						const UINT8* ROMData)
{
	MultiPCM *ptChip = (MultiPCM *)_info;
	UINT16 CurSmpl;
	struct _Sample* TempSmpl;
	UINT8* ptSample;
	
	if (ptChip->ROMSize != ROMSize)
	{
		ptChip->ROM = (INT8*)realloc(ptChip->ROM, ROMSize);
		ptChip->ROMSize = ROMSize;
		
		for (ptChip->ROMMask = 1; ptChip->ROMMask < ROMSize; ptChip->ROMMask <<= 1)
			;
		ptChip->ROMMask --;
		
		memset(ptChip->ROM, 0xFF, ROMSize);
	}
	if (DataStart > ROMSize)
		return;
	if (DataStart + DataLength > ROMSize)
		DataLength = ROMSize - DataStart;
	
	memcpy(ptChip->ROM + DataStart, ROMData, DataLength);
	
	if (DataStart < 0x200 * 12)
	{
		for (CurSmpl = 0; CurSmpl < 512; CurSmpl ++)
		{
			TempSmpl = &ptChip->Samples[CurSmpl];
			ptSample = (UINT8*)ptChip->ROM + CurSmpl * 12;
			
			TempSmpl->Start = (ptSample[0]<<16)|(ptSample[1]<<8)|(ptSample[2]<<0);
			TempSmpl->Loop = (ptSample[3]<<8)|(ptSample[4]<<0);
			TempSmpl->End = 0xffff-((ptSample[5]<<8)|(ptSample[6]<<0));
			TempSmpl->LFOVIB = ptSample[7];
			TempSmpl->DR1 = ptSample[8]&0xf;
			TempSmpl->AR = (ptSample[8]>>4)&0xf;
			TempSmpl->DR2 = ptSample[9]&0xf;
			TempSmpl->DL = (ptSample[9]>>4)&0xf;
			TempSmpl->RR = ptSample[10]&0xf;
			TempSmpl->KRS = (ptSample[10]>>4)&0xf;
			TempSmpl->AM = ptSample[11];
		}
	}
	
	return;
}


void multipcm_set_mute_mask(void *_info, UINT32 MuteMask)
{
	MultiPCM* ptChip = (MultiPCM *)_info;
	UINT8 CurChn;
	
	for (CurChn = 0; CurChn < 28; CurChn ++)
		ptChip->Slots[CurChn].Muted = (MuteMask >> CurChn) & 0x01;
	
	return;
}

#if 0	// for debugging only
UINT8 multipcm_get_channels(UINT8 ChipID, UINT32* ChannelMask)
{
	MultiPCM* ptChip = &MultiPCMData[ChipID];
	UINT8 CurChn;
	UINT8 UsedChns;
	UINT32 ChnMask;
	
	ChnMask = 0x00000000;
	UsedChns = 0x00;
	for (CurChn = 0; CurChn < 28; CurChn ++)
	{
		if (ptChip->Slots[CurChn].Playing)
		{
			ChnMask |= (1 << CurChn);
			UsedChns ++;
		}
	}
	if (ChannelMask != NULL)
		*ChannelMask = ChnMask;
	
	return UsedChns;
}
#endif



/**************************************************************************
 * Generic get_info
 **************************************************************************/

/*DEVICE_GET_INFO( multipcm )
{
	switch (state)
	{
		// --- the following bits of info are returned as 64-bit signed integers ---
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(MultiPCM);				break;

		// --- the following bits of info are returned as pointers to data or functions ---
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME( multipcm );		break;
		case DEVINFO_FCT_STOP:							// Nothing										break;
		case DEVINFO_FCT_RESET:							// Nothing										break;

		// --- the following bits of info are returned as NULL-terminated strings ---
		case DEVINFO_STR_NAME:							strcpy(info->s, "Sega/Yamaha 315-5560");		break;
		case DEVINFO_STR_FAMILY:					strcpy(info->s, "Sega custom");					break;
		case DEVINFO_STR_VERSION:					strcpy(info->s, "2.0");							break;
		case DEVINFO_STR_SOURCE_FILE:						strcpy(info->s, __FILE__);						break;
		case DEVINFO_STR_CREDITS:					strcpy(info->s, "Copyright Nicola Salmoria and the MAME Team"); break;
	}
}*/


//DEFINE_LEGACY_SOUND_DEVICE(MULTIPCM, multipcm);
