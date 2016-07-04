// ChipMapper.c - Handles Chip Write (including OPL Hardware Support)

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <wchar.h>
#include "stdbool.h"

#include "chips/mamedef.h"

#include "chips/ChipIncl.h"

#include "VGMPlay.h"

#include "ChipMapper.h"

// To finish filling out later
UINT8 chip_reg_read(void *param, UINT8 ChipType, UINT8 ChipID, UINT8 Port, UINT8 Offset)
{
	VGM_PLAYER* p = (VGM_PLAYER *) param;
	switch(ChipType)
	{
	case 0x1B:	// HuC6280
		return c6280_r(p->huc6280[ChipID], Offset);
	}
	return 0;
}

void chip_reg_write(void *param, UINT8 ChipType, UINT8 ChipID,
					UINT8 Port, UINT8 Offset, UINT8 Data)
{
	VGM_PLAYER* p = (VGM_PLAYER *) param;
		switch(ChipType)
		{
		case 0x00:	// SN76496
			sn764xx_w(p->sn764xx[ChipID], Port, Data);
			break;
		case 0x01:	// YM2413
			ym2413_w(p->ym2413[ChipID], 0x00, Offset);
			ym2413_w(p->ym2413[ChipID], 0x01, Data);
			break;
		case 0x02:	// YM2612
			ym2612_w(p->ym2612[ChipID], (Port << 1) | 0x00, Offset);
			ym2612_w(p->ym2612[ChipID], (Port << 1) | 0x01, Data);
			break;
		case 0x03:	// YM2151
			ym2151_w(p->ym2151[ChipID], 0x00, Offset);
			ym2151_w(p->ym2151[ChipID], 0x01, Data);
			break;
		case 0x04:	// SegaPCM
			break;
		case 0x05:	// RF5C68
			rf5c68_w(p->rf5c68, Offset, Data);
			break;
		case 0x06:	// YM2203
			ym2203_w(p->ym2203[ChipID], 0x00, Offset);
			ym2203_w(p->ym2203[ChipID], 0x01, Data);
			break;
		case 0x07:	// YM2608
			ym2608_w(p->ym2608[ChipID], (Port << 1) | 0x00, Offset);
			ym2608_w(p->ym2608[ChipID], (Port << 1) | 0x01, Data);
			break;
		case 0x08:	// YM2610/YM2610B
			ym2610_w(p->ym2610[ChipID], (Port << 1) | 0x00, Offset);
			ym2610_w(p->ym2610[ChipID], (Port << 1) | 0x01, Data);
			break;
		case 0x09:	// YM3812
			ym3812_w(p->ym3812[ChipID], 0x00, Offset);
			ym3812_w(p->ym3812[ChipID], 0x01, Data);
			break;
		case 0x0A:	// YM3526
			ym3526_w(p->ym3526[ChipID], 0x00, Offset);
			ym3526_w(p->ym3526[ChipID], 0x01, Data);
			break;
		case 0x0B:	// Y8950
			y8950_w(p->y8950[ChipID], 0x00, Offset);
			y8950_w(p->y8950[ChipID], 0x01, Data);
			break;
		case 0x0C:	// YMF262
			ymf262_w(p->ymf262[ChipID], (Port << 1) | 0x00, Offset);
			ymf262_w(p->ymf262[ChipID], (Port << 1) | 0x01, Data);
			break;
		case 0x0D:	// YMF278B
			ymf278b_w(p->ymf278b[ChipID], (Port << 1) | 0x00, Offset);
			ymf278b_w(p->ymf278b[ChipID], (Port << 1) | 0x01, Data);
			break;
		case 0x0E:	// YMF271
			ymf271_w(p->ymf271[ChipID], (Port << 1) | 0x00, Offset);
			ymf271_w(p->ymf271[ChipID], (Port << 1) | 0x01, Data);
			break;
		case 0x0F:	// YMZ280B
			ymz280b_w(p->ymz280b[ChipID], 0x00, Offset);
			ymz280b_w(p->ymz280b[ChipID], 0x01, Data);
			break;
		case 0x10:	// RF5C164
			rf5c164_w(p->rf5c164, Offset, Data);
			break;
		case 0x11:	// PWM
			pwm_chn_w(p->pwm, Port, (Offset << 8) | (Data << 0));
			break;
		case 0x12:	// AY8910
			ayxx_w(p->ay8910[ChipID], 0x00, Offset);
			ayxx_w(p->ay8910[ChipID], 0x01, Data);
			break;
		case 0x13:	// GameBoy
			gb_sound_w(p->gbdmg[ChipID], Offset, Data);
			break;
		case 0x14:	// NES APU
			nes_w(p->nesapu[ChipID], Offset, Data);
			break;
		case 0x15:	// MultiPCM
			multipcm_w(p->multipcm[ChipID], Offset, Data);
			break;
		case 0x16:	// UPD7759
			upd7759_write(p->upd7759[ChipID], Offset, Data);
			break;
		case 0x17:	// OKIM6258
			okim6258_write(p->okim6258[ChipID], Offset, Data);
			break;
		case 0x18:	// OKIM6295
			okim6295_w(p->okim6295[ChipID], Offset, Data);
			break;
		case 0x19:	// K051649 / SCC1
			k051649_w(p->k051649[ChipID], (Port << 1) | 0x00, Offset);
			k051649_w(p->k051649[ChipID], (Port << 1) | 0x01, Data);
			break;
		case 0x1A:	// K054539
			k054539_w(p->k054539[ChipID], (Port << 8) | (Offset << 0), Data);
			break;
		case 0x1B:	// HuC6280
			c6280_w(p->huc6280[ChipID], Offset, Data);
			break;
		case 0x1C:	// C140
			c140_w(p->c140[ChipID], (Port << 8) | (Offset << 0), Data);
			break;
		case 0x1D:	// K053260
			k053260_w(p->k053260[ChipID], Offset, Data);
			break;
		case 0x1E:	// Pokey
			pokey_w(p->pokey[ChipID], Offset, Data);
			break;
		case 0x1F:	// QSound
			qsound_w(p->qsound[ChipID], 0x00, Port);	// Data MSB
			qsound_w(p->qsound[ChipID], 0x01, Offset);	// Data LSB
			qsound_w(p->qsound[ChipID], 0x02, Data);	// Register
			break;
		case 0x20:	// YMF292/SCSP
			scsp_w(p->scsp[ChipID], (Port << 8) | (Offset << 0), Data);
			break;
		case 0x21:	// WonderSwan
			ws_audio_port_write(p->wswan[ChipID], 0x80 | Offset, Data);
			break;
		case 0x22:	// VSU
			VSU_Write(p->vsu[ChipID], (Port << 8) | (Offset << 0), Data);
			break;
		case 0x23:	// SAA1099
			saa1099_control_w(p->saa1099[ChipID], 0, Offset);
			saa1099_data_w(p->saa1099[ChipID], 0, Data);
			break;
		case 0x24:	// ES5503
			es5503_w(p->es5503[ChipID], Offset, Data);
			break;
		case 0x25:	// ES5506
			if (Port & 0x80)
				es550x_w16(p->es550x[ChipID], Port & 0x7F, (Offset << 8) | (Data << 0));
			else
				es550x_w(p->es550x[ChipID], Port, Data);
			break;
		case 0x26:	// X1-010
			seta_sound_w(p->x1_010[ChipID], (Port << 8) | (Offset << 0), Data);
			break;
		case 0x27:	// C352
			c352_w(p->c352[ChipID], Port, (Offset << 8) | (Data << 0));
			break;
		case 0x28:	// GA20
			irem_ga20_w(p->ga20[ChipID], Offset, Data);
			break;
//		case 0x##:	// OKIM6376
//			break;
		}
	return;
}
