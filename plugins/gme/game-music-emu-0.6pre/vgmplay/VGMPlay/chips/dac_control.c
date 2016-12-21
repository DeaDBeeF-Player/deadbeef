// TODO: SCSP and (especially) WonderSwan
 /************************
  *  DAC Stream Control  *
  ***********************/
// (Custom Driver to handle PCM Streams of YM2612 DAC and PWM.)
//
// Written on 3 February 2011 by Valley Bell
// Last Update: 04 October 2015
//
// Only for usage in non-commercial, VGM file related software.

/* How it basically works:

1. send command X with data Y at frequency F to chip C
2. do that until you receive a STOP command, or until you sent N commands

*/

#include <stdlib.h>

#include "mamedef.h"
#include "dac_control.h"

#include "../ChipMapper.h"

#define DAC_SMPL_RATE	chip->SampleRate

typedef struct _dac_control
{
	// Commands sent to dest-chip
	UINT8 DstChipType;
	UINT8 DstChipID;
	UINT16 DstCommand;
	UINT8 CmdSize;
	
	UINT32 Frequency;	// Frequency (Hz) at which the commands are sent
	UINT32 DataLen;		// to protect from reading beyond End Of Data
	const UINT8* Data;
	UINT32 DataStart;	// Position where to start
	UINT8 StepSize;		// usually 1, set to 2 for L/R interleaved data
	UINT8 StepBase;		// usually 0, set to 0/1 for L/R interleaved data
	UINT32 CmdsToSend;
	
	// Running Bits:	0 (01) - is playing
	//					2 (04) - loop sample (simple loop from start to end)
	//					4 (10) - already sent this command
	//					7 (80) - disabled
	UINT8 Running;
	UINT8 Reverse;
	UINT32 Step;		// Position in Player SampleRate
	UINT32 Pos;			// Position in Data SampleRate
	UINT32 RemainCmds;
	UINT32 RealPos;		// true Position in Data (== Pos, if Reverse is off)
	UINT8 DataStep;		// always StepSize * CmdSize

	void* param;
	UINT32 SampleRate;
} dac_control;

#ifndef NULL
#define NULL	(void*)0
#endif

INLINE void daccontrol_SendCommand(dac_control *chip)
{
	UINT8 Port;
	UINT8 Command;
	UINT8 Data;
	const UINT8* ChipData;
	
	if (chip->Running & 0x10)	// command already sent
		return;
	if (chip->DataStart + chip->RealPos >= chip->DataLen)
		return;
	
	//if (! chip->Reverse)
		ChipData = chip->Data + (chip->DataStart + chip->RealPos);
	//else
	//	ChipData = chip->Data + (chip->DataStart + chip->CmdsToSend - 1 - chip->Pos);
	switch(chip->DstChipType)
	{
	// Support for the important chips
	case 0x02:	// YM2612 (16-bit Register (actually 9 Bit), 8-bit Data)
		Port = (chip->DstCommand & 0xFF00) >> 8;
		Command = (chip->DstCommand & 0x00FF) >> 0;
		Data = ChipData[0x00];
		chip_reg_write(chip->param, chip->DstChipType, chip->DstChipID, Port, Command, Data);
		break;
	case 0x11:	// PWM (4-bit Register, 12-bit Data)
		Port = (chip->DstCommand & 0x000F) >> 0;
		Command = ChipData[0x01] & 0x0F;
		Data = ChipData[0x00];
		chip_reg_write(chip->param, chip->DstChipType, chip->DstChipID, Port, Command, Data);
		break;
	// Support for other chips (mainly for completeness)
	case 0x00:	// SN76496 (4-bit Register, 4-bit/10-bit Data)
		Command = (chip->DstCommand & 0x00F0) >> 0;
		Data = ChipData[0x00] & 0x0F;
		if (Command & 0x10)
		{
			// Volume Change (4-Bit value)
			chip_reg_write(chip->param, chip->DstChipType, chip->DstChipID, 0x00, 0x00, Command | Data);
		}
		else
		{
			// Frequency Write (10-Bit value)
			Port = ((ChipData[0x01] & 0x03) << 4) | ((ChipData[0x00] & 0xF0) >> 4);
			chip_reg_write(chip->param, chip->DstChipType, chip->DstChipID, 0x00, 0x00, Command | Data);
			chip_reg_write(chip->param, chip->DstChipType, chip->DstChipID, 0x00, 0x00, Port);
		}
		break;
	case 0x18:	// OKIM6295 - TODO: verify
		Command = (chip->DstCommand & 0x00FF) >> 0;
		Data = ChipData[0x00];
		
		if (! Command)
		{
			Port = (chip->DstCommand & 0x0F00) >> 8;
			if (Data & 0x80)
			{
				// Sample Start
				// write sample ID
				chip_reg_write(chip->param, chip->DstChipType, chip->DstChipID, 0x00, Command, Data);
				// write channel(s) that should play the sample
				chip_reg_write(chip->param, chip->DstChipType, chip->DstChipID, 0x00, Command, Port << 4);
			}
			else
			{
				// Sample Stop
				chip_reg_write(chip->param, chip->DstChipType, chip->DstChipID, 0x00, Command, Port << 3);
			}
		}
		else
		{
			chip_reg_write(chip->param, chip->DstChipType, chip->DstChipID, 0x00, Command, Data);
		}
		break;
		// Generic support: 8-bit Register, 8-bit Data
	case 0x01:	// YM2413
	case 0x03:	// YM2151
	case 0x06:	// YM2203
	case 0x09:	// YM3812
	case 0x0A:	// YM3526
	case 0x0B:	// Y8950
	case 0x0F:	// YMZ280B
	case 0x12:	// AY8910
	case 0x13:	// GameBoy DMG
	case 0x14:	// NES APU
//	case 0x15:	// MultiPCM
	case 0x16:	// UPD7759
	case 0x17:	// OKIM6258
	case 0x1D:	// K053260 - TODO: Verify
	case 0x1E:	// Pokey - TODO: Verify
		Command = (chip->DstCommand & 0x00FF) >> 0;
		Data = ChipData[0x00];
		chip_reg_write(chip->param, chip->DstChipType, chip->DstChipID, 0x00, Command, Data);
		break;
		// Generic support: 16-bit Register, 8-bit Data
	case 0x07:	// YM2608
	case 0x08:	// YM2610/B
	case 0x0C:	// YMF262
	case 0x0D:	// YMF278B
	case 0x0E:	// YMF271
	case 0x19:	// K051649 - TODO: Verify
	case 0x1A:	// K054539 - TODO: Verify
	case 0x1C:	// C140 - TODO: Verify
		Port = (chip->DstCommand & 0xFF00) >> 8;
		Command = (chip->DstCommand & 0x00FF) >> 0;
		Data = ChipData[0x00];
		chip_reg_write(chip->param, chip->DstChipType, chip->DstChipID, Port, Command, Data);
		break;
		// Generic support: 8-bit Register with Channel Select, 8-bit Data
	case 0x05:	// RF5C68
	case 0x10:	// RF5C164
	case 0x1B:	// HuC6280
		Port = (chip->DstCommand & 0xFF00) >> 8;
		Command = (chip->DstCommand & 0x00FF) >> 0;
		Data = ChipData[0x00];
		
		if (Port == 0xFF)
		{
			chip_reg_write(chip->param, chip->DstChipType, chip->DstChipID, 0x00, Command & 0x0F, Data);
		}
		else
		{
			UINT8 prevChn;
			
			prevChn = Port;	// by default don't restore channel
			// get current channel for supported chips
			if (chip->DstChipType == 0x05)
				;	// TODO
			else if (chip->DstChipType == 0x05)
				;	// TODO
			else if (chip->DstChipType == 0x1B)
				prevChn = chip_reg_read(chip->param, 0x1B, chip->DstChipID, 0x00, 0x00);
			
			// Send Channel Select
			chip_reg_write(chip->param, chip->DstChipType, chip->DstChipID, 0x00, Command >> 4, Port);
			// Send Data
			chip_reg_write(chip->param, chip->DstChipType, chip->DstChipID, 0x00, Command & 0x0F, Data);
			// restore old channel
			if (prevChn != Port)
				chip_reg_write(chip->param, chip->DstChipType, chip->DstChipID, 0x00, Command >> 4, prevChn);
		}
		break;
		// Generic support: 8-bit Register, 16-bit Data
	case 0x1F:	// QSound
		Command = (chip->DstCommand & 0x00FF) >> 0;
		chip_reg_write(chip->param, chip->DstChipType, chip->DstChipID, ChipData[0x00], ChipData[0x01], Command);
		break;
	}
	chip->Running |= 0x10;
	
	return;
}

INLINE UINT32 muldiv64round(UINT32 Multiplicand, UINT32 Multiplier, UINT32 Divisor)
{
	// Yes, I'm correctly rounding the values.
	return (UINT32)(((UINT64)Multiplicand * Multiplier + Divisor / 2) / Divisor);
}

void daccontrol_update(void *_info, UINT32 samples)
{
	dac_control *chip = (dac_control *)_info;
	UINT32 NewPos;
	INT16 RealDataStp;
	
	if (chip->Running & 0x80)	// disabled
		return;
	if (! (chip->Running & 0x01))	// stopped
		return;
	
	if (! chip->Reverse)
		RealDataStp = chip->DataStep;
	else
		RealDataStp = -chip->DataStep;
	
	if (samples > 0x20)
	{
		// very effective Speed Hack for fast seeking
		NewPos = chip->Step + (samples - 0x10);
		NewPos = muldiv64round(NewPos * chip->DataStep, chip->Frequency, DAC_SMPL_RATE);
		while(chip->RemainCmds && chip->Pos < NewPos)
		{
			chip->Pos += chip->DataStep;
			chip->RealPos += RealDataStp;
			chip->RemainCmds --;
		}
	}
	
	chip->Step += samples;
	// Formula: Step * Freq / SampleRate
	NewPos = muldiv64round(chip->Step * chip->DataStep, chip->Frequency, DAC_SMPL_RATE);
	daccontrol_SendCommand(chip);
	
	while(chip->RemainCmds && chip->Pos < NewPos)
	{
		daccontrol_SendCommand(chip);
		chip->Pos += chip->DataStep;
		chip->RealPos += RealDataStp;
		chip->Running &= ~0x10;
		chip->RemainCmds --;
	}
	
	if (! chip->RemainCmds && (chip->Running & 0x04))
	{
		// loop back to start
		chip->RemainCmds = chip->CmdsToSend;
		chip->Step = 0x00;
		chip->Pos = 0x00;
		if (! chip->Reverse)
			chip->RealPos = 0x00;
		else
			chip->RealPos = (chip->CmdsToSend - 0x01) * chip->DataStep;
	}
	
	if (! chip->RemainCmds)
		chip->Running &= ~0x01;	// stop
	
	return;
}

UINT8 device_start_daccontrol(void **_info, void *param, int SampleRate)
{
	dac_control *chip;
	
	chip = (dac_control *) calloc(1, sizeof(dac_control));
	*_info = (void *) chip;

	chip->param = param;
	chip->SampleRate = SampleRate;
	
	chip->DstChipType = 0xFF;
	chip->DstChipID = 0x00;
	chip->DstCommand = 0x0000;
	
	chip->Running = 0xFF;	// disable all actions (except setup_chip)
	
	return 1;
}

void device_stop_daccontrol(void *_info)
{
	dac_control *chip = (dac_control *)_info;
	
	chip->Running = 0xFF;

	free(chip);
	
	return;
}

void device_reset_daccontrol(void *_info)
{
	dac_control *chip = (dac_control *)_info;
	
	chip->DstChipType = 0x00;
	chip->DstChipID = 0x00;
	chip->DstCommand = 0x00;
	chip->CmdSize = 0x00;
	
	chip->Frequency = 0;
	chip->DataLen = 0x00;
	chip->Data = NULL;
	chip->DataStart = 0x00;
	chip->StepSize = 0x00;
	chip->StepBase = 0x00;
	
	chip->Running = 0x00;
	chip->Reverse = 0x00;
	chip->Step = 0x00;
	chip->Pos = 0x00;
	chip->RealPos = 0x00;
	chip->RemainCmds = 0x00;
	chip->DataStep = 0x00;
	
	return;
}

void daccontrol_setup_chip(void *_info, UINT8 ChType, UINT8 ChNum, UINT16 Command)
{
	dac_control *chip = (dac_control *)_info;
	
	chip->DstChipType = ChType;	// TypeID (e.g. 0x02 for YM2612)
	chip->DstChipID = ChNum;	// chip number (to send commands to 1st or 2nd chip)
	chip->DstCommand = Command;	// Port and Command (would be 0x02A for YM2612)
	
	switch(chip->DstChipType)
	{
	case 0x00:	// SN76496
		if (chip->DstCommand & 0x0010)
			chip->CmdSize = 0x01;	// Volume Write
		else
			chip->CmdSize = 0x02;	// Frequency Write
		break;
	case 0x02:	// YM2612
		chip->CmdSize = 0x01;
		break;
	case 0x11:	// PWM
	case 0x1F:	// QSound
		chip->CmdSize = 0x02;
		break;
	default:
		chip->CmdSize = 0x01;
		break;
	}
	chip->DataStep = chip->CmdSize * chip->StepSize;
	
	return;
}

void daccontrol_set_data(void *_info, UINT8* Data, UINT32 DataLen, UINT8 StepSize, UINT8 StepBase)
{
	dac_control *chip = (dac_control *)_info;
	
	if (chip->Running & 0x80)
		return;
	
	if (DataLen && Data != NULL)
	{
		chip->DataLen = DataLen;
		chip->Data = Data;
	}
	else
	{
		chip->DataLen = 0x00;
		chip->Data = NULL;
	}
	chip->StepSize = StepSize ? StepSize : 1;
	chip->StepBase = StepBase;
	chip->DataStep = chip->CmdSize * chip->StepSize;
	
	return;
}

void daccontrol_refresh_data(void *_info, UINT8* Data, UINT32 DataLen)
{
	// Should be called to fix the data pointer. (e.g. after a realloc)
	dac_control *chip = (dac_control *)_info;
	
	if (chip->Running & 0x80)
		return;
	
	if (DataLen && Data != NULL)
	{
		chip->DataLen = DataLen;
		chip->Data = Data;
	}
	else
	{
		chip->DataLen = 0x00;
		chip->Data = NULL;
	}
	
	return;
}

void daccontrol_set_frequency(void *_info, UINT32 Frequency)
{
	dac_control *chip = (dac_control *)_info;
	
	if (chip->Running & 0x80)
		return;
	
	chip->Frequency = Frequency;
	
	return;
}

void daccontrol_start(void *_info, UINT32 DataPos, UINT8 LenMode, UINT32 Length)
{
	dac_control *chip = (dac_control *)_info;
	UINT16 CmdStepBase;
	
	if (chip->Running & 0x80)
		return;
	
	CmdStepBase = chip->CmdSize * chip->StepBase;
	if (DataPos != 0xFFFFFFFF)	// skip setting DataStart, if Pos == -1
	{
		chip->DataStart = DataPos + CmdStepBase;
		if (chip->DataStart > chip->DataLen)	// catch bad value and force silence
			chip->DataStart = chip->DataLen;
	}
	
	switch(LenMode & 0x0F)
	{
	case DCTRL_LMODE_IGNORE:	// Length is already set - ignore
		break;
	case DCTRL_LMODE_CMDS:		// Length = number of commands
		chip->CmdsToSend = Length;
		break;
	case DCTRL_LMODE_MSEC:		// Length = time in msec
		chip->CmdsToSend = 1000 * Length / chip->Frequency;
		break;
	case DCTRL_LMODE_TOEND:		// play unti stop-command is received (or data-end is reached)
		chip->CmdsToSend = (chip->DataLen - (chip->DataStart - CmdStepBase)) / chip->DataStep;
		break;
	case DCTRL_LMODE_BYTES:		// raw byte count
		chip->CmdsToSend = Length / chip->DataStep;
		break;
	default:
		chip->CmdsToSend = 0x00;
		break;
	}
	chip->Reverse = (LenMode & 0x10) >> 4;
	
	chip->RemainCmds = chip->CmdsToSend;
	chip->Step = 0x00;
	chip->Pos = 0x00;
	if (! chip->Reverse)
		chip->RealPos = 0x00;
	else
		chip->RealPos = (chip->CmdsToSend - 0x01) * chip->DataStep;
	
	chip->Running &= ~0x04;
	chip->Running |= (LenMode & 0x80) ? 0x04 : 0x00;	// set loop mode
	
	chip->Running |= 0x01;	// start
	chip->Running &= ~0x10;	// command isn't yet sent
	
	return;
}

void daccontrol_stop(void *_info)
{
	dac_control *chip = (dac_control *)_info;
	
	if (chip->Running & 0x80)
		return;
	
	chip->Running &= ~0x01;	// stop
	
	return;
}
