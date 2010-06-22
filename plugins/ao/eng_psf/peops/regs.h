/***************************************************************************
                           regs.h  -  description
                             -------------------
    begin                : Wed May 15 2002
    copyright            : (C) 2002 by Pete Bernert
    email                : BlackDove@addcom.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version. See also the license.txt file for *
 *   additional informations.                                              *
 *                                                                         *
 ***************************************************************************/

//*************************************************************************//
// History of changes:
//
// 2002/05/15 - Pete
// - generic cleanup for the Peops release
//
//*************************************************************************//


static void SoundOn(int start,int end,u16 val);
static void SoundOff(int start,int end,u16 val);
static void FModOn(int start,int end,u16 val);
static void NoiseOn(int start,int end,u16 val);
static void SetVolumeLR(int right, u8 ch,s16 vol);
static void SetPitch(int ch,u16 val);
void SPUwriteRegister(u32 reg, u16 val);
