#pragma once

/*READ8_DEVICE_HANDLER( sn76496_ready_r );
WRITE8_DEVICE_HANDLER( sn76496_w );
WRITE8_DEVICE_HANDLER( sn76496_stereo_w );

DEVICE_GET_INFO( sn76496 );
DEVICE_GET_INFO( sn76489 );
DEVICE_GET_INFO( sn76489a );
DEVICE_GET_INFO( sn76494 );
DEVICE_GET_INFO( sn94624 );
DEVICE_GET_INFO( ncr7496 );
DEVICE_GET_INFO( gamegear );
DEVICE_GET_INFO( smsiii );

#define SOUND_SN76496 DEVICE_GET_INFO_NAME( sn76496 )
#define SOUND_SN76489 DEVICE_GET_INFO_NAME( sn76489 )
#define SOUND_SN76489A DEVICE_GET_INFO_NAME( sn76489a )
#define SOUND_SN76494 DEVICE_GET_INFO_NAME( sn76494 )
#define SOUND_SN94624 DEVICE_GET_INFO_NAME( sn94624 )
#define SOUND_NCR7496 DEVICE_GET_INFO_NAME( ncr7496 )
#define SOUND_GAMEGEAR DEVICE_GET_INFO_NAME( gamegear )
#define SOUND_SMSIII DEVICE_GET_INFO_NAME( smsiii )*/

UINT8 sn76496_ready_r(void *chip, offs_t offset);
void sn76496_write_reg(void *chip, offs_t offset, UINT8 data);
void sn76496_stereo_w(void *chip, offs_t offset, UINT8 data);

void SN76496Update(void *chip, stream_sample_t **outputs, int samples);
unsigned long int sn76496_start(void **chip, int clock, int shiftregwidth, int noisetaps,
								int negate, int stereo, int clockdivider, int freq0);
void sn76496_shutdown(void *chip);
void sn76496_reset(void *chip);
void sn76496_freq_limiter(int clock, int clockdiv, int sample_rate);
void sn76496_set_mutemask(void *chip, UINT32 MuteMask);
