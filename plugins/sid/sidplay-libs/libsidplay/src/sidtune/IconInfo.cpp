/*
 * /home/ms/files/source/libsidtune/RCS/IconInfo.cpp,v
 *
 * Amiga icon tooltype PlaySID file format (.info) support.
 *
 * This is a derived work, courtesy of Peter Kunath, who has
 * provided an examplary source code to examine an Amiga icon file.
 * 
 * It has been ported and heavily modified to suit certain
 * requirements. This replaces the old code, which was simply
 * scanning input data for a first, presumedly constant, Id string.
 * This code does not require the default tool to serve as a
 * constant Id by containing "SID:PlaySID".
 * 
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include "SidTune.h"
#include "SmartPtr.h"
#include "SidTuneTools.h"
#include "sidendian.h"

#ifdef HAVE_EXCEPTIONS
#   include <new>
#endif
#include <string.h>

// Amiga Workbench specific structures.

struct Border
{
    uint8_t LeftEdge[2];          // uint_least16_t; initial offsets from the origin
    uint8_t TopEdge[2];           // uint_least16_t
    uint8_t FrontPen, BackPen;    // pens numbers for rendering
    uint8_t DrawMode;             // mode for rendering
    uint8_t Count;                // number of XY pairs
    uint8_t pXY[4];               // int_least16_t *XY; vector coordinate pairs rel to LeftTop
    uint8_t pNextBorder[4];       // Border *NextBorder; pointer to any other Border too
};

struct Image
{
    uint8_t LeftEdge[2];          // uint_least16_t; starting offset relative to some origin
    uint8_t TopEdge[2];           // uint_least16_t; starting offsets relative to some origin
    uint8_t Width[2];             // uint_least16_t; pixel size (though data is word-aligned)
    uint8_t Height[2];            // uint_least16_t
    uint8_t Depth[2];             // uint_least16_t; >= 0, for images you create
    uint8_t pImageData[4];        // uint_least16_t *ImageData; pointer to the actual word-aligned bits
    uint8_t PlanePick, PlaneOnOff;
    uint8_t pNextImage[4];        // Image *NextImage;
};

struct Gadget
{
    uint8_t pNextGadget[4];       // Gadget *NextGadget; next gadget in the list 
    uint8_t LeftEdge[2];          // uint_least16_t; "hit box" of gadget 
    uint8_t TopEdge[2];           // uint_least16_t
    uint8_t Width[2];             // uint_least16_t; "hit box" of gadget 
    uint8_t Height[2];            // uint_least16_t
    uint8_t Flags[2];             // uint_least16_t; see below for list of defines 
    uint8_t Activation[2];        // uint_least16_t
    uint8_t GadgetType[2];        // uint_least16_t; see below for defines 
    uint8_t pGadgetRender[4];     // Image *GadgetRender;
    uint8_t pSelectRender[4];     // Image *SelectRender;
    uint8_t pGadgetText[4];       // void *GadgetText;
    uint8_t MutualExclude[4];     // udword
    uint8_t pSpecialInfo[4];      // void *SpecialInfo;
    uint8_t GadgetID[2];          // uint_least16_t
    uint8_t UserData[4];          // udword; ptr to general purpose User data 
};

struct DiskObject
{
    uint8_t Magic[2];             // uint_least16_t; a magic num at the start of the file 
    uint8_t Version[2];           // uint_least16_t; a version number, so we can change it 
    struct Gadget Gadget;         // a copy of in core gadget 
    uint8_t Type;
    uint8_t PAD_BYTE;             // Pad it out to the next word boundry 
    uint8_t pDefaultTool[4];      // char *DefaultTool;
    uint8_t ppToolTypes[4];       // char **ToolTypes;
    uint8_t CurrentX[4];          // udword
    uint8_t CurrentY[4];          // udword
    uint8_t pDrawerData[4];       // char *DrawerData;
    uint8_t pToolWindow[4];       // char *ToolWindow; only applies to tools 
    uint8_t StackSize[4];         // udword; only applies to tools 
};


// A magic number, not easily impersonated.
#define WB_DISKMAGIC 0xE310
// Our current version number.
#define WB_DISKVERSION  1
// Our current revision number.
#define WB_DISKREVISION 1
// I only use the lower 8 bits of Gadget.UserData for the revision #.
#define WB_DISKREVISIONMASK 0xFF

// The Workbench object types.
#define    WB_DISK       1
#define    WB_DRAWER     2
#define    WB_TOOL       3
#define    WB_PROJECT    4
#define    WB_GARBAGE    5
#define    WB_DEVICE     6
#define    WB_KICK       7
#define    WB_APPICON    8

// --- Gadget.Flags values    ---
// Combinations in these bits describe the highlight technique to be used.
#define GFLG_GADGHIGHBITS 0x0003
// Complement the select box.
#define GFLG_GADGHCOMP    0x0000
// Draw a box around the image.
#define GFLG_GADGHBOX     0x0001
// Blast in this alternate image.
#define GFLG_GADGHIMAGE   0x0002
// Don't highlight.
#define GFLG_GADGHNONE    0x0003
// Set if GadgetRender and SelectRender point to an Image structure,
// clear if they point to Border structures.
#define GFLG_GADGIMAGE    0x0004

const char _sidtune_txt_format[] = "Raw plus PlaySID icon tooltype file (INFO)";

const char _sidtune_keyword_id[] = "SID:PLAYSID";
const char _sidtune_keyword_address[] = "ADDRESS=";
const char _sidtune_keyword_songs[] = "SONGS=";
const char _sidtune_keyword_speed[] = "SPEED=";
const char _sidtune_keyword_name[] = "NAME=";
const char _sidtune_keyword_author[] = "AUTHOR=";
const char _sidtune_keyword_copyright[] = "COPYRIGHT=";
const char _sidtune_keyword_musPlayer[] = "SIDSONG=YES";

const char _sidtune_txt_noMemError[] = "ERROR: Not enough free memory";
const char _sidtune_txt_corruptError[] = "ERROR: Info file is incomplete or corrupt";
const char _sidtune_txt_noStringsError[] = "ERROR: Info file does not contain required strings";
const char _sidtune_txt_dataCorruptError[] = "ERROR: C64 data file is corrupt";
#if defined(SIDTUNE_REJECT_UNKNOWN_FIELDS)
const char _sidtune_txt_chunkError[] = "ERROR: Invalid tooltype information in icon file";
#endif

const uint_least16_t safeBufferSize = 64;  // for string comparison, stream parsing


bool SidTune::INFO_fileSupport(const void* dataBuffer, uint_least32_t dataLength,
                               const void* infoBuffer, uint_least32_t infoLength)
{
    // Require a first minimum safety size.
    uint_least32_t minSize = 1+sizeof(struct DiskObject);
    if (infoLength < minSize)
        return( false );

    const DiskObject *dobject = (const DiskObject *)infoBuffer;

    // Require Magic_Id in the first two bytes of the file.
    if ( endian_16(dobject->Magic[0],dobject->Magic[1]) != WB_DISKMAGIC )
        return false;

    // Only version 1.x supported.
    if ( endian_16(dobject->Version[0],dobject->Version[1]) != WB_DISKVERSION )
        return false;

    // A PlaySID icon must be of type project.
    if ( dobject->Type != WB_PROJECT )
        return false;

    uint i;  // general purpose index variable

    // We want to skip a possible Gadget Image item.
    const char *icon = (const char*)infoBuffer + sizeof(DiskObject);

    if ( (endian_16(dobject->Gadget.Flags[0],dobject->Gadget.Flags[1]) & GFLG_GADGIMAGE) == 0)
    {
        // Calculate size of gadget borders (vector image).
        
        if (dobject->Gadget.pGadgetRender[0] |
            dobject->Gadget.pGadgetRender[1] |
            dobject->Gadget.pGadgetRender[2] |
            dobject->Gadget.pGadgetRender[3])  // border present?
        {
            // Require another minimum safety size.
            minSize += sizeof(struct Border);
            if (infoLength < minSize)
                return( false );

            const Border *brd = (const Border *)icon;
            icon += sizeof(Border);
            icon += brd->Count * (2+2);           // pair of uint_least16_t
        }

        if (dobject->Gadget.pSelectRender[0] |
            dobject->Gadget.pSelectRender[1] |
            dobject->Gadget.pSelectRender[2] |
            dobject->Gadget.pSelectRender[3])  // alternate border present?
        {
            // Require another minimum safety size.
            minSize += sizeof(Border);
            if (infoLength < minSize)
                return( false );

            const Border *brd = (const Border *)icon;
            icon += sizeof(Border);
            icon += brd->Count * (2+2);           // pair of uint_least16_t
        }
    }
    else
    {
        // Calculate size of gadget images (bitmap image).

        if (dobject->Gadget.pGadgetRender[0] |
            dobject->Gadget.pGadgetRender[1] |
            dobject->Gadget.pGadgetRender[2] |
            dobject->Gadget.pGadgetRender[3])  // image present?
        {
            // Require another minimum safety size.
            minSize += sizeof(Image);
            if (infoLength < minSize)
                return( false );

            const Image *img = (const Image *)icon;
            icon += sizeof(Image);

            uint_least32_t imgsize = 0;
            for(i=0;i<endian_16(img->Depth[0],img->Depth[1]);i++)
            {
                if ( (img->PlanePick & (1<<i)) != 0)
                {
                    // NOTE: Intuition relies on PlanePick to know how many planes
                    // of data are found in ImageData. There should be no more
                    // '1'-bits in PlanePick than there are planes in ImageData.
                    imgsize++;
                }
            }

            imgsize *= ((endian_16(img->Width[0],img->Width[1])+15)/16)*2;  // bytes per line
            imgsize *= endian_16(img->Height[0],img->Height[1]);            // bytes per plane

            icon += imgsize;
        }
      
        if (dobject->Gadget.pSelectRender[0] |
            dobject->Gadget.pSelectRender[1] |
            dobject->Gadget.pSelectRender[2] |
            dobject->Gadget.pSelectRender[3])  // alternate image present?
        {
            // Require another minimum safety size.
            minSize += sizeof(Image);
            if (infoLength < minSize)
                return( false );

            const Image *img = (const Image *)icon;
            icon += sizeof(Image);

            uint_least32_t imgsize = 0;
            for(i=0;i<endian_16(img->Depth[0],img->Depth[1]);i++)
            {
                if ( (img->PlanePick & (1<<i)) != 0)
                {
                    // NOTE: Intuition relies on PlanePick to know how many planes
                    // of data are found in ImageData. There should be no more
                    // '1'-bits in PlanePick than there are planes in ImageData.
                    imgsize++;
                }
            }

            imgsize *= ((endian_16(img->Width[0],img->Width[1])+15)/16)*2;  // bytes per line
            imgsize *= endian_16(img->Height[0],img->Height[1]);            // bytes per plane
            icon += imgsize;
        }
    }

    // Here use a smart pointer to prevent access violation errors.
    SmartPtr_sidtt<const char> spTool((const char*)icon,infoLength-(uint_least32_t)(icon-(const char*)infoBuffer));
    if ( !spTool )
    {
        info.formatString = _sidtune_txt_corruptError;
        return false;
    }

    // A separate safe buffer is used for each tooltype string.
#ifdef HAVE_EXCEPTIONS
    SmartPtr_sidtt<char> spCmpBuf(new(std::nothrow) char[safeBufferSize],safeBufferSize,true);
#else
    SmartPtr_sidtt<char> spCmpBuf(new char[safeBufferSize],safeBufferSize,true);
#endif
    if ( !spCmpBuf )
    {
        info.formatString = _sidtune_txt_noMemError;
        return false;
    }

#ifndef SID_HAVE_BAD_COMPILER
    char* cmpBuf = spCmpBuf.tellBegin();
#else
    // This should not be necessary, but for some reason
    // Microsoft Visual C++ says spCmpBuf is const...
    char* cmpBuf = (char*) spCmpBuf.tellBegin();
#endif

    // Skip default tool.
    spTool += endian_32(spTool[0],spTool[1],spTool[2],spTool[3]) + 4;

    // Defaults.
    fileOffset = 0;                   // no header in separate data file
    info.sidChipBase1 = 0xd400;
    info.sidChipBase2 = 0;
    info.musPlayer = false;
    info.numberOfInfoStrings = 0;
    uint_least32_t oldStyleSpeed = 0;

    // Flags for required entries.
    bool hasAddress = false,
    hasName = false,
    hasAuthor = false,
    hasCopyright = false,
    hasSongs = false,
    hasSpeed = false,
    hasUnknownChunk = false;

    // Calculate number of tooltype strings.
    i = (endian_32(spTool[0],spTool[1],spTool[2],spTool[3])/4) - 1;
    spTool += 4;  // skip size info

    while( i-- > 0 )
    {
        // Get length of this tool.
        uint_least32_t toolLen = endian_32(spTool[0],spTool[1],spTool[2],spTool[3]);
        spTool += 4;  // skip tool length
        // Copy item to safe buffer.
        for ( uint ci = 0; ci < toolLen; ci++ )
        {
#ifndef SID_HAVE_BAD_COMPILER
            spCmpBuf[ci] = spTool[ci];
#else
            // This should not be necessary, but for some reason
            // Microsoft Visual C++ says spCmpBuf is const...
            (*((char*) (&spCmpBuf[ci]))) = (char) spTool[ci];
#endif
        }
        if ( !(spTool&&spCmpBuf) )
        {
            return false;
        }

        // Now check all possible keywords.
        if ( SidTuneTools::myStrNcaseCmp(cmpBuf,_sidtune_keyword_address) == 0 )
        {
            const char *addrIn= cmpBuf + strlen(_sidtune_keyword_address);
            int len = toolLen - strlen(_sidtune_keyword_address);
            int pos = 0;
            info.loadAddr = (uint_least16_t)SidTuneTools::readHex( addrIn, len, pos );
            info.initAddr = (uint_least16_t)SidTuneTools::readHex( addrIn, len, pos );
            info.playAddr = (uint_least16_t)SidTuneTools::readHex( addrIn, len, pos );
            if ( pos >= len )
            {
                return false;
            }
            hasAddress = true;
        }
        else if ( SidTuneTools::myStrNcaseCmp(cmpBuf,_sidtune_keyword_songs) == 0 )
        {
            const char *numIn = cmpBuf + strlen(_sidtune_keyword_songs);
            int len = toolLen - strlen(_sidtune_keyword_songs);
            int pos = 0;
            if ( !pos >= len )
            {
                return false;
            }
            info.songs = (uint_least16_t)SidTuneTools::readDec( numIn, len, pos );
            info.startSong = (uint_least16_t)SidTuneTools::readDec( numIn, len, pos );
            hasSongs = true;
        }
        else if ( SidTuneTools::myStrNcaseCmp(cmpBuf,_sidtune_keyword_speed) == 0 )
        {
            const char *speedIn = cmpBuf + strlen(_sidtune_keyword_speed);
            int len = toolLen - strlen(_sidtune_keyword_speed);
            int pos = 0;
            if ( pos >= len )
            {
                return false;
            }
            oldStyleSpeed = SidTuneTools::readHex(speedIn, len, pos);
            hasSpeed = true;
        }
        else if ( SidTuneTools::myStrNcaseCmp(cmpBuf,_sidtune_keyword_name) == 0 )
        {
            strncpy( &infoString[0][0], cmpBuf + strlen(_sidtune_keyword_name), 31 );
            info.infoString[0] = &infoString[0][0];
            hasName = true;
        }
        else if ( SidTuneTools::myStrNcaseCmp(cmpBuf,_sidtune_keyword_author) == 0 )
        {
            strncpy( &infoString[1][0], cmpBuf + strlen(_sidtune_keyword_author), 31 );
            info.infoString[1] = &infoString[1][0];
            hasAuthor = true;
        }
        else if ( SidTuneTools::myStrNcaseCmp(cmpBuf,_sidtune_keyword_copyright) == 0 )
        {
            strncpy( &infoString[2][0], cmpBuf + strlen(_sidtune_keyword_copyright), 31 );
            info.infoString[2] = &infoString[2][0];
            hasCopyright = true;
        }
        else if ( SidTuneTools::myStrNcaseCmp(cmpBuf,_sidtune_keyword_musPlayer) == 0 )
        {
            info.musPlayer = true;
        }
        else
        {
            hasUnknownChunk = true;
#if defined(SIDTUNE_REJECT_UNKNOWN_FIELDS)
            info.formatString = _sidtune_txt_chunkError;
            return false;
#endif
        }
        // Skip to next tool.
        spTool += toolLen;
    }

    // Collected ``required'' information complete ?
    if ( hasAddress && hasName && hasAuthor && hasCopyright && hasSongs && hasSpeed )
    {
        // Create the speed/clock setting table.
        convertOldStyleSpeedToTables(oldStyleSpeed);
        if (( info.loadAddr == 0 ) && ( dataLength != 0 ))
        {
            SmartPtr_sidtt<const uint_least8_t> spDataBuf((const uint_least8_t*)dataBuffer,dataLength);
            spDataBuf += fileOffset;
            info.loadAddr = endian_16(spDataBuf[1],spDataBuf[0]);
            if ( !spDataBuf )
            {
                info.formatString = _sidtune_txt_dataCorruptError;
                return false;
            }
            fileOffset += 2;
        }
        if ( info.initAddr == 0 )
        {
            info.initAddr = info.loadAddr;
        }
        info.numberOfInfoStrings = 3;
        // We finally accept the input data.
        info.formatString = _sidtune_txt_format;
        return true;
    }
    else if ( hasAddress || hasName || hasAuthor || hasCopyright || hasSongs || hasSpeed )
    {
        // Something is missing (or damaged?).
        info.formatString = _sidtune_txt_corruptError;
        return false;
    }
    else
    {
        // No PlaySID conform info strings.
        info.formatString = _sidtune_txt_noStringsError;
        return false;
    }
}
