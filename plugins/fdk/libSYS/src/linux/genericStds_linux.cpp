
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
   Description: Linux genericStds (mostly kernel SRAM driver bindings)

******************************************************************************/


/*
 * NOTE: it makes only sense to enable this if you also have the corresponding
 * GNU/Linux kernel driver to access fast SRAM.
 */
#if defined(__arm__) /* || defined(__mips__) */

/**
 * EABI static linking problem workaround
 *
 * These function are normally present in libc.a but
 * apparently can be linked only statically.
 * While using C++ (iisisoff) that is a problem,
 * because it wont work (static global constructors
 * cause problems with static linked programs).
 * So the workaround is to include those functions here,
 * because libSYS.a is linked statically, and libc can be
 * linked dynamically as usual.
 *
 * Add more EABI functions here if you get unresolved
 * symbols of EABI functions.
 */
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif
void __aeabi_memcpy(void *dest, void *src, int size)
{
  memcpy(dest, src, size);
}
void __aeabi_memcpy4(void *dest, void *src, int size)
{
  memcpy(dest, src, size);
}
void __aeabi_memmove4(void *dest, void *src, int size)
{
  memmove(dest, src, size);
}
void __aeabi_memclr(void *ptr, int size)
{
  memset(ptr, 0, size);
}
void __aeabi_memclr4(void *ptr, int size)
{
  memset(ptr, 0, size);
}
#ifdef __cplusplus
}
#endif

/* Include Linux kernel config, or set ARCH and processor macros directly */
/*
#define CONFIG_ARCH_MXC
#define CONFIG_ARCH_MX25
*/

#if defined(CONFIG_ARCH_OMAP3)
#define KERNEL_SRAM_SIZE       65536
#elif defined(CONFIG_ARCH_MX31)
#define KERNEL_SRAM_SIZE       16384
#elif defined(CONFIG_ARCH_MX25)
#define KERNEL_SRAM_SIZE      131072
#elif defined(CONFIG_ARCH_MX35)
#define KERNEL_SRAM_SIZE      131072
#else
#define KERNEL_SRAM_SIZE 0
#endif

#if (KERNEL_SRAM_SIZE > 0)
#define KERNEL_SCRATCH_SIZE (4096)
#define FDK_SCRATCHBUF_SIZE (KERNEL_SCRATCH_SIZE/sizeof(INT))
#define DATA_L1_A_SIZE (KERNEL_SRAM_SIZE-KERNEL_SCRATCH_SIZE)

#define RESOURCE_scratchBuffer
#define FUNCTION_FDKprolog
#define FUNCTION_FDKepilog

static unsigned char *L1_DATA_A=NULL;
static unsigned char *_a=NULL;


#ifdef RESOURCE_scratchBuffer
static INT *__scratchBuffer;
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
    printf("Unable to open /dev/sram. Fallback to malloc\n");
    /* Signal "no sram driver at use". */
    fd = -1;
    /* Return malloced pointer (fallback) */
    return FDKaalloc(KERNEL_SRAM_SIZE, 8);
  }

  /* Get memory mapped into CPU (virtual) address space */
  ptr = (unsigned long *)mmap(NULL, KERNEL_SRAM_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
  if(ptr == MAP_FAILED)
  {
     printf("Unable to mmap(). Fallback to malloc\n");
     /* Give up on the sram driver */
     close(fd);
     /* Signal "no sram driver at use". */
     fd = -1;
     /* Return malloced pointer (fallback) */
     ptr = (unsigned long *)FDKaalloc(KERNEL_SRAM_SIZE, 8);
  }


  /* Return pointer to sram */
  return (void*)ptr;
}

static inline void freeSram(void* ptr)
{
  /* Check if sram driver is being used. */
  if (fd == -1)
  {
    FDKafree(ptr);
    return;
  }

  /* Unmap memory */
  munmap(ptr, KERNEL_SRAM_SIZE);
  /* Close driver */
  close(fd);

  return;
}

#else

static inline void * getSram(void)
{
  return FDKaalloc(KERNEL_SRAM_SIZE, 8);
}
static inline void * freeSram(void* ptr)
{
  FDKafree(ptr);
}

#endif


#ifdef FUNCTION_FDKprolog
void FDKprolog(void)
{
   unsigned char *addr = (unsigned char*)getSram();


   if (addr == NULL)
   {
     printf("SRAM allocation failed ! This is fatal.\n");
     exit(-1);
   }

#ifdef RESOURCE_scratchBuffer
   __scratchBuffer = (INT*) ( addr + (KERNEL_SRAM_SIZE-KERNEL_SCRATCH_SIZE) );
   __pScratchBuffer = addr + (KERNEL_SRAM_SIZE);
#endif

   printf("SRAM @ 0x%08x\n", (unsigned int) addr);
   atexit(FDKepilog);

   FDKprolog_generic();
}
#endif

#ifdef FUNCTION_FDKepilog
void FDKepilog(void)
{
   /* Because of atexit(), make sure to call this only once */
   if (L1_DATA_A != NULL)
   {
     freeSram(L1_DATA_A);
     L1_DATA_A = NULL;

     FDKepilog_generic();
   }
}
#endif

#endif /* KERNEL_SRAM > 0 */

#endif /* ifdef __arm__ */

