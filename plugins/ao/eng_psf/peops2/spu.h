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
// 2004/04/04 - Pete
// - changed plugin to emulate PS2 spu
//
// 2002/05/15 - Pete
// - generic cleanup for the Peops release
//
//*************************************************************************//

#include "../psx.h"


//void SetupTimer(mips_cpu_context *cpu);
//void RemoveTimer(mips_cpu_context *cpu);
//EXPORT_GCC void CALLBACK SPU2playADPCMchannel(mips_cpu_context *cpu, xa_decode_t *xap);

EXPORT_GCC long CALLBACK SPU2init(mips_cpu_context *cpu, void (*callback)(unsigned char *, long, void *), void *data);
EXPORT_GCC long CALLBACK SPU2open(mips_cpu_context *cpu, void *pDsp);
EXPORT_GCC void CALLBACK SPU2async(mips_cpu_context *cpu, unsigned long cycle);
EXPORT_GCC void CALLBACK SPU2close(mips_cpu_context *cpu);

