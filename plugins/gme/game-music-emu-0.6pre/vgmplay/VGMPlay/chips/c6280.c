/*
    HuC6280 sound chip emulator
    by Charles MacDonald
    E-mail: cgfm2@hotmail.com
    WWW: http://cgfm2.emuviews.com

    Thanks to:

    - Paul Clifford for his PSG documentation.
    - Richard Bannister for the TGEmu-specific sound updating code.
    - http://www.uspto.gov for the PSG patents.
    - All contributors to the tghack-list.

    Changes:

    (03/30/2003)
    - Removed TGEmu specific code and added support functions for MAME.
    - Modified setup code to handle multiple chips with different clock and
      volume settings.

    Missing features / things to do:

    - Add LFO support. But do any games actually use it?

    - Add shared index for waveform playback and sample writes. Almost every
      game will reset the index prior to playback so this isn't an issue.

    - While the noise emulation is complete, the data for the pseudo-random
      bitstream is calculated by machine.rand() and is not a representation of what
      the actual hardware does.

    For some background on Hudson Soft's C62 chipset:

    - http://www.hudsonsoft.net/ww/about/about.html
    - http://www.hudson.co.jp/corp/eng/coinfo/history.html

    Legal information:

    Copyright Charles MacDonald

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

//#include "emu.h"
#include <stdlib.h>	// for rand()
#include <string.h>	// for memset()
#include <math.h>	// for pow()
#include "mamedef.h"
#include "c6280.h"

typedef struct {
    UINT16 frequency;
    UINT8 control;
    UINT8 balance;
    UINT8 waveform[32];
    UINT8 index;
    INT16 dda;
    UINT8 noise_control;
    UINT32 noise_counter;
    UINT32 counter;
	UINT8 Muted;
} t_channel;

typedef struct {
	//sound_stream *stream;
	//device_t *device;
	//device_t *cpudevice;
    UINT8 select;
    UINT8 balance;
    UINT8 lfo_frequency;
    UINT8 lfo_control;
    t_channel channel[8];	// is 8, because: p->select = data & 0x07;
    INT16 volume_table[32];
    UINT32 noise_freq_tab[32];
    UINT32 wave_freq_tab[4096];
} c6280_t;

/*INLINE c6280_t *get_safe_token(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == C6280);
	return (c6280_t *)downcast<legacy_device_base *>(device)->token();
}*/


/* only needed for io_buffer */
//#include "cpu/h6280/h6280.h"


static void c6280_init(/*device_t *device,*/ c6280_t *p, double clk, double rate)
{
	//const c6280_interface *intf = (const c6280_interface *)device->static_config();
    int i;
    double step;

    /* Loudest volume level for table */
    //double level = 65535.0 / 6.0 / 32.0;
	double level = 65536.0 / 6.0 / 32.0;

    /* Clear context */
    memset(p, 0, sizeof(c6280_t));

    //p->device = device;
    //p->cpudevice = device->machine().device(intf->cpu);
    //if (p->cpudevice == NULL)
    //	fatalerror("c6280_init: no CPU found with tag of '%s'\n", device->tag());

    /* Make waveform frequency table */
    for(i = 0; i < 4096; i += 1)
    {
        step = ((clk / rate) * 4096) / (i+1);
        p->wave_freq_tab[(1 + i) & 0xFFF] = (UINT32)step;
    }

    /* Make noise frequency table */
    for(i = 0; i < 32; i += 1)
    {
        step = ((clk / rate) * 32) / (i+1);
        p->noise_freq_tab[i] = (UINT32)step;
    }

    /* Make volume table */
    /* PSG has 48dB volume range spread over 32 steps */
    step = 48.0 / 32.0;
    for(i = 0; i < 31; i++)
    {
        p->volume_table[i] = (UINT16)level;
        level /= pow(10.0, step / 20.0);
    }
    p->volume_table[31] = 0;
}


static void c6280_write(c6280_t *p, int offset, int data)
{
    t_channel *q = &p->channel[p->select];

    /* Update stream */
    //p->stream->update();

    switch(offset & 0x0F)
    {
        case 0x00: /* Channel select */
            p->select = data & 0x07;
            break;

        case 0x01: /* Global balance */
            p->balance  = data;
            break;

        case 0x02: /* Channel frequency (LSB) */
            q->frequency = (q->frequency & 0x0F00) | data;
            q->frequency &= 0x0FFF;
            break;

        case 0x03: /* Channel frequency (MSB) */
            q->frequency = (q->frequency & 0x00FF) | (data << 8);
            q->frequency &= 0x0FFF;
            break;

        case 0x04: /* Channel control (key-on, DDA mode, volume) */

            /* 1-to-0 transition of DDA bit resets waveform index */
            if((q->control & 0x40) && ((data & 0x40) == 0))
            {
                q->index = 0;
            }
            q->control = data;
            break;

        case 0x05: /* Channel balance */
            q->balance = data;
            break;

        case 0x06: /* Channel waveform data */

            switch(q->control & 0xC0)
            {
                case 0x00:
                    q->waveform[q->index & 0x1F] = data & 0x1F;
                    q->index = (q->index + 1) & 0x1F;
                    break;

                case 0x40:
                    break;

                case 0x80:
                    q->waveform[q->index & 0x1F] = data & 0x1F;
                    q->index = (q->index + 1) & 0x1F;
                    break;

                case 0xC0:
                    q->dda = data & 0x1F;
                    break;
            }

            break;

        case 0x07: /* Noise control (enable, frequency) */
            q->noise_control = data;
            break;

        case 0x08: /* LFO frequency */
            p->lfo_frequency = data;
            break;

        case 0x09: /* LFO control (enable, mode) */
            p->lfo_control = data;
            break;

        default:
            break;
    }
}


//static STREAM_UPDATE( c6280_update )
void c6280m_update(void* param, stream_sample_t **outputs, int samples)
{
    static const int scale_tab[] = {
        0x00, 0x03, 0x05, 0x07, 0x09, 0x0B, 0x0D, 0x0F,
        0x10, 0x13, 0x15, 0x17, 0x19, 0x1B, 0x1D, 0x1F
    };
    int ch;
    int i;
    c6280_t *p = (c6280_t *)param;

    int lmal = (p->balance >> 4) & 0x0F;
    int rmal = (p->balance >> 0) & 0x0F;
    int vll, vlr;

    lmal = scale_tab[lmal];
    rmal = scale_tab[rmal];

    /* Clear buffer */
    for(i = 0; i < samples; i++)
    {
        outputs[0][i] = 0;
        outputs[1][i] = 0;
    }

    for(ch = 0; ch < 6; ch++)
    {
        /* Only look at enabled channels */
        if((p->channel[ch].control & 0x80) && ! p->channel[ch].Muted)
        {
            int lal = (p->channel[ch].balance >> 4) & 0x0F;
            int ral = (p->channel[ch].balance >> 0) & 0x0F;
            int al  = p->channel[ch].control & 0x1F;

            lal = scale_tab[lal];
            ral = scale_tab[ral];

            /* Calculate volume just as the patent says */
            vll = (0x1F - lal) + (0x1F - al) + (0x1F - lmal);
            if(vll > 0x1F) vll = 0x1F;

            vlr = (0x1F - ral) + (0x1F - al) + (0x1F - rmal);
            if(vlr > 0x1F) vlr = 0x1F;

            vll = p->volume_table[vll];
            vlr = p->volume_table[vlr];

            /* Check channel mode */
            if((ch >= 4) && (p->channel[ch].noise_control & 0x80))
            {
                /* Noise mode */
                UINT32 step = p->noise_freq_tab[(p->channel[ch].noise_control & 0x1F) ^ 0x1F];
                for(i = 0; i < samples; i += 1)
                {
                    static int data = 0;
                    p->channel[ch].noise_counter += step;
                    if(p->channel[ch].noise_counter >= 0x800)
                    {
                        //data = (p->device->machine().rand() & 1) ? 0x1F : 0;
						data = (rand() & 1) ? 0x1F : 0;
                    }
                    p->channel[ch].noise_counter &= 0x7FF;
                    outputs[0][i] += (INT16)(vll * (data - 16));
                    outputs[1][i] += (INT16)(vlr * (data - 16));
                }
            }
            else
            if(p->channel[ch].control & 0x40)
            {
                /* DDA mode */
                for(i = 0; i < samples; i++)
                {
                    outputs[0][i] += (INT16)(vll * (p->channel[ch].dda - 16));
                    outputs[1][i] += (INT16)(vlr * (p->channel[ch].dda - 16));
                }
            }
            else
            {
                /* Waveform mode */
                UINT32 step = p->wave_freq_tab[p->channel[ch].frequency];
                for(i = 0; i < samples; i += 1)
                {
                    int offset;
                    INT16 data;
                    offset = (p->channel[ch].counter >> 12) & 0x1F;
                    p->channel[ch].counter += step;
                    p->channel[ch].counter &= 0x1FFFF;
                    data = p->channel[ch].waveform[offset];
                    outputs[0][i] += (INT16)(vll * (data - 16));
                    outputs[1][i] += (INT16)(vlr * (data - 16));
                }
            }
        }
    }
}


/*--------------------------------------------------------------------------*/
/* MAME specific code                                                       */
/*--------------------------------------------------------------------------*/

//static DEVICE_START( c6280 )
void* device_start_c6280m(int clock, int rate)
{
    //int rate = device->clock()/16;
    //c6280_t *info = get_safe_token(device);
	c6280_t *info;
	UINT8 CurChn;

	info = (c6280_t*)malloc(sizeof(c6280_t));
	if (info == NULL)
		return 0;
	memset(info, 0x00, sizeof(c6280_t));
	
    /* Initialize PSG emulator */
    //c6280_init(device, info, device->clock(), rate);
	c6280_init(info, clock & 0x7FFFFFFF, rate);

    /* Create stereo stream */
    //info->stream = device->machine().sound().stream_alloc(*device, 0, 2, rate, info, c6280_update);
	
	for (CurChn = 0; CurChn < 6; CurChn ++)
		info->channel[CurChn].Muted = 0x00;
	
	return info;
}

void device_stop_c6280m(void* chip)
{
	c6280_t *info = (c6280_t *)chip;
	
	free(info);
	
	return;
}

void device_reset_c6280m(void* chip)
{
	c6280_t *info = (c6280_t *)chip;
	UINT8 CurChn;
	t_channel* TempChn;
	
	info->select = 0x00;
	info->balance = 0x00;
	info->lfo_frequency = 0x00;
	info->lfo_control = 0x00;
	
	for (CurChn = 0; CurChn < 6; CurChn ++)
	{
		TempChn = &info->channel[CurChn];
		
		TempChn->frequency = 0x00;
		TempChn->control = 0x00;
		TempChn->balance = 0x00;
		memset(TempChn->waveform, 0x00, 0x20);
		TempChn->index = 0x00;
		TempChn->dda = 0x00;
		TempChn->noise_control = 0x00;
		TempChn->noise_counter = 0x00;
		TempChn->counter = 0x00;
	}
	
	return;
}

//READ8_DEVICE_HANDLER( c6280_r )
UINT8 c6280m_r(void* chip, offs_t offset)
{
    //c6280_t *info = get_safe_token(device);
	c6280_t *info = (c6280_t *)chip;
	//return h6280io_get_buffer(info->cpudevice);
	if (offset == 0)
		return info->select;
	return 0;
}

//WRITE8_DEVICE_HANDLER( c6280_w )
void c6280m_w(void* chip, offs_t offset, UINT8 data)
{
    //c6280_t *info = get_safe_token(device);
	c6280_t *info = (c6280_t *)chip;
	//h6280io_set_buffer(info->cpudevice, data);
	c6280_write(info, offset, data);
}


void c6280m_set_mute_mask(void* chip, UINT32 MuteMask)
{
	c6280_t *info = (c6280_t *)chip;
	UINT8 CurChn;
	
	for (CurChn = 0; CurChn < 6; CurChn ++)
		info->channel[CurChn].Muted = (MuteMask >> CurChn) & 0x01;
	
	return;
}



/**************************************************************************
 * Generic get_info
 **************************************************************************/

/*DEVICE_GET_INFO( c6280 )
{
	switch (state)
	{
		// --- the following bits of info are returned as 64-bit signed integers ---
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(c6280_t);						break;

		// --- the following bits of info are returned as pointers to data or functions ---
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME( c6280 );			break;
		case DEVINFO_FCT_STOP:							// nothing //									break;
		case DEVINFO_FCT_RESET:							// nothing //									break;

		// --- the following bits of info are returned as NULL-terminated strings ---
		case DEVINFO_STR_NAME:							strcpy(info->s, "HuC6280");						break;
		case DEVINFO_STR_FAMILY:					strcpy(info->s, "????");						break;
		case DEVINFO_STR_VERSION:					strcpy(info->s, "1.0");							break;
		case DEVINFO_STR_SOURCE_FILE:						strcpy(info->s, __FILE__);						break;
		case DEVINFO_STR_CREDITS:					strcpy(info->s, "Copyright Nicola Salmoria and the MAME Team"); break;
	}
}


DEFINE_LEGACY_SOUND_DEVICE(C6280, c6280);*/
