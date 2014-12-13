////////////////////////////////////////////////////////////////////////////
//                           **** WAVPACK ****                            //
//                  Hybrid Lossless Wavefile Compressor                   //
//              Copyright (c) 1998 - 2006 Conifer Software.               //
//                          All Rights Reserved.                          //
//      Distributed under the BSD Software License (see license.txt)      //
////////////////////////////////////////////////////////////////////////////

This package contains all the source code required to build the WavPack
command-line programs and the WavPack library and it has been tested on many
platforms.

On Windows there are solution and project files for Visual Studio 2005 and
additional sourcecode to build the CoolEdit/Audition plugin and the winamp
plugin. The CoolEdit/Audition plugin provides a good example for using the
library to both read and write WavPack files.

To build everything on Linux, type:

1. ./configure [--enable-mmx]
2. make
3. make install (optionally, to install into /usr/local/bin)

If you are using the code directly from SVN (rather than a distribution)
then you will need to do a ./autogen.sh before the configure step. For
processors that support MMX, use the --enable-mmx switch to utilize MMX
intrinsics to speed up encoding of stereo 24-bit (and higher) audio.

Notes:

1. There are two documentation files contained in the distribution:

   doc/library_use.txt    contains a detailed description of the API provided
                          by WavPack library appropriate for read and writing
                          WavPack files

   doc/file_format.txt    contains a description of the WavPack file format,
                          including details needed for parsing WavPack blocks
                          and interpreting the block header and flags

   There is also a description of the WavPack algorithms in the forth edition
   of David Salomon's book "Data Compression: The Complete Reference". The
   section on WavPack can be found here:

   www.wavpack.com/WavPack.pdf

2. This code is designed to be easy to port to other platforms. File I/O is
   done with streams and all file functions (except "fopen") are handled in
   a wrapper in the "utils.c" module. The code is endian-independent.

   To maintain compatibility on various platforms, the following conventions
   are used: A "short" must be 16-bits and an "int" must be 32-bits.
   The "long" type is not used. The "char" type must be 8-bits, signed or
   unsigned.

3. For WavPack file decoding, a library interface in "wputils.c" provides all
   the functionality required for both the winamp plugin and the "wvunpack"
   command-line program (including the transparent decoding of "correction"
   files). There is also an alternate entry point that uses reader callbacks
   for all input, although in this case it is the caller's responsibility to
   to open the "correction" file. The header file "include/wavpack.h"
   includes everything needed while hiding all the WavPack internals from the
   application.

4. For WavPack file creation, the library interface in "wputils.c" provides
   all the functionality for both the Audition filter and the "wavpack"
   command-line program. No file I/O is performed by the library when creating
   WavPack files. Instead, the user supplies a "write_block" function that
   accepts completed WavPack blocks. It is also possible to append APEv2 tags
   to WavPack files during creation and edit APEv2 tags on existing files
   (although there is no support currently for "binary" fields in the tags).

5. The following #define's can be optionally used to eliminate some functionality
   to create smaller binaries. It is important that they must be specified
   the same for the compilation of ALL files:
   
   NO_UNPACK       no unpacking of audio samples from WavPack files
                    (also don't include unpack.c)
   NO_PACK         no creating WavPack files from raw audio data
                    (also don't include pack.c, extra1.c and extra2.c)
   INFO_ONLY       to obtain information from WavPack files, but not audio
                    (also don't include pack.c, extra1.c and extra2.c)
   NO_SEEKING      to not allow seeking to a specific sample index (unpack only)
   NO_USE_FSTREAMS to not open WavPack files by name using fstreams
   NO_TAGS         to not read specified fields from ID3v1 and APEv2 tags and
                    create APEv2 tags
   VER4_ONLY       to only handle WavPack files from versions 4.0 onward
   WIN32           required for Win32 platform

6. There are alternate versions of this library available specifically designed
   for "resource limited" CPUs or hardware encoding and decoding. There is the
   "tiny decoder" library which works with less than 32k of code and less than
   4k of data and has assembly language optimizations for the ARM and Freescale
   ColdFire CPUs.  The "tiny encoder" is also designed for embedded use and
   handles the pure lossless, lossy, and hybrid lossless modes. Neither of the
   "tiny" versions use any memory allocation functions nor do they require
   floating-point arithmetic support.

   For applications requiring very low latency, there is a special version of
   the library that supports a variation on the regular WavPack block format
   to facilitate this.

7. Questions or comments should be directed to david@wavpack.com
