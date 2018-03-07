/*
    SCSP (YMF292-F) header
*/

#pragma once

#ifndef __SCSP_H__
#define __SCSP_H__

//#include "devlegcy.h"

/*typedef struct _scsp_interface scsp_interface;
struct _scsp_interface
{
	int roffset;				// offset in the region 
	void (*irq_callback)(device_t *device, int state);	// irq callback
	devcb_write_line   main_irq;
};

void scsp_set_ram_base(device_t *device, void *base);*/


void SCSP_Update(void *chip, stream_sample_t **outputs, int samples);
int device_start_scsp(void **chip, int clock, int options);
void device_stop_scsp(void *chip);
void device_reset_scsp(void *chip);

// SCSP register access
/*READ16_DEVICE_HANDLER( scsp_r );
WRITE16_DEVICE_HANDLER( scsp_w );*/
void scsp_w(void *chip, offs_t offset, UINT8 data);
//UINT8 scsp_r(void *chip, offs_t offset);

// MIDI I/O access (used for comms on Model 2/3)
/*WRITE16_DEVICE_HANDLER( scsp_midi_in );
READ16_DEVICE_HANDLER( scsp_midi_out_r );*/

//void scsp_write_rom(void *chip, offs_t ROMSize, offs_t DataStart, offs_t DataLength,
//					const UINT8* ROMData);
void scsp_write_ram(void *chip, offs_t DataStart, offs_t DataLength, const UINT8* RAMData);
void scsp_set_mute_mask(void *chip, UINT32 MuteMask);

/*extern UINT32* stv_scu;

DECLARE_LEGACY_SOUND_DEVICE(SCSP, scsp);*/

#endif /* __SCSP_H__ */
