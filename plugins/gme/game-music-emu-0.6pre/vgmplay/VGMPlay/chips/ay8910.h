#pragma once

//#include "devcb.h"
#define DEVCB_TYPE_NULL				(0)
#define DEVCB_NULL							{ DEVCB_TYPE_NULL }

/*
AY-3-8910A: 2 I/O ports
AY-3-8912A: 1 I/O port
AY-3-8913A: 0 I/O port
AY8930: upper compatible with 8910.
In extended mode, it has higher resolution and duty ratio setting
YM2149: higher resolution
YM3439: same as 2149
YMZ284: 0 I/O port, different clock divider
YMZ294: 0 I/O port
*/

#define ALL_8910_CHANNELS -1

/* Internal resistance at Volume level 7. */

#define AY8910_INTERNAL_RESISTANCE	(356)
#define YM2149_INTERNAL_RESISTANCE	(353)

/*
 * Default values for resistor loads.
 * The macro should be used in AY8910interface if
 * the real values are unknown.
 */
#define AY8910_DEFAULT_LOADS		{1000, 1000, 1000}

/*
 * The following is used by all drivers not reviewed yet.
 * This will like the old behaviour, output between
 * 0 and 7FFF
 */
#define AY8910_LEGACY_OUTPUT		(1)

/*
 * Specifing the next define will simulate the special
 * cross channel mixing if outputs are tied together.
 * The driver will only provide one stream in this case.
 */
#define AY8910_SINGLE_OUTPUT		(2)

/*
 * The follwoing define is the default behaviour.
 * Output level 0 is 0V and 7ffff corresponds to 5V.
 * Use this to specify that a discrete mixing stage
 * follows.
 */
#define AY8910_DISCRETE_OUTPUT		(4)

/*
 * The follwoing define causes the driver to output
 * raw volume levels, i.e. 0 .. 15 and 0..31.
 * This is intended to be used in a subsequent
 * mixing modul (i.e. mpatrol ties 6 channels from
 * AY-3-8910 together). Do not use it now.
 */
/* TODO: implement mixing module */
#define AY8910_RAW_OUTPUT			(8)

#define AY8910_ZX_STEREO			0x80
/*
* This define specifies the initial state of YM2149
* pin 26 (SEL pin). By default it is set to high,
* compatible with AY8910.
*/
/* TODO: make it controllable while it's running (used by any hw???) */
#define YM2149_PIN26_HIGH           (0x00) /* or N/C */
#define YM2149_PIN26_LOW            (0x10)

typedef struct _ay8910_interface ay8910_interface;
struct _ay8910_interface
{
	int					flags;			/* Flags */
	int					res_load[3]; 	/* Load on channel in ohms */
	//devcb_read8			portAread;
	//devcb_read8			portBread;
	//devcb_write8		portAwrite;
	//devcb_write8		portBwrite;
};


//void ay8910_set_volume(UINT8 ChipID,int channel,int volume);

/*READ8_DEVICE_HANDLER( ay8910_r );
WITE8_DEVICE_HANDLER( ay8910_address_w );
WRITE8_DEVICE_HANDLER( ay8910_data_w );*/
/*UINT8 ay8910_r(UINT8 ChipID, offs_t offset);
void ay8910_address_w(UINT8 ChipID, offs_t offset, UINT8 data);
void ay8910_data_w(UINT8 ChipID, offs_t offset, UINT8 data);*/

/* use this when BC1 == A0; here, BC1=0 selects 'data' and BC1=1 selects 'latch address' */
//WRITE8_DEVICE_HANDLER( ay8910_data_address_w );
//void ay8910_data_address_w(UINT8 ChipID, offs_t offset, UINT8 data);

/* use this when BC1 == !A0; here, BC1=0 selects 'latch address' and BC1=1 selects 'data' */
//WRITE8_DEVICE_HANDLER( ay8910_address_data_w );
//void ay8910_address_data_w(UINT8 ChipID, offs_t offset, UINT8 data);


/*********** An interface for SSG of YM2203 ***********/

//void *ay8910_start_ym(void *infoptr, sound_type chip_type, const device_config *device, int clock, const ay8910_interface *intf);
void *ay8910_start_ym(void *infoptr, unsigned char chip_type, int clock, const ay8910_interface *intf);

void ay8910_stop_ym(void *chip);
void ay8910_reset_ym(void *chip);
void ay8910_set_clock_ym(void *chip, int clock);
void ay8910_write_ym(void *chip, int addr, int data);
int ay8910_read_ym(void *chip);

//void ay8910_update(UINT8 ChipID, stream_sample_t **outputs, int samples);
void ay8910_update_one(void *param, stream_sample_t **outputs, int samples);
int ay8910_start(void **chip, int clock, UINT8 chip_type, UINT8 Flags);
/*int device_start_ay8910(UINT8 ChipID, int clock, unsigned char chip_type, unsigned char Flags);
void device_stop_ay8910(UINT8 ChipID);
void device_reset_ay8910(UINT8 ChipID);*/

void ay8910_set_mute_mask_ym(void *chip, UINT32 MuteMask);
//void ay8910_set_mute_mask(UINT8 ChipID, UINT32 MuteMask);
void ay8910_set_srchg_cb_ym(void *chip, SRATE_CALLBACK CallbackFunc, void* DataPtr);

/*DEVICE_GET_INFO( ay8910 );
DEVICE_GET_INFO( ay8912 );
DEVICE_GET_INFO( ay8913 );
DEVICE_GET_INFO( ay8930 );
DEVICE_GET_INFO( ym2149 );
DEVICE_GET_INFO( ym3439 );
DEVICE_GET_INFO( ymz284 );
DEVICE_GET_INFO( ymz294 );

#define SOUND_AY8910 DEVICE_GET_INFO_NAME( ay8910 )
#define SOUND_AY8912 DEVICE_GET_INFO_NAME( ay8912 )
#define SOUND_AY8913 DEVICE_GET_INFO_NAME( ay8913 )
#define SOUND_AY8930 DEVICE_GET_INFO_NAME( ay8930 )
#define SOUND_YM2149 DEVICE_GET_INFO_NAME( ym2149 )
#define SOUND_YM3439 DEVICE_GET_INFO_NAME( ym3439 )
#define SOUND_YMZ284 DEVICE_GET_INFO_NAME( ymz284 )
#define SOUND_YMZ294 DEVICE_GET_INFO_NAME( ymz294 )*/
