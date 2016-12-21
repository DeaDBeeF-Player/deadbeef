
/* -----------------------------------------------------------------------------------------------------------
Software License for The Fraunhofer FDK AAC Codec Library for Android

© Copyright  1995 - 2013 Fraunhofer-Gesellschaft zur Förderung der angewandten Forschung e.V.
  All rights reserved.

 1.    INTRODUCTION
The Fraunhofer FDK AAC Codec Library for Android ("FDK AAC Codec") is software that implements
the MPEG Advanced Audio Coding ("AAC") encoding and decoding scheme for digital audio.
This FDK AAC Codec software is intended to be used on a wide variety of Android devices.

AAC's HE-AAC and HE-AAC v2 versions are regarded as today's most efficient general perceptual
audio codecs. AAC-ELD is considered the best-performing full-bandwidth communications codec by
independent studies and is widely deployed. AAC has been standardized by ISO and IEC as part
of the MPEG specifications.

Patent licenses for necessary patent claims for the FDK AAC Codec (including those of Fraunhofer)
may be obtained through Via Licensing (www.vialicensing.com) or through the respective patent owners
individually for the purpose of encoding or decoding bit streams in products that are compliant with
the ISO/IEC MPEG audio standards. Please note that most manufacturers of Android devices already license
these patent claims through Via Licensing or directly from the patent owners, and therefore FDK AAC Codec
software may already be covered under those patent licenses when it is used for those licensed purposes only.

Commercially-licensed AAC software libraries, including floating-point versions with enhanced sound quality,
are also available from Fraunhofer. Users are encouraged to check the Fraunhofer website for additional
applications information and documentation.

2.    COPYRIGHT LICENSE

Redistribution and use in source and binary forms, with or without modification, are permitted without
payment of copyright license fees provided that you satisfy the following conditions:

You must retain the complete text of this software license in redistributions of the FDK AAC Codec or
your modifications thereto in source code form.

You must retain the complete text of this software license in the documentation and/or other materials
provided with redistributions of the FDK AAC Codec or your modifications thereto in binary form.
You must make available free of charge copies of the complete source code of the FDK AAC Codec and your
modifications thereto to recipients of copies in binary form.

The name of Fraunhofer may not be used to endorse or promote products derived from this library without
prior written permission.

You may not charge copyright license fees for anyone to use, copy or distribute the FDK AAC Codec
software or your modifications thereto.

Your modified versions of the FDK AAC Codec must carry prominent notices stating that you changed the software
and the date of any change. For modified versions of the FDK AAC Codec, the term
"Fraunhofer FDK AAC Codec Library for Android" must be replaced by the term
"Third-Party Modified Version of the Fraunhofer FDK AAC Codec Library for Android."

3.    NO PATENT LICENSE

NO EXPRESS OR IMPLIED LICENSES TO ANY PATENT CLAIMS, including without limitation the patents of Fraunhofer,
ARE GRANTED BY THIS SOFTWARE LICENSE. Fraunhofer provides no warranty of patent non-infringement with
respect to this software.

You may use this FDK AAC Codec software or modifications thereto only for purposes that are authorized
by appropriate patent licenses.

4.    DISCLAIMER

This FDK AAC Codec software is provided by Fraunhofer on behalf of the copyright holders and contributors
"AS IS" and WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES, including but not limited to the implied warranties
of merchantability and fitness for a particular purpose. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
CONTRIBUTORS BE LIABLE for any direct, indirect, incidental, special, exemplary, or consequential damages,
including but not limited to procurement of substitute goods or services; loss of use, data, or profits,
or business interruption, however caused and on any theory of liability, whether in contract, strict
liability, or tort (including negligence), arising in any way out of the use of this software, even if
advised of the possibility of such damage.

5.    CONTACT INFORMATION

Fraunhofer Institute for Integrated Circuits IIS
Attention: Audio and Multimedia Departments - FDK AAC LL
Am Wolfsmantel 33
91058 Erlangen, Germany

www.iis.fraunhofer.de/amm
amm-info@iis.fraunhofer.de
----------------------------------------------------------------------------------------------------------- */

/**************************  Fraunhofer IIS FDK SysLib  **********************

   Author(s):   Manuel Jander
   Description:

******************************************************************************/

#define RESOURCE_scratchBuffer
#define FUNCTION_FDKprolog
#define FUNCTION_FDKepilog

#define MIPS_VIRTUAL_START  (0x80000000)
/* value below is defined in simulator config (MipsMemIntf-{24KE,4KE}.cfg) */
#define MIPS_SDE_SCRATCHPAD (0x00058000)

//#define MIPS_SRAM_SIZE  (32768)
#define MIPS_SRAM_SIZE  (4096)

#define MIPS_SCRATCH_SIZE (4096)
#define DATA_L1_A_SIZE (MIPS_SRAM_SIZE-MIPS_SCRATCH_SIZE)




#ifdef RESOURCE_scratchBuffer
#define FDK_SCRATCHBUF_SIZE 1024
static LONG *___scratchBuffer = NULL;
static LONG *__scratchBuffer = NULL;
static unsigned char *__pScratchBuffer = NULL;
#endif


#ifdef __linux__

#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

static int fd;
static inline void * getSram(void)
{
  unsigned long *ptr = NULL;

  /* Open driver */
  fd = open("/dev/sram", 0);
  if (fd < 0)
  {
    printf("Unable to access sram. Fallback to malloc\n");
    /* Signal "no sram driver at use". */
    fd = -1;
    /* Return malloced pointer (fallback) */
    return malloc(MIPS_SRAM_SIZE);
  }

  /* Get memory mapped into CPU (virtual) address space */
  ptr = (unsigned long *)mmap(NULL, MIPS_SRAM_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
  if(ptr == MAP_FAILED)
  {
     printf("Unable to access sram. Fallback to malloc\n");
     /* Give up on the sram driver */
     close(fd);
     /* Signal "no sram driver at use". */
     fd = -1;
     /* Return malloced pointer (fallback) */
     ptr = (unsigned long *)malloc(MIPS_SRAM_SIZE);
  }


  /* Return pointer to sram */
  return (void*)ptr;
}

static inline void freeSram(void* ptr)
{
  /* Check if sram driver is being used. */
  if (fd == -1)
  {
    free(ptr);
    return;
  }

  /* Unmap memory */
  munmap(ptr, MIPS_SRAM_SIZE);
  /* Close driver */
  close(fd);

  return;
}

#elif defined(__SDE_MIPS__)

#include <stdio.h>
#include <mips/cpu.h>

static int hasISPRAM, hasDSPRAM;

static inline void * getSram(void)
{
  void *addr;
  unsigned int Config;

  Config = mips_getconfig();
  hasISPRAM = (Config >> 24) & 1;
  hasDSPRAM = (Config >> 23) & 1;

  FDKprintf("Config ISP/DSP: %d/%d\n", hasISPRAM, hasDSPRAM);

  if (hasDSPRAM) {
    long paddr, laddr;

    FDKprintf("wrong\n");
    paddr = MIPS_SDE_SCRATCHPAD;
    /* Fixed mapping of kseg0: 0x80000000-0x9fffffff virtual => 0x00000000-0x1fffffff physical */
    laddr = MIPS_VIRTUAL_START + MIPS_SDE_SCRATCHPAD;
    addr = (void*)(laddr);
  } else {
    FDKprintf("ok\n");
    addr = malloc(MIPS_SRAM_SIZE);
    FDKprintf("addr %d\n", (int)addr);
  }
  return addr;
}
static inline void freeSram(void* ptr)
{
  if (!hasDSPRAM) {
    free(ptr);
  }
}

#else

static inline void * getSram(void)
{
  return malloc(MIPS_SRAM_SIZE);
}
static inline void freeSram(void* ptr)
{
  free(ptr);
}

#endif


#ifdef FUNCTION_FDKprolog
void FDKprolog(void)
{
   unsigned char *addr;

#ifdef _MIPS_ARCH_MIPS32R2
   unsigned status;
   asm volatile("mfc0 %0, $12, 0;\n" : "=r" (status));
   status |= (1 << 24);
   asm volatile("mtc0 %0, $12, 0;\n" :: "r" (status));
#endif

   addr = (unsigned char*)getSram();
   if (addr == NULL) {
     FDKprintfErr("SRAM allocation failed ! This is fatal.\n");
     exit(-1);
   } else {
     FDKprintf("SRAM @ 0x%08x, size = 0x%x\n", (unsigned int) addr, MIPS_SRAM_SIZE);
   }


#ifdef RESOURCE_scratchBuffer
   ___scratchBuffer = (LONG*)(addr + MIPS_SRAM_SIZE - MIPS_SCRATCH_SIZE);
#endif

   atexit(FDKepilog);

   FDKprolog_generic();
}
#endif

#ifdef FUNCTION_FDKepilog
void FDKepilog(void)
{

#ifdef _MIPS_ARCH_MIPS32R2
   unsigned status;
   asm volatile("mfc0 %0, $12, 0;\n" : "=r" (status));
           status &= ~(1 << 24);
   asm volatile("mtc0 %0, $12, 0;\n" :: "r" (status));
#endif

   FDKepilog_generic();
}
#endif


#if !defined(__linux__)

#define FUNCTION_FDKclock

#ifndef MIPS_CPU_CLK
#define MIPS_CPU_CLK 100000000
#endif

INT FDKclock(void) {
  INT clk;

  asm volatile ("mfc0 %0,$9 " : "=r" (clk));
  return clk;
}

#endif /* !defined(__linux__) */
