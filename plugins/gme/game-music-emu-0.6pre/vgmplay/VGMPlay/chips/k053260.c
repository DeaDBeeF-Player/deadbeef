/*********************************************************

    Konami 053260 PCM Sound Chip

*********************************************************/

#include "mamedef.h"
//#include "emu.h"
#ifdef _DEBUG
#include <stdio.h>
#endif
#include <stdlib.h>
#include <string.h>
#include "k053260.h"

#ifndef NULL
#define NULL	((void *)0)
#endif

/* 2004-02-28: Fixed PPCM decoding. Games sound much better now.*/

#define LOG 0

#define BASE_SHIFT	16

typedef struct _k053260_channel k053260_channel;
struct _k053260_channel
{
	UINT32		rate;
	UINT32		size;
	UINT32		start;
	UINT32		bank;
	UINT32		volume;
	int			play;
	UINT32		pan;
	UINT32		pos;
	int			loop;
	int			ppcm; /* packed PCM ( 4 bit signed ) */
	int			ppcm_data;
	UINT8		Muted;
};

typedef struct _k053260_state k053260_state;
struct _k053260_state
{
	//sound_stream *				channel;
	int							mode;
	int							regs[0x30];
	UINT8						*rom;
	//int							rom_size;
	UINT32							rom_size;
	UINT32						*delta_table;
	k053260_channel				channels[4];
	//const k053260_interface		*intf;
	//device_t				*device;
};

/*INLINE k053260_state *get_safe_token(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == K053260);
	return (k053260_state *)downcast<legacy_device_base *>(device)->token();
}*/


static void InitDeltaTable( k053260_state *ic, int rate, int clock )
{
	int		i;
	double	base = ( double )rate;
	double	max = (double)(clock); /* Hz */
	UINT32 val;

	for( i = 0; i < 0x1000; i++ ) {
		double v = ( double )( 0x1000 - i );
		double target = (max) / v;
		double fixed = ( double )( 1 << BASE_SHIFT );

		if ( target && base ) {
			target = fixed / ( base / target );
			val = ( UINT32 )target;
			if ( val == 0 )
				val = 1;
		} else
			val = 1;

		ic->delta_table[i] = val;
	}
}

//static DEVICE_RESET( k053260 )
void device_reset_k053260(void *_info)
{
	//k053260_state *ic = get_safe_token(device);
	k053260_state *ic = (k053260_state *)_info;
	int i;

	for( i = 0; i < 4; i++ ) {
		ic->channels[i].rate = 0;
		ic->channels[i].size = 0;
		ic->channels[i].start = 0;
		ic->channels[i].bank = 0;
		ic->channels[i].volume = 0;
		ic->channels[i].play = 0;
		ic->channels[i].pan = 0;
		ic->channels[i].pos = 0;
		ic->channels[i].loop = 0;
		ic->channels[i].ppcm = 0;
		ic->channels[i].ppcm_data = 0;
	}
}

INLINE int limit( int val, int max, int min )
{
	if ( val > max )
		val = max;
	else if ( val < min )
		val = min;

	return val;
}

//#define MAXOUT 0x7fff
#define MAXOUT +0x8000
#define MINOUT -0x8000

//static STREAM_UPDATE( k053260_update )
void k053260_update(void *param, stream_sample_t **outputs, int samples)
{
	static const INT8 dpcmcnv[] = { 0,1,2,4,8,16,32,64, -128, -64, -32, -16, -8, -4, -2, -1};

	int i, j, lvol[4], rvol[4], play[4], loop[4], ppcm[4];
	UINT8 *rom[4];
	UINT32 delta[4], end[4], pos[4];
	INT8 ppcm_data[4];
	int dataL, dataR;
	INT8 d;
	k053260_state *ic = (k053260_state *)param;

	/* precache some values */
	for ( i = 0; i < 4; i++ ) {
		if (ic->channels[i].Muted)
		{
			play[i] = 0;
			continue;
		}
		rom[i]= &ic->rom[ic->channels[i].start + ( ic->channels[i].bank << 16 )];
		delta[i] = ic->delta_table[ic->channels[i].rate];
		lvol[i] = ic->channels[i].volume * ic->channels[i].pan;
		rvol[i] = ic->channels[i].volume * ( 8 - ic->channels[i].pan );
		end[i] = ic->channels[i].size;
		pos[i] = ic->channels[i].pos;
		play[i] = ic->channels[i].play;
		loop[i] = ic->channels[i].loop;
		ppcm[i] = ic->channels[i].ppcm;
		ppcm_data[i] = ic->channels[i].ppcm_data;
		if ( ppcm[i] )
			delta[i] /= 2;
	}

		for ( j = 0; j < samples; j++ ) {

			dataL = dataR = 0;

			for ( i = 0; i < 4; i++ ) {
				/* see if the voice is on */
				if ( play[i] ) {
					/* see if we're done */
					if ( ( pos[i] >> BASE_SHIFT ) >= end[i] ) {

						ppcm_data[i] = 0;
						if ( loop[i] )
							pos[i] = 0;
						else {
							play[i] = 0;
							continue;
						}
					}

					if ( ppcm[i] ) { /* Packed PCM */
						/* we only update the signal if we're starting or a real sound sample has gone by */
						/* this is all due to the dynamic sample rate convertion */
						if ( pos[i] == 0 || ( ( pos[i] ^ ( pos[i] - delta[i] ) ) & 0x8000 ) == 0x8000 )

						{
							int newdata;
							if ( pos[i] & 0x8000 ){

								newdata = ((rom[i][pos[i] >> BASE_SHIFT]) >> 4) & 0x0f; /*high nybble*/
							}
							else{
								newdata = ( ( rom[i][pos[i] >> BASE_SHIFT] ) ) & 0x0f; /*low nybble*/
							}

							/*ppcm_data[i] = (( ( ppcm_data[i] * 62 ) >> 6 ) + dpcmcnv[newdata]);

							if ( ppcm_data[i] > 127 )
								ppcm_data[i] = 127;
							else
								if ( ppcm_data[i] < -128 )
									ppcm_data[i] = -128;*/
							ppcm_data[i] += dpcmcnv[newdata];
						}



						d = ppcm_data[i];

						pos[i] += delta[i];
					} else { /* PCM */
						d = rom[i][pos[i] >> BASE_SHIFT];

						pos[i] += delta[i];
					}

					if ( ic->mode & 2 ) {
						dataL += ( d * lvol[i] ) >> 2;
						dataR += ( d * rvol[i] ) >> 2;
					}
				}
			}

			outputs[1][j] = limit( dataL, MAXOUT, MINOUT );
			outputs[0][j] = limit( dataR, MAXOUT, MINOUT );
		}

	/* update the regs now */
	for ( i = 0; i < 4; i++ ) {
		if (ic->channels[i].Muted)
			continue;
		ic->channels[i].pos = pos[i];
		ic->channels[i].play = play[i];
		ic->channels[i].ppcm_data = ppcm_data[i];
	}
}

//static DEVICE_START( k053260 )
int device_start_k053260(void **_info, int clock)
{
	//static const k053260_interface defintrf = { 0 };
	//k053260_state *ic = get_safe_token(device);
	k053260_state *ic;
	//int rate = device->clock() / 32;
	int rate = clock / 32;
	int i;

	ic = (k053260_state *) calloc(1, sizeof(k053260_state));
	*_info = (void *) ic;	
	
	/* Initialize our chip structure */
	//ic->device = device;
	//ic->intf = (device->static_config() != NULL) ? (const k053260_interface *)device->static_config() : &defintrf;

	ic->mode = 0;

	//const memory_region *region = (ic->intf->rgnoverride != NULL) ? device->machine().region(ic->intf->rgnoverride) : device->region();

	//ic->rom = *region;
	//ic->rom_size = region->bytes();
	ic->rom = NULL;
	ic->rom_size = 0x00;

	// has to be done by the player after calling device_start
	//DEVICE_RESET_CALL(k053260);

	for ( i = 0; i < 0x30; i++ )
		ic->regs[i] = 0;

	//ic->delta_table = auto_alloc_array( device->machine(), UINT32, 0x1000 );
	ic->delta_table = (UINT32*)malloc(0x1000 * sizeof(UINT32));

	//ic->channel = device->machine().sound().stream_alloc( *device, 0, 2, rate, ic, k053260_update );

	//InitDeltaTable( ic, rate, device->clock() );
	InitDeltaTable( ic, rate, clock );

	/* register with the save state system */
	/*device->save_item(NAME(ic->mode));
	device->save_item(NAME(ic->regs));

	for ( i = 0; i < 4; i++ )
	{
		device->save_item(NAME(ic->channels[i].rate), i);
		device->save_item(NAME(ic->channels[i].size), i);
		device->save_item(NAME(ic->channels[i].start), i);
		device->save_item(NAME(ic->channels[i].bank), i);
		device->save_item(NAME(ic->channels[i].volume), i);
		device->save_item(NAME(ic->channels[i].play), i);
		device->save_item(NAME(ic->channels[i].pan), i);
		device->save_item(NAME(ic->channels[i].pos), i);
		device->save_item(NAME(ic->channels[i].loop), i);
		device->save_item(NAME(ic->channels[i].ppcm), i);
		device->save_item(NAME(ic->channels[i].ppcm_data), i);
	}*/

	/* setup SH1 timer if necessary */
	//if ( ic->intf->irq )
	//	device->machine().scheduler().timer_pulse( attotime::from_hz(device->clock()) * 32, ic->intf->irq, "ic->intf->irq" );
	
	for (i = 0; i < 4; i ++)
		ic->channels[i].Muted = 0x00;

	return rate;
}

void device_stop_k053260(void *_info)
{
	k053260_state *ic = (k053260_state *)_info;
	
	free(ic->delta_table);
	free(ic->rom);	ic->rom = NULL;

	free(ic);	

	return;
}

INLINE void check_bounds( k053260_state *ic, int channel )
{

	int channel_start = ( ic->channels[channel].bank << 16 ) + ic->channels[channel].start;
	int channel_end = channel_start + ic->channels[channel].size - 1;

	if ( channel_start > ic->rom_size ) {
#ifdef _DEBUG
		logerror("K53260: Attempting to start playing past the end of the ROM ( start = %06x, end = %06x ).\n", channel_start, channel_end );
#endif

		ic->channels[channel].play = 0;

		return;
	}

	if ( channel_end > ic->rom_size ) {
#ifdef _DEBUG
		logerror("K53260: Attempting to play past the end of the ROM ( start = %06x, end = %06x ).\n", channel_start, channel_end );
#endif

		ic->channels[channel].size = ic->rom_size - channel_start;
	}
#ifdef _DEBUG
	if (LOG) logerror("K053260: Sample Start = %06x, Sample End = %06x, Sample rate = %04x, PPCM = %s\n", channel_start, channel_end, ic->channels[channel].rate, ic->channels[channel].ppcm ? "yes" : "no" );
#endif
}

//WRITE8_DEVICE_HANDLER( k053260_w )
void k053260_w(void *_info, offs_t offset, UINT8 data)
{
	int i, t;
	int r = offset;
	int v = data;

	//k053260_state *ic = get_safe_token(device);
	k053260_state *ic = (k053260_state *)_info;

	if ( r > 0x2f ) {
#ifdef _DEBUG
		logerror("K053260: Writing past registers\n" );
#endif
		return;
	}

	 //ic->channel->update();

	/* before we update the regs, we need to check for a latched reg */
	if ( r == 0x28 ) {
		t = ic->regs[r] ^ v;

		for ( i = 0; i < 4; i++ ) {
			if ( t & ( 1 << i ) ) {
				if ( v & ( 1 << i ) ) {
					ic->channels[i].play = 1;
					ic->channels[i].pos = 0;
					ic->channels[i].ppcm_data = 0;
					check_bounds( ic, i );
				} else
					ic->channels[i].play = 0;
			}
		}

		ic->regs[r] = v;
		return;
	}

	/* update regs */
	ic->regs[r] = v;

	/* communication registers */
	if ( r < 8 )
		return;

	/* channel setup */
	if ( r < 0x28 ) {
		int channel = ( r - 8 ) / 8;

		switch ( ( r - 8 ) & 0x07 ) {
			case 0: /* sample rate low */
				ic->channels[channel].rate &= 0x0f00;
				ic->channels[channel].rate |= v;
			break;

			case 1: /* sample rate high */
				ic->channels[channel].rate &= 0x00ff;
				ic->channels[channel].rate |= ( v & 0x0f ) << 8;
			break;

			case 2: /* size low */
				ic->channels[channel].size &= 0xff00;
				ic->channels[channel].size |= v;
			break;

			case 3: /* size high */
				ic->channels[channel].size &= 0x00ff;
				ic->channels[channel].size |= v << 8;
			break;

			case 4: /* start low */
				ic->channels[channel].start &= 0xff00;
				ic->channels[channel].start |= v;
			break;

			case 5: /* start high */
				ic->channels[channel].start &= 0x00ff;
				ic->channels[channel].start |= v << 8;
			break;

			case 6: /* bank */
				ic->channels[channel].bank = v & 0xff;
			break;

			case 7: /* volume is 7 bits. Convert to 8 bits now. */
				ic->channels[channel].volume = ( ( v & 0x7f ) << 1 ) | ( v & 1 );
			break;
		}

		return;
	}

	switch( r ) {
		case 0x2a: /* loop, ppcm */
			for ( i = 0; i < 4; i++ )
				ic->channels[i].loop = ( v & ( 1 << i ) ) != 0;

			for ( i = 4; i < 8; i++ )
				ic->channels[i-4].ppcm = ( v & ( 1 << i ) ) != 0;
		break;

		case 0x2c: /* pan */
			ic->channels[0].pan = v & 7;
			ic->channels[1].pan = ( v >> 3 ) & 7;
		break;

		case 0x2d: /* more pan */
			ic->channels[2].pan = v & 7;
			ic->channels[3].pan = ( v >> 3 ) & 7;
		break;

		case 0x2f: /* control */
			ic->mode = v & 7;
			/* bit 0 = read ROM */
			/* bit 1 = enable sound output */
			/* bit 2 = unknown */
		break;
	}
}

//READ8_DEVICE_HANDLER( k053260_r )
UINT8 k053260_r(void *_info, offs_t offset)
{
	//k053260_state *ic = get_safe_token(device);
	k053260_state *ic = (k053260_state *)_info;

	switch ( offset ) {
		case 0x29: /* channel status */
			{
				int i, status = 0;

				for ( i = 0; i < 4; i++ )
					status |= ic->channels[i].play << i;

				return status;
			}
		break;

		case 0x2e: /* read ROM */
			if ( ic->mode & 1 )
			{
				UINT32 offs = ic->channels[0].start + ( ic->channels[0].pos >> BASE_SHIFT ) + ( ic->channels[0].bank << 16 );

				ic->channels[0].pos += ( 1 << 16 );

				if ( offs > ic->rom_size ) {
					//logerror("%s: K53260: Attempting to read past ROM size in ROM Read Mode (offs = %06x, size = %06x).\n", device->machine().describe_context(),offs,ic->rom_size );
#ifdef _DEBUG
					logerror("K53260: Attempting to read past ROM size in ROM Read Mode (offs = %06x, size = %06x).\n", offs,ic->rom_size );
#endif

					return 0;
				}

				return ic->rom[offs];
			}
		break;
	}

	return ic->regs[offset];
}

void k053260_write_rom(void *_info, offs_t ROMSize, offs_t DataStart, offs_t DataLength,
					   const UINT8* ROMData)
{
	k053260_state *info = (k053260_state *)_info;
	
	if (info->rom_size != ROMSize)
	{
		info->rom = (UINT8*)realloc(info->rom, ROMSize);
		info->rom_size = ROMSize;
		memset(info->rom, 0xFF, ROMSize);
	}
	if (DataStart > ROMSize)
		return;
	if (DataStart + DataLength > ROMSize)
		DataLength = ROMSize - DataStart;
	
	memcpy(info->rom + DataStart, ROMData, DataLength);
	
	return;
}


void k053260_set_mute_mask(void *_info, UINT32 MuteMask)
{
	k053260_state *info = (k053260_state *)_info;
	UINT8 CurChn;
	
	for (CurChn = 0; CurChn < 4; CurChn ++)
		info->channels[CurChn].Muted = (MuteMask >> CurChn) & 0x01;
	
	return;
}

/**************************************************************************
 * Generic get_info
 **************************************************************************/

/*DEVICE_GET_INFO( k053260 )
{
	switch (state)
	{
		// --- the following bits of info are returned as 64-bit signed integers --- //
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(k053260_state);				break;

		// --- the following bits of info are returned as pointers to data or functions --- //
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME( k053260 );		break;
		case DEVINFO_FCT_STOP:							// nothing //									break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME( k053260 );		break;

		// --- the following bits of info are returned as NULL-terminated strings --- //
		case DEVINFO_STR_NAME:							strcpy(info->s, "K053260");						break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "Konami custom");				break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");							break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);						break;
		case DEVINFO_STR_CREDITS:						strcpy(info->s, "Copyright Nicola Salmoria and the MAME Team"); break;
	}
}


DEFINE_LEGACY_SOUND_DEVICE(K053260, k053260);*/
