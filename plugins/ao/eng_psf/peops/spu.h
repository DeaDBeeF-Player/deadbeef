/***************************************************************************
                            spu.h  -  description
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

void sexyd_update(unsigned char* pSound,long lBytes);

int SPUasync(u32 cycles);
void SPU_flushboot(void);
int SPUinit(void);
int SPUopen(void);
int SPUclose(void);
int SPUshutdown(void);
void SPUinjectRAMImage(u16 *pIncoming);
void SPUreadDMAMem(u32 usPSXMem,int iSize);
void SPUwriteDMAMem(u32 usPSXMem,int iSize);
u16 SPUreadRegister(u32 reg);

