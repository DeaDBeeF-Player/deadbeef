/***************************************************************************

    Konami 051649 - SCC1 sound as used in Haunted Castle, City Bomber

    This file is pieced together by Bryan McPhail from a combination of
    Namco Sound, Amuse by Cab, Haunted Castle schematics and whoever first
    figured out SCC!

    The 051649 is a 5 channel sound generator, each channel gets its
    waveform from RAM (32 bytes per waveform, 8 bit signed data).

    This sound chip is the same as the sound chip in some Konami
    megaROM cartridges for the MSX. It is actually well researched
    and documented:

        http://bifi.msxnet.org/msxnet/tech/scc.html

    Thanks to Sean Young (sean@mess.org) for some bugfixes.

    K052539 is more or less equivalent to this chip except channel 5
    does not share waveram with channel 4.

***************************************************************************/

#include "mamedef.h"
#include <stdlib.h>
#include <string.h>
//#include "emu.h"
//#include "streams.h"
#include "k051649.h"

#define FREQ_BITS	16
#define DEF_GAIN	8

/* this structure defines the parameters for a channel */
typedef struct
{
	unsigned long counter;
	int frequency;
	int volume;
	int key;
	signed char waveram[32];		/* 19991207.CAB */
	UINT8 Muted;
} k051649_sound_channel;

typedef struct _k051649_state k051649_state;
struct _k051649_state
{
	k051649_sound_channel channel_list[5];

	/* global sound parameters */
	//sound_stream * stream;
	int mclock,rate;

	/* mixer tables and internal buffers */
	INT16 *mixer_table;
	INT16 *mixer_lookup;
	short *mixer_buffer;

	int cur_reg;
	UINT8 test;
};

/*INLINE k051649_state *get_safe_token(running_device *device)
{
	assert(device != NULL);
	assert(device->type() == K051649);
	return (k051649_state *)downcast<legacy_device_base *>(device)->token();
}*/

/* build a table to divide by the number of voices */
static void make_mixer_table(/*running_machine *machine,*/ k051649_state *info, int voices)
{
	int count = voices * 256;
	int i;

	/* allocate memory */
	//info->mixer_table = auto_alloc_array(machine, INT16, 512 * voices);
	info->mixer_table = (INT16*)malloc(sizeof(INT16) * 2 * count);

	/* find the middle of the table */
	info->mixer_lookup = info->mixer_table + count;

	/* fill in the table - 16 bit case */
	for (i = 0; i < count; i++)
	{
		int val = i * DEF_GAIN * 16 / voices;
		//if (val > 32767) val = 32767;
		if (val > 32768) val = 32768;
		info->mixer_lookup[ i] = val;
		info->mixer_lookup[-i] = -val;
	}
}


/* generate sound to the mix buffer */
//static STREAM_UPDATE( k051649_update )
void k051649_update(void *param, stream_sample_t **outputs, int samples)
{
	k051649_state *info = (k051649_state *)param;
	k051649_sound_channel *voice=info->channel_list;
	stream_sample_t *buffer = outputs[0];
	stream_sample_t *buffer2 = outputs[1];
	short *mix;
	int i,j;

	// zap the contents of the mixer buffer
	memset(info->mixer_buffer, 0, samples * sizeof(short));

	for (j=0; j<5; j++) {
		// channel is halted for freq < 9
		if (voice[j].frequency > 8 && ! voice[j].Muted)
		{
			const signed char *w = voice[j].waveram;			/* 19991207.CAB */
			int v=voice[j].volume * voice[j].key;
			int c=voice[j].counter;
			/* Amuse source:  Cab suggests this method gives greater resolution */
			/* Sean Young 20010417: the formula is really: f = clock/(16*(f+1))*/
			int step = (int)(((INT64)info->mclock * (1 << FREQ_BITS)) / (float)((voice[j].frequency + 1) * 16 * (info->rate / 32)) + 0.5);

			mix = info->mixer_buffer;

			// add our contribution
			for (i = 0; i < samples; i++)
			{
				int offs;

				c += step;
				offs = (c >> FREQ_BITS) & 0x1f;
				*mix++ += (w[offs] * v)>>3;
			}

			// update the counter for this voice
			voice[j].counter = c;
		}
	}

	// mix it down
	mix = info->mixer_buffer;
	for (i = 0; i < samples; i++)
		*buffer++ = *buffer2++ = info->mixer_lookup[*mix++];
}

//static DEVICE_START( k051649 )
int device_start_k051649(void **_info, int clock)
{
	//k051649_state *info = get_safe_token(device);
	k051649_state *info;
	UINT8 CurChn;

	info = (k051649_state *) calloc(1, sizeof(k051649_state));
	*_info = (void *) info;
	
	/* get stream channels */
	//info->rate = device->clock()/16;
	//info->stream = stream_create(device, 0, 1, info->rate, info, k051649_update);
	//info->mclock = device->clock();
	info->mclock = clock & 0x7FFFFFFF;
	info->rate = info->mclock / 16;

	/* allocate a buffer to mix into - 1 second's worth should be more than enough */
	//info->mixer_buffer = auto_alloc_array(device->machine, short, 2 * info->rate);
	info->mixer_buffer = (short*)malloc(sizeof(short) * info->rate);

	/* build the mixer table */
	//make_mixer_table(device->machine, info, 5);
	make_mixer_table(info, 5);
	
	for (CurChn = 0; CurChn < 5; CurChn ++)
		info->channel_list[CurChn].Muted = 0x00;

	return info->rate;
}

void device_stop_k051649(void *_info)
{
	k051649_state *info = (k051649_state *)_info;
	
	free(info->mixer_buffer);
	free(info->mixer_table);

	free(info);
	
	return;
}

//static DEVICE_RESET( k051649 )
void device_reset_k051649(void *_info)
{
	//k051649_state *info = get_safe_token(device);
	k051649_state *info = (k051649_state *)_info;
	k051649_sound_channel *voice = info->channel_list;
	int i;

	// reset all the voices
	for (i = 0; i < 5; i++)
	{
		voice[i].frequency = 0;
		voice[i].volume = 0;
		voice[i].counter = 0;
		voice[i].key = 0;
	}
	
	// other parameters
	info->test = 0x00;
	info->cur_reg = 0x00;
	
	return;
}

/********************************************************************************/

//WRITE8_DEVICE_HANDLER( k051649_waveform_w )
void k051649_waveform_w(void *_info, offs_t offset, UINT8 data)
{
	//k051649_state *info = get_safe_token(device);
	k051649_state *info = (k051649_state *)_info;
	
	// waveram is read-only?
	if (info->test & 0x40 || (info->test & 0x80 && offset >= 0x60))
		return;

	//stream_update(info->stream);
	
	if (offset >= 0x60)
	{
		// channel 5 shares waveram with channel 4
		info->channel_list[3].waveram[offset&0x1f]=data;
		info->channel_list[4].waveram[offset&0x1f]=data;
	}
	else
		info->channel_list[offset>>5].waveram[offset&0x1f]=data;
}

//READ8_DEVICE_HANDLER ( k051649_waveform_r )
UINT8 k051649_waveform_r(void *_info, offs_t offset)
{
	//k051649_state *info = get_safe_token(device);
	k051649_state *info = (k051649_state *)_info;
	
	// test-register bits 6/7 expose the internal counter
	if (info->test & 0xc0)
	{
		//stream_update(info->stream);

		if (offset >= 0x60)
			offset += (info->channel_list[3 + (info->test >> 6 & 1)].counter >> FREQ_BITS);
		else if (info->test & 0x40)
			offset += (info->channel_list[offset>>5].counter >> FREQ_BITS);
	}
	return info->channel_list[offset>>5].waveram[offset&0x1f];
}

/* SY 20001114: Channel 5 doesn't share the waveform with channel 4 on this chip */
//WRITE8_DEVICE_HANDLER( k052539_waveform_w )
void k052539_waveform_w(void *_info, offs_t offset, UINT8 data)
{
	//k051649_state *info = get_safe_token(device);
	k051649_state *info = (k051649_state *)_info;
	
	// waveram is read-only?
	if (info->test & 0x40)
		return;

	//stream_update(info->stream);
	info->channel_list[offset>>5].waveram[offset&0x1f]=data;
}

//READ8_DEVICE_HANDLER ( k052539_waveform_r )
UINT8 k052539_waveform_r(void *_info, offs_t offset)
{
	//k051649_state *info = get_safe_token(device);
	k051649_state *info = (k051649_state *)_info;
	
	// test-register bit 6 exposes the internal counter
	if (info->test & 0x40)
	{
		//stream_update(info->stream);
		offset += (info->channel_list[offset>>5].counter >> FREQ_BITS);
	}
	return info->channel_list[offset>>5].waveram[offset&0x1f];
}

//WRITE8_DEVICE_HANDLER( k051649_volume_w )
void k051649_volume_w(void *_info, offs_t offset, UINT8 data)
{
	//k051649_state *info = get_safe_token(device);
	k051649_state *info = (k051649_state *)_info;
	//stream_update(info->stream);
	info->channel_list[offset&0x7].volume=data&0xf;
}

//WRITE8_DEVICE_HANDLER( k051649_frequency_w )
void k051649_frequency_w(void *_info, offs_t offset, UINT8 data)
{
	//k051649_state *info = get_safe_token(device);
	k051649_state *info = (k051649_state *)_info;
	k051649_sound_channel* chn = &info->channel_list[offset >> 1];

	//stream_update(info->stream);
	
	// test-register bit 5 resets the internal counter
	if (info->test & 0x20)
		chn->counter = ~0;
	else if (chn->frequency < 9)
		chn->counter |= ((1 << FREQ_BITS) - 1);

	// update frequency
	if (offset & 1)
		chn->frequency = (chn->frequency & 0x0FF) | ((data << 8) & 0xF00);
	else
		chn->frequency = (chn->frequency & 0xF00) |  (data << 0);
	chn->counter &= 0xFFFF0000;	// Valley Bell: Behaviour according to openMSX
}

//WRITE8_DEVICE_HANDLER( k051649_keyonoff_w )
void k051649_keyonoff_w(void *_info, offs_t offset, UINT8 data)
{
	//k051649_state *info = get_safe_token(device);
	k051649_state *info = (k051649_state *)_info;
	int i;
	//stream_update(info->stream);
	
	for (i = 0; i < 5; i++)
	{
		info->channel_list[i].key=data&1;
		data >>= 1;
	}
}

//WRITE8_MEMBER( k051649_device::k051649_test_w )
void k051649_test_w(void *_info, offs_t offset, UINT8 data)
{
	k051649_state *info = (k051649_state *)_info;
	info->test = data;
}


//READ8_MEMBER ( k051649_device::k051649_test_r )
UINT8 k051649_test_r(void *info, offs_t offset)
{
	// reading the test register sets it to $ff!
	k051649_test_w(info, offset, 0xff);
	return 0xff;
}


void k051649_w(void *_info, offs_t offset, UINT8 data)
{
	k051649_state *info = (k051649_state *)_info;
	
	switch(offset & 1)
	{
	case 0x00:
		info->cur_reg = data;
		break;
	case 0x01:
		switch(offset >> 1)
		{
		case 0x00:
			k051649_waveform_w(info, info->cur_reg, data);
			break;
		case 0x01:
			k051649_frequency_w(info, info->cur_reg, data);
			break;
		case 0x02:
			k051649_volume_w(info, info->cur_reg, data);
			break;
		case 0x03:
			k051649_keyonoff_w(info, info->cur_reg, data);
			break;
		case 0x04:
			k052539_waveform_w(info, info->cur_reg, data);
			break;
		case 0x05:
			k051649_test_w(info, info->cur_reg, data);
			break;
		}
		break;
	}
	
	return;
}


void k051649_set_mute_mask(void *_info, UINT32 MuteMask)
{
	k051649_state *info = (k051649_state *)_info;
	UINT8 CurChn;
	
	for (CurChn = 0; CurChn < 5; CurChn ++)
		info->channel_list[CurChn].Muted = (MuteMask >> CurChn) & 0x01;
	
	return;
}




/**************************************************************************
 * Generic get_info
 **************************************************************************/

/*DEVICE_GET_INFO( k051649 )
{
	switch (state)
	{
		// --- the following bits of info are returned as 64-bit signed integers ---
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(k051649_state);				break;

		// --- the following bits of info are returned as pointers to data or functions ---
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME( k051649 );		break;
		case DEVINFO_FCT_STOP:							// nothing //									break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME( k051649 );		break;

		// --- the following bits of info are returned as NULL-terminated strings ---
		case DEVINFO_STR_NAME:							strcpy(info->s, "K051649");						break;
		case DEVINFO_STR_FAMILY:					strcpy(info->s, "Konami custom");				break;
		case DEVINFO_STR_VERSION:					strcpy(info->s, "1.0");							break;
		case DEVINFO_STR_SOURCE_FILE:						strcpy(info->s, __FILE__);						break;
		case DEVINFO_STR_CREDITS:					strcpy(info->s, "Copyright Nicola Salmoria and the MAME Team"); break;
	}
}*/


//DEFINE_LEGACY_SOUND_DEVICE(K051649, k051649);
