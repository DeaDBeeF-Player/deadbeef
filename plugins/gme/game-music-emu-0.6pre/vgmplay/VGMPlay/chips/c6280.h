#pragma once

//#include "devlegcy.h"

typedef struct _c6280_interface c6280_interface;
struct _c6280_interface
{
	const char *	cpu;
};

/* Function prototypes */
//WRITE8_DEVICE_HANDLER( c6280_w );
//READ8_DEVICE_HANDLER( c6280_r );
void c6280m_w(void* chip, offs_t offset, UINT8 data);
UINT8 c6280m_r(void* chip, offs_t offset);

void c6280m_update(void* param, stream_sample_t **outputs, int samples);
void* device_start_c6280m(int clock, int rate);
void device_stop_c6280m(void* chip);
void device_reset_c6280m(void* chip);

void c6280m_set_mute_mask(void* chip, UINT32 MuteMask);

//DECLARE_LEGACY_SOUND_DEVICE(C6280, c6280);
