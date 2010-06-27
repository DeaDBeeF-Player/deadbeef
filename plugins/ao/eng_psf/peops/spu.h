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

#include "externals.h"
#include "../psx.h"
void sexyd_update(unsigned char* pSound,long lBytes);

int SPUasync(mips_cpu_context *cpu, u32 cycles);
void SPU_flushboot(mips_cpu_context *cpu);
int SPUinit(mips_cpu_context *cpu, void (*update_cb)(unsigned char *pSound, long lBytes, void *data), void *data);
int SPUopen(mips_cpu_context *cpu);
int SPUclose(mips_cpu_context *cpu);
int SPUshutdown(mips_cpu_context *cpu);
void SPUinjectRAMImage(mips_cpu_context *cpu, u16 *pIncoming);
void SPUreadDMAMem(mips_cpu_context *cpu, u32 usPSXMem,int iSize);
void SPUwriteDMAMem(mips_cpu_context *cpu, u32 usPSXMem,int iSize);
u16 SPUreadRegister(mips_cpu_context *cpu, u32 reg);

