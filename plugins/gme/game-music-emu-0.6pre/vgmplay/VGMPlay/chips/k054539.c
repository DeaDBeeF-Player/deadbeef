/*********************************************************

    Konami 054539 (TOP) PCM Sound Chip

    A lot of information comes from Amuse.
    Big thanks to them.

*********************************************************/

//#include "emu.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "mamedef.h"
#ifdef _DEBUG
#include <stdio.h>
#endif
#include "k054539.h"

#ifndef NULL
#define NULL	((void *)0)
#endif

#define VERBOSE 0
#define LOG(x) do { if (VERBOSE) logerror x; } while (0)

/* Registers:
   00..ff: 20 bytes/channel, 8 channels
     00..02: pitch (lsb, mid, msb)
         03: volume (0=max, 0x40=-36dB)
         04: reverb volume (idem)
     05: pan (1-f right, 10 middle, 11-1f left)
     06..07: reverb delay (0=max, current computation non-trusted)
     08..0a: loop (lsb, mid, msb)
     0c..0e: start (lsb, mid, msb) (and current position ?)

   100.1ff: effects?
     13f: pan of the analog input (1-1f)

   200..20f: 2 bytes/channel, 8 channels
     00: type (b2-3), reverse (b5)
     01: loop (b0)

   214: Key on (b0-7 = channel 0-7)
   215: Key off          ""
   225: ?
   227: Timer frequency
   228: ?
   229: ?
   22a: ?
   22b: ?
   22c: Channel active? (b0-7 = channel 0-7)
   22d: Data read/write port
   22e: ROM/RAM select (00..7f == ROM banks, 80 = Reverb RAM)
   22f: Global control:
		.......x - Enable PCM
		......x. - Timer related?
		...x.... - Enable ROM/RAM readback from 0x22d
		..x..... - Timer output enable?
		x....... - Disable register RAM updates

	The chip has an optional 0x8000 byte reverb buffer.
	The reverb delay is actually an offset in this buffer.
*/

typedef struct _k054539_channel k054539_channel;
struct _k054539_channel {
	UINT32 pos;
	UINT32 pfrac;
	INT32 val;
	INT32 pval;
};

typedef struct _k054539_state k054539_state;
struct _k054539_state {
	//const k054539_interface *intf;
	//device_t *device;
	double voltab[256];
	double pantab[0xf];

	double k054539_gain[8];
	UINT8 k054539_posreg_latch[8][3];
	int k054539_flags;

	unsigned char regs[0x230];
	unsigned char *ram;
	int reverb_pos;

	INT32 cur_ptr;
	int cur_limit;
	unsigned char *cur_zone;
	unsigned char *rom;
	UINT32 rom_size;
	UINT32 rom_mask;
	//sound_stream * stream;

	k054539_channel channels[8];
	UINT8 Muted[8];
	
	int clock;
};

/*INLINE k054539_state *get_safe_token(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == K054539);
	return (k054539_state *)downcast<legacy_device_base *>(device)->token();
}*/

//*

//void k054539_init_flags(device_t *device, int flags)
void k054539_init_flags(void *_info, int flags)
{
	//k054539_state *info = get_safe_token(device);
	k054539_state *info = (k054539_state *)_info;
	info->k054539_flags = flags;
}

//void k054539_set_gain(device_t *device, int channel, double gain)
void k054539_set_gain(void *_info, int channel, double gain)
{
	//k054539_state *info = get_safe_token(device);
	k054539_state *info = (k054539_state *)_info;
	if (gain >= 0) info->k054539_gain[channel] = gain;
}
//*

static int k054539_regupdate(k054539_state *info)
{
	return !(info->regs[0x22f] & 0x80);
}

static void k054539_keyon(k054539_state *info, int channel)
{
	if(k054539_regupdate(info))
		info->regs[0x22c] |= 1 << channel;
}

static void k054539_keyoff(k054539_state *info, int channel)
{
	if(k054539_regupdate(info))
		info->regs[0x22c] &= ~(1 << channel);
}

//static STREAM_UPDATE( k054539_update )
void k054539_update(void *param, stream_sample_t **outputs, int samples)
{
	k054539_state *info = (k054539_state *)param;
#define VOL_CAP 1.80

	static const INT16 dpcm[16] = {
		0<<8, 1<<8, 4<<8, 9<<8, 16<<8, 25<<8, 36<<8, 49<<8,
		-64*(1<<8), -49*(1<<8), -36*(1<<8), -25*(1<<8), -16*(1<<8), -9*(1<<8), -4*(1<<8), -1*(1<<8)
	};

	INT16 *rbase = (INT16 *)info->ram;
	unsigned char *rom;
	UINT32 rom_mask;
	int i, ch;
	double lval, rval;
	unsigned char *base1, *base2;
	k054539_channel *chan;
	int delta, vol, bval, pan;
	double cur_gain, lvol, rvol, rbvol;
	int rdelta;
	UINT32 cur_pos;
	int fdelta, pdelta;
	int cur_pfrac, cur_val, cur_pval;

	memset(outputs[0], 0, samples*sizeof(*outputs[0]));
	memset(outputs[1], 0, samples*sizeof(*outputs[1]));

	if(!(info->regs[0x22f] & 1))
		return;

	rom = info->rom;
	rom_mask = info->rom_mask;

	for(i = 0; i != samples; i++) {
		if(!(info->k054539_flags & K054539_DISABLE_REVERB))
			lval = rval = rbase[info->reverb_pos];
		else
			lval = rval = 0;
		rbase[info->reverb_pos] = 0;

		for(ch=0; ch<8; ch++)
			if(info->regs[0x22c] & (1<<ch) && ! info->Muted[ch]) {
				base1 = info->regs + 0x20*ch;
				base2 = info->regs + 0x200 + 0x2*ch;
				chan = info->channels + ch;

				delta = base1[0x00] | (base1[0x01] << 8) | (base1[0x02] << 16);

				vol = base1[0x03];

				bval = vol + base1[0x04];
				if (bval > 255)
					bval = 255;

				pan = base1[0x05];
				// DJ Main: 81-87 right, 88 middle, 89-8f left
				if (pan >= 0x81 && pan <= 0x8f)
					pan -= 0x81;
				else if (pan >= 0x11 && pan <= 0x1f)
					pan -= 0x11;
				else
					pan = 0x18 - 0x11;

				cur_gain = info->k054539_gain[ch];

				lvol = info->voltab[vol] * info->pantab[pan] * cur_gain;
				if (lvol > VOL_CAP)
					lvol = VOL_CAP;

				rvol = info->voltab[vol] * info->pantab[0xe - pan] * cur_gain;
				if (rvol > VOL_CAP)
					rvol = VOL_CAP;

				rbvol= info->voltab[bval] * cur_gain / 2;
				if (rbvol > VOL_CAP)
					rbvol = VOL_CAP;

				rdelta = (base1[6] | (base1[7] << 8)) >> 3;
				rdelta = (rdelta + info->reverb_pos) & 0x3fff;

				cur_pos = (base1[0x0c] | (base1[0x0d] << 8) | (base1[0x0e] << 16)) & rom_mask;

				if(base2[0] & 0x20) {
					delta = -delta;
					fdelta = +0x10000;
					pdelta = -1;
				} else {
					fdelta = -0x10000;
					pdelta = +1;
				}

				if(cur_pos != chan->pos) {
					chan->pos = cur_pos;
					cur_pfrac = 0;
					cur_val = 0;
					cur_pval = 0;
				} else {
					cur_pfrac = chan->pfrac;
					cur_val = chan->val;
					cur_pval = chan->pval;
				}

				switch(base2[0] & 0xc) {
				case 0x0: { // 8bit pcm
					cur_pfrac += delta;
					while(cur_pfrac & ~0xffff) {
						cur_pfrac += fdelta;
						cur_pos += pdelta;

						cur_pval = cur_val;
						cur_val = (INT16)(rom[cur_pos] << 8);
						if(cur_val == (INT16)0x8000 && (base2[1] & 1)) {
							cur_pos = (base1[0x08] | (base1[0x09] << 8) | (base1[0x0a] << 16)) & rom_mask;
							cur_val = (INT16)(rom[cur_pos] << 8);
						}
						if(cur_val == (INT16)0x8000) {
							k054539_keyoff(info, ch);
							cur_val = 0;
							break;
						}
					}
					break;
				}

				case 0x4: { // 16bit pcm lsb first
					pdelta <<= 1;

					cur_pfrac += delta;
					while(cur_pfrac & ~0xffff) {
						cur_pfrac += fdelta;
						cur_pos += pdelta;

						cur_pval = cur_val;
						cur_val = (INT16)(rom[cur_pos] | rom[cur_pos+1]<<8);
						if(cur_val == (INT16)0x8000 && (base2[1] & 1)) {
							cur_pos = (base1[0x08] | (base1[0x09] << 8) | (base1[0x0a] << 16)) & rom_mask;
							cur_val = (INT16)(rom[cur_pos] | rom[cur_pos+1]<<8);
						}
						if(cur_val == (INT16)0x8000) {
							k054539_keyoff(info, ch);
							cur_val = 0;
							break;
						}
					}
					break;
				}

				case 0x8: { // 4bit dpcm
					cur_pos <<= 1;
					cur_pfrac <<= 1;
					if(cur_pfrac & 0x10000) {
						cur_pfrac &= 0xffff;
						cur_pos |= 1;
					}

					cur_pfrac += delta;
					while(cur_pfrac & ~0xffff) {
						cur_pfrac += fdelta;
						cur_pos += pdelta;

						cur_pval = cur_val;
						cur_val = rom[cur_pos>>1];
						if(cur_val == 0x88 && (base2[1] & 1)) {
							cur_pos = ((base1[0x08] | (base1[0x09] << 8) | (base1[0x0a] << 16)) & rom_mask) << 1;
							cur_val = rom[cur_pos>>1];
						}
						if(cur_val == 0x88) {
							k054539_keyoff(info, ch);
							cur_val = 0;
							break;
						}
						if(cur_pos & 1)
							cur_val >>= 4;
						else
							cur_val &= 15;
						cur_val = cur_pval + dpcm[cur_val];
						if(cur_val < -32768)
							cur_val = -32768;
						else if(cur_val > 32767)
							cur_val = 32767;
					}

					cur_pfrac >>= 1;
					if(cur_pos & 1)
						cur_pfrac |= 0x8000;
					cur_pos >>= 1;
					break;
				}
				default:
#ifdef _DEBUG
					LOG(("Unknown sample type %x for channel %d\n", base2[0] & 0xc, ch));
#endif
					break;
				}
				lval += cur_val * lvol;
				rval += cur_val * rvol;
				rbase[(rdelta + info->reverb_pos) & 0x1fff] += (INT16)(cur_val*rbvol);

				chan->pos = cur_pos;
				chan->pfrac = cur_pfrac;
				chan->pval = cur_pval;
				chan->val = cur_val;

				if(k054539_regupdate(info)) {
					base1[0x0c] = cur_pos     & 0xff;
					base1[0x0d] = cur_pos>> 8 & 0xff;
					base1[0x0e] = cur_pos>>16 & 0xff;
				}
			}
		info->reverb_pos = (info->reverb_pos + 1) & 0x1fff;
		outputs[0][i] = (INT32)lval;
		outputs[1][i] = (INT32)rval;
	}
}


/*static TIMER_CALLBACK( k054539_irq )
{
	k054539_state *info = (k054539_state *)ptr;
	if(info->regs[0x22f] & 0x20)
		info->intf->irq(info->device);
}*/

//static void k054539_init_chip(device_t *device, k054539_state *info)
static int k054539_init_chip(k054539_state *info, int clock)
{
	//int i;

	if (clock < 1000000)	// if < 1 MHz, then it's the sample rate, not the clock
		clock *= 384;	// (for backwards compatibility with old VGM logs)
	info->clock = clock;
	// most of these are done in device_reset
//	memset(info->regs, 0, sizeof(info->regs));
//	memset(info->k054539_posreg_latch, 0, sizeof(info->k054539_posreg_latch)); //*
	info->k054539_flags |= K054539_UPDATE_AT_KEYON; //* make it default until proven otherwise

	info->ram = (unsigned char*)malloc(0x4000);
//	info->reverb_pos = 0;
//	info->cur_ptr = 0;
//	memset(info->ram, 0, 0x4000);

	/*const memory_region *region = (info->intf->rgnoverride != NULL) ? device->machine().region(info->intf->rgnoverride) : device->region();
	info->rom = *region;
	info->rom_size = region->bytes();
	info->rom_mask = 0xffffffffU;
	for(i=0; i<32; i++)
		if((1U<<i) >= info->rom_size) {
			info->rom_mask = (1U<<i) - 1;
			break;
		}*/
	info->rom = NULL;
	info->rom_size = 0;
	info->rom_mask = 0x00;

	//if(info->intf->irq)
		// One or more of the registers must be the timer period
		// And anyway, this particular frequency is probably wrong
		// 480 hz is TRUSTED by gokuparo disco stage - the looping sample doesn't line up otherwise
	//	device->machine().scheduler().timer_pulse(attotime::from_hz(480), FUNC(k054539_irq), 0, info);

	//info->stream = device->machine().sound().stream_alloc(*device, 0, 2, device->clock() / 384, info, k054539_update);

	//device->save_item(NAME(info->regs));
	//device->save_pointer(NAME(info->ram), 0x4000);
	//device->save_item(NAME(info->cur_ptr));
	
	return info->clock / 384;
}

//WRITE8_DEVICE_HANDLER( k054539_w )
void k054539_w(void *_info, offs_t offset, UINT8 data)
{
	//k054539_state *info = get_safe_token(device);
	k054539_state *info = (k054539_state *)_info;

#if 0
	int voice, reg;

	/* The K054539 has behavior like many other wavetable chips including
       the Ensoniq 550x and Gravis GF-1: if a voice is active, writing
       to it's current position is silently ignored.

       Dadandaan depends on this or the vocals go wrong.
       */
	if (offset < 8*0x20)
	{
		voice = offset / 0x20;
		reg = offset & ~0x20;

		if(info->regs[0x22c] & (1<<voice))
		{
			if (reg >= 0xc && reg <= 0xe)
				return;
		}
	}
#endif

	int latch, offs, ch, pan;
	UINT8 *regbase, *regptr, *posptr;

	regbase = info->regs;
	latch = (info->k054539_flags & K054539_UPDATE_AT_KEYON) && (regbase[0x22f] & 1);

	if (latch && offset < 0x100)
	{
		offs = (offset & 0x1f) - 0xc;
		ch = offset >> 5;

		if (offs >= 0 && offs <= 2)
		{
			// latch writes to the position index registers
			info->k054539_posreg_latch[ch][offs] = data;
			return;
		}
	}

	else switch(offset)
	{
		case 0x13f:
			pan = data >= 0x11 && data <= 0x1f ? data - 0x11 : 0x18 - 0x11;
			//if(info->intf->apan)
			//	info->intf->apan(info->device, info->pantab[pan], info->pantab[0xe - pan]);
		break;

		case 0x214:
			if (latch)
			{
				for(ch=0; ch<8; ch++)
				{
					if(data & (1<<ch))
					{
						posptr = &info->k054539_posreg_latch[ch][0];
						regptr = regbase + (ch<<5) + 0xc;

						// update the chip at key-on
						regptr[0] = posptr[0];
						regptr[1] = posptr[1];
						regptr[2] = posptr[2];

						k054539_keyon(info, ch);
					}
				}
			}
			else
			{
				for(ch=0; ch<8; ch++)
					if(data & (1<<ch))
						k054539_keyon(info, ch);
			}
		break;

		case 0x215:
			for(ch=0; ch<8; ch++)
				if(data & (1<<ch))
					k054539_keyoff(info, ch);
		break;

		/*case 0x227:
		{
			attotime period = attotime::from_hz((float)(38 + data) * (clock()/384.0f/14400.0f)) / 2.0f;

			m_timer->adjust(period, 0, period);

			m_timer_state = 0;
			m_timer_handler(m_timer_state);
		}*/
		break;

		case 0x22d:
			if(regbase[0x22e] == 0x80)
				info->cur_zone[info->cur_ptr] = data;
			info->cur_ptr++;
			if(info->cur_ptr == info->cur_limit)
				info->cur_ptr = 0;
		break;

		case 0x22e:
			info->cur_zone =
				data == 0x80 ? info->ram :
				info->rom + 0x20000*data;
			info->cur_limit = data == 0x80 ? 0x4000 : 0x20000;
			info->cur_ptr = 0;
		break;
		
		/*case 0x22f:
			if (!(data & 0x20)) // Disable timer output?
			{
				m_timer_state = 0;
				m_timer_handler(m_timer_state);
			}
		break;*/

		default:
#if 0
			if(regbase[offset] != data) {
				if((offset & 0xff00) == 0) {
					chanoff = offset & 0x1f;
					if(chanoff < 4 || chanoff == 5 ||
					   (chanoff >=8 && chanoff <= 0xa) ||
					   (chanoff >= 0xc && chanoff <= 0xe))
						break;
				}
				if(1 || ((offset >= 0x200) && (offset <= 0x210)))
					break;
				logerror("K054539 %03x = %02x\n", offset, data);
			}
#endif
		break;
	}

	regbase[offset] = data;
}

/*static void reset_zones(k054539_state *info)
{
	int data = info->regs[0x22e];
	info->cur_zone = data == 0x80 ? info->ram : info->rom + 0x20000*data;
	info->cur_limit = data == 0x80 ? 0x4000 : 0x20000;
}*/

//READ8_DEVICE_HANDLER( k054539_r )
UINT8 k054539_r(void *_info, offs_t offset)
{
	//k054539_state *info = get_safe_token(device);
	k054539_state *info = (k054539_state *)_info;
	switch(offset) {
	case 0x22d:
		if(info->regs[0x22f] & 0x10) {
			UINT8 res = info->cur_zone[info->cur_ptr];
			info->cur_ptr++;
			if(info->cur_ptr == info->cur_limit)
				info->cur_ptr = 0;
			return res;
		} else
			return 0;
	case 0x22c:
		break;
	default:
#ifdef _DEBUG
		LOG(("K054539 read %03x\n", offset));
#endif
		break;
	}
	return info->regs[offset];
}

//static DEVICE_START( k054539 )
int device_start_k054539(void **_info, int clock)
{
	//static const k054539_interface defintrf = { 0 };
	int i;
	//k054539_state *info = get_safe_token(device);
	k054539_state *info;

	info = (k054539_state *) calloc(1, sizeof(k054539_state));
	*_info = (void *) info;
	//info->device = device;

	for (i = 0; i < 8; i++)
		info->k054539_gain[i] = 1.0;
	info->k054539_flags = K054539_RESET_FLAGS;

	//info->intf = (device->static_config() != NULL) ? (const k054539_interface *)device->static_config() : &defintrf;

	/*
        I've tried various equations on volume control but none worked consistently.
        The upper four channels in most MW/GX games simply need a significant boost
        to sound right. For example, the bass and smash sound volumes in Violent Storm
        have roughly the same values and the voices in Tokimeki Puzzledama are given
        values smaller than those of the hihats. Needless to say the two K054539 chips
        in Mystic Warriors are completely out of balance. Rather than forcing a
        "one size fits all" function to the voltab the current invert exponential
        appraoch seems most appropriate.
    */
	// Factor the 1/4 for the number of channels in the volume (1/8 is too harsh, 1/2 gives clipping)
	// vol=0 -> no attenuation, vol=0x40 -> -36dB
	for(i=0; i<256; i++)
		info->voltab[i] = pow(10.0, (-36.0 * (double)i / (double)0x40) / 20.0) / 4.0;

	// Pan table for the left channel
	// Right channel is identical with inverted index
	// Formula is such that pan[i]**2+pan[0xe-i]**2 = 1 (constant output power)
	// and pan[0xe] = 1 (full panning)
	for(i=0; i<0xf; i++)
		info->pantab[i] = sqrt((double)i) / sqrt((double)0xe);

	//k054539_init_chip(device, info);

	//device->machine().save().register_postload(save_prepost_delegate(FUNC(reset_zones), info));
	
	for (i = 0; i < 8; i ++)
		info->Muted[i] = 0x00;

	return k054539_init_chip(info, clock);
}

void device_stop_k054539(void *_info)
{
	k054539_state *info = (k054539_state *)_info;
	
	free(info->rom);	info->rom = NULL;
	free(info->ram);	info->ram = NULL;

	free(info);
	
	return;
}

void device_reset_k054539(void *_info)
{
	k054539_state *info = (k054539_state *)_info;
	
	memset(info->regs, 0, sizeof(info->regs));
	memset(info->k054539_posreg_latch, 0, sizeof(info->k054539_posreg_latch));
	//info->k054539_flags |= K054539_UPDATE_AT_KEYON;
	
	info->reverb_pos = 0;
	info->cur_ptr = 0;
	memset(info->ram, 0, 0x4000);
	
	return;
}

void k054539_write_rom(void *_info, offs_t ROMSize, offs_t DataStart, offs_t DataLength,
					   const UINT8* ROMData)
{
	k054539_state *info = (k054539_state *)_info;
	
	if (info->rom_size != ROMSize)
	{
		UINT8 i;
		
		info->rom = (UINT8*)realloc(info->rom, ROMSize);
		info->rom_size = ROMSize;
		memset(info->rom, 0xFF, ROMSize);
		
		info->rom_mask = 0xFFFFFFFF;
		for (i = 0; i < 32; i ++)
		{
			if ((1U << i) >= info->rom_size)
			{
				info->rom_mask = (1 << i) - 1;
				break;
			}
		}
	}
	if (DataStart > ROMSize)
		return;
	if (DataStart + DataLength > ROMSize)
		DataLength = ROMSize - DataStart;
	
	memcpy(info->rom + DataStart, ROMData, DataLength);
	
	return;
}


void k054539_set_mute_mask(void *_info, UINT32 MuteMask)
{
	k054539_state *info = (k054539_state *)_info;
	UINT8 CurChn;
	
	for (CurChn = 0; CurChn < 8; CurChn ++)
		info->Muted[CurChn] = (MuteMask >> CurChn) & 0x01;
	
	return;
}




/**************************************************************************
 * Generic get_info
 **************************************************************************/

/*DEVICE_GET_INFO( k054539 )
{
	switch (state)
	{
		// --- the following bits of info are returned as 64-bit signed integers --- //
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(k054539_state);				break;

		// --- the following bits of info are returned as pointers to data or functions --- //
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME( k054539 );		break;
		case DEVINFO_FCT_STOP:							// nothing //									break;
		case DEVINFO_FCT_RESET:							// nothing //									break;

		// --- the following bits of info are returned as NULL-terminated strings --- //
		case DEVINFO_STR_NAME:							strcpy(info->s, "K054539");						break;
		case DEVINFO_STR_FAMILY:					strcpy(info->s, "Konami custom");				break;
		case DEVINFO_STR_VERSION:					strcpy(info->s, "1.0");							break;
		case DEVINFO_STR_SOURCE_FILE:						strcpy(info->s, __FILE__);						break;
		case DEVINFO_STR_CREDITS:					strcpy(info->s, "Copyright Nicola Salmoria and the MAME Team"); break;
	}
}*/


//DEFINE_LEGACY_SOUND_DEVICE(K054539, k054539);
