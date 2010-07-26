/*
 * /home/ms/files/source/libsidtune/RCS/InfoFile.cpp,v
 *
 * SIDPLAY INFOFILE format support.
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

#ifdef HAVE_EXCEPTIONS
#   include <new>
#endif
#include <ctype.h>
#include <string.h>

#include "SidTune.h"
#include "SidTuneTools.h"
#include "sidendian.h"


const char text_format[] = "Raw plus SIDPLAY ASCII text file (SID)";

const char text_truncatedError[] = "ERROR: SID file is truncated";
const char text_noMemError[] = "ERROR: Not enough free memory";

const char keyword_id[] = "SIDPLAY INFOFILE";

const char keyword_name[] = "NAME=";            // No white-space characters 
const char keyword_author[] = "AUTHOR=";        // in these keywords, because
const char keyword_copyright[] = "COPYRIGHT=";  // we want to use a white-space
const char keyword_released[] = "RELEASED=";    // we want to use a white-space
const char keyword_address[] = "ADDRESS=";      // eating string stream to
const char keyword_songs[] = "SONGS=";          // parse most of the header.
const char keyword_speed[] = "SPEED=";
const char keyword_musPlayer[] = "SIDSONG=YES";
const char keyword_reloc[] = "RELOC=";
const char keyword_clock[] = "CLOCK=";
const char keyword_sidModel[] = "SIDMODEL=";
const char keyword_compatibility[] = "COMPATIBILITY=";

const uint_least16_t sidMinFileSize = 1+sizeof(keyword_id);  // Just to avoid a first segm.fault.
const uint_least16_t parseChunkLen = 80;                     // Enough for all keywords incl. their values.


bool SidTune::SID_fileSupport(const void* dataBuffer, uint_least32_t dataBufLen,
                              const void* sidBuffer, uint_least32_t sidBufLen)
{
    // Make sure SID buffer pointer is not zero.
    // Check for a minimum file size. If it is smaller, we will not proceed.
    if ((sidBuffer==0) || (sidBufLen<sidMinFileSize))
    {
        return false;
    }

    const char* pParseBuf = (const char*)sidBuffer;
    // First line has to contain the exact identification string.
    if ( SidTuneTools::myStrNcaseCmp( pParseBuf, keyword_id ) != 0 )
    {
        return false;
    }
    else
    {
        // At least the ID was found, so set a default error message.
        info.formatString = text_truncatedError;
        
        // Defaults.
        fileOffset = 0;                // no header in separate data file
        info.sidChipBase1 = 0xd400;
        info.sidChipBase2 = 0;
        info.musPlayer = false;
        info.numberOfInfoStrings = 0;
        uint_least32_t oldStyleSpeed = 0;

        // Flags for required entries.
        bool hasAddress = false,
            hasName = false,
            hasAuthor = false,
            hasReleased = false,
            hasSongs = false,
            hasSpeed = false;
    
        // Using a temporary instance of an input string chunk.
#ifdef HAVE_EXCEPTIONS
        char* pParseChunk = new(std::nothrow) char[parseChunkLen+1];
#else
        char* pParseChunk = new char[parseChunkLen+1];
#endif
        if ( pParseChunk == 0 )
        {
            info.formatString = text_noMemError;
            return false;
        }
        
        // Parse as long we have not collected all ``required'' entries.
        //while ( !hasAddress || !hasName || !hasAuthor || !hasCopyright
        //        || !hasSongs || !hasSpeed )

        // Above implementation is wrong, we need to get all known
        // fields and then check if all ``required'' ones were found.
        for (;;)
        {
            // Skip to next line. Leave loop, if none.
            if (( pParseBuf = SidTuneTools::returnNextLine( pParseBuf )) == 0 )
            {
                break;
            }
            // And get a second pointer to the following line.
            const char* pNextLine = SidTuneTools::returnNextLine( pParseBuf );
            uint_least32_t restLen;
            if ( pNextLine != 0 )
            {
                // Calculate number of chars between current pos and next line.
                restLen = (uint_least32_t)(pNextLine - pParseBuf);
            }
            else
            {
                // Calculate number of chars between current pos and end of buf.
                restLen = sidBufLen - (uint_least32_t)(pParseBuf - (char*)sidBuffer);
            }
            // Create whitespace eating (!) input string stream.

            const char *s = pParseBuf;
            int pos = 0;
            int posCopy = 0;
            // Now copy the next X characters except white-spaces.
            for ( uint_least16_t i = 0; i < parseChunkLen; i++ )
            {
                pParseChunk[i] = s[i];
            }
            pParseChunk[parseChunkLen]=0;
            // Now check for the possible keywords.
            // ADDRESS
            if ( SidTuneTools::myStrNcaseCmp( pParseChunk, keyword_address ) == 0 )
            {
                SidTuneTools::skipToEqu( s, restLen, pos );
                info.loadAddr = (uint_least16_t)SidTuneTools::readHex( s, restLen, pos );
                if ( pos>=restLen )
                    break;
                info.initAddr = (uint_least16_t)SidTuneTools::readHex( s, restLen, pos );
                if ( pos>=restLen )
                    break;
                info.playAddr = (uint_least16_t)SidTuneTools::readHex( s, restLen, pos );
                hasAddress = true;
            }
            // NAME
            else if ( SidTuneTools::myStrNcaseCmp( pParseChunk, keyword_name ) == 0 )
            {
                SidTuneTools::copyStringValueToEOL(pParseBuf,&infoString[0][0],SIDTUNE_MAX_CREDIT_STRLEN);
                info.infoString[0] = &infoString[0][0];
                hasName = true;
            }
            // AUTHOR
            else if ( SidTuneTools::myStrNcaseCmp( pParseChunk, keyword_author ) == 0 )
            {
                SidTuneTools::copyStringValueToEOL(pParseBuf,&infoString[1][0],SIDTUNE_MAX_CREDIT_STRLEN);
                info.infoString[1] = &infoString[1][0];
                hasAuthor = true;
            }
            // COPYRIGHT
            else if ( SidTuneTools::myStrNcaseCmp( pParseChunk, keyword_copyright ) == 0 )
            {
                SidTuneTools::copyStringValueToEOL(pParseBuf,&infoString[2][0],SIDTUNE_MAX_CREDIT_STRLEN);
                info.infoString[2] = &infoString[2][0];
                hasReleased = true;
            }
            // RELEASED
            else if ( SidTuneTools::myStrNcaseCmp( pParseChunk, keyword_released ) == 0 )
            {
                SidTuneTools::copyStringValueToEOL(pParseBuf,&infoString[2][0],SIDTUNE_MAX_CREDIT_STRLEN);
                info.infoString[2] = &infoString[2][0];
                hasReleased = true;
            }
            // SONGS
            else if ( SidTuneTools::myStrNcaseCmp( pParseChunk, keyword_songs ) == 0 )
            {
                SidTuneTools::skipToEqu( s, restLen, pos );
                info.songs = (uint_least16_t)SidTuneTools::readDec( s, restLen, pos );
                info.startSong = (uint_least16_t)SidTuneTools::readDec( s, restLen, pos );
                hasSongs = true;
            }
            // SPEED
            else if ( SidTuneTools::myStrNcaseCmp( pParseChunk, keyword_speed ) == 0 )
            {
                SidTuneTools::skipToEqu( s, restLen, pos );
                oldStyleSpeed = SidTuneTools::readHex( s, restLen, pos );
                hasSpeed = true;
            }
            // SIDSONG
            else if ( SidTuneTools::myStrNcaseCmp( pParseChunk, keyword_musPlayer ) == 0 )
            {
                info.musPlayer = true;
            }
            // RELOC
            else if ( SidTuneTools::myStrNcaseCmp( pParseChunk, keyword_reloc ) == 0 )
            {
                info.relocStartPage = (uint_least8_t)SidTuneTools::readHex( s, restLen, pos );
                if ( pos >= restLen )
                    break;
                info.relocPages = (uint_least8_t)SidTuneTools::readHex( s, restLen, pos );
            }
            // CLOCK
            else if ( SidTuneTools::myStrNcaseCmp( pParseChunk, keyword_clock ) == 0 )
            {
                char clock[8];
                SidTuneTools::copyStringValueToEOL(pParseBuf,clock,sizeof(clock));
                if ( SidTuneTools::myStrNcaseCmp( clock, "UNKNOWN" ) == 0 )
                    info.clockSpeed = SIDTUNE_CLOCK_UNKNOWN;
                else if ( SidTuneTools::myStrNcaseCmp( clock, "PAL" ) == 0 )
                    info.clockSpeed = SIDTUNE_CLOCK_PAL;
                else if ( SidTuneTools::myStrNcaseCmp( clock, "NTSC" ) == 0 )
                    info.clockSpeed = SIDTUNE_CLOCK_NTSC;
                else if ( SidTuneTools::myStrNcaseCmp( clock, "ANY" ) == 0 )
                    info.clockSpeed = SIDTUNE_CLOCK_ANY;
            }
            // SIDMODEL
            else if ( SidTuneTools::myStrNcaseCmp( pParseChunk, keyword_sidModel ) == 0 )
            {
                char model[8];
                SidTuneTools::copyStringValueToEOL(pParseBuf,model,sizeof(model));
                if ( SidTuneTools::myStrNcaseCmp( model, "UNKNOWN" ) == 0 )
                    info.sidModel = SIDTUNE_SIDMODEL_UNKNOWN;
                else if ( SidTuneTools::myStrNcaseCmp( model, "6581" ) == 0 )
                    info.sidModel = SIDTUNE_SIDMODEL_6581;
                else if ( SidTuneTools::myStrNcaseCmp( model, "8580" ) == 0 )
                    info.sidModel = SIDTUNE_SIDMODEL_8580;
                else if ( SidTuneTools::myStrNcaseCmp( model, "ANY" ) == 0 )
                    info.sidModel = SIDTUNE_SIDMODEL_ANY;
            }
            // COMPATIBILITY
            else if ( SidTuneTools::myStrNcaseCmp( pParseChunk, keyword_compatibility ) == 0 )
            {
                char comp[5];
                SidTuneTools::copyStringValueToEOL(pParseBuf,comp,sizeof(comp));
                if ( SidTuneTools::myStrNcaseCmp( comp, "C64" ) == 0 )
                    info.compatibility = SIDTUNE_COMPATIBILITY_C64;
                else if ( SidTuneTools::myStrNcaseCmp( comp, "PSID" ) == 0 )
                    info.compatibility = SIDTUNE_COMPATIBILITY_PSID;
                else if ( SidTuneTools::myStrNcaseCmp( comp, "R64" ) == 0 )
                    info.compatibility = SIDTUNE_COMPATIBILITY_R64;
            }
        };
        
        delete[] pParseChunk;
        
        // Again check for the ``required'' values.
        if ( hasAddress || hasName || hasAuthor || hasReleased || hasSongs || hasSpeed )
        {
            // Check reserved fields to force real c64 compliance
            if (info.compatibility == SIDTUNE_COMPATIBILITY_R64)
            {
                if (checkRealC64Info (oldStyleSpeed) == false)
                    return false;
            }
            // Create the speed/clock setting table.
            convertOldStyleSpeedToTables(oldStyleSpeed, info.clockSpeed);
            // loadAddr = 0 means, the address is stored in front of the C64 data.
            // We cannot verify whether the dataBuffer contains valid data.
            // All we want to know is whether the SID buffer is valid.
            // If data is present, we access it (here to get the C64 load address).
            if (info.loadAddr==0 && (dataBufLen>=(fileOffset+2)) && dataBuffer!=0)
            {
                const uint8_t* pDataBufCp = (const uint8_t*)dataBuffer + fileOffset;
                info.loadAddr = endian_16( *(pDataBufCp + 1), *pDataBufCp );
                fileOffset += 2;  // begin of data
            }
            // Keep compatibility to PSID/SID.
            if ( info.initAddr == 0 )
            {
                info.initAddr = info.loadAddr;
            }
            info.numberOfInfoStrings = 3;
            // We finally accept the input data.
            info.formatString = text_format;
            return true;
        }
        else
        {
            // Something is missing (or damaged ?).
            // Error string set above.
            return false;
        }
    }
}


#if 0
bool SidTune::SID_fileSupportSave( std::ofstream& toFile )
{
    toFile << keyword_id << std::endl
        << keyword_address << std::hex << std::setw(4)
        << std::setfill('0') << 0 << ','
        << std::hex << std::setw(4) << info.initAddr << ","
        << std::hex << std::setw(4) << info.playAddr << std::endl
        << keyword_songs << std::dec << (int)info.songs << ","
        << (int)info.startSong << std::endl;

    uint_least32_t oldStyleSpeed = 0;
    int maxBugSongs = ((info.songs <= 32) ? info.songs : 32);
    for (int s = 0; s < maxBugSongs; s++)
    {
        if (songSpeed[s] == SIDTUNE_SPEED_CIA_1A)
        {
            oldStyleSpeed |= (1<<s);
        }
    }

    toFile
        << keyword_speed << std::hex << std::setw(8)
        << oldStyleSpeed << std::endl
        << keyword_name << info.infoString[0] << std::endl
        << keyword_author << info.infoString[1] << std::endl
        << keyword_released << info.infoString[2] << std::endl;
    if ( info.musPlayer )
    {
        toFile << keyword_musPlayer << std::endl;
    }
    if ( info.relocStartPage )
    {
        toFile
            << keyword_reloc << std::setfill('0')
            << std::hex << std::setw(2) << (int) info.relocStartPage << ","
            << std::hex << std::setw(2) << (int) info.relocPages << std::endl;
    }
    if ( info.clockSpeed != SIDTUNE_CLOCK_UNKNOWN )
    {
        toFile << keyword_clock;
        switch (info.clockSpeed)
        {
        case SIDTUNE_CLOCK_PAL:
            toFile << "PAL";
            break;
        case SIDTUNE_CLOCK_NTSC:
            toFile << "NTSC";
            break;
        case SIDTUNE_CLOCK_ANY:
            toFile << "ANY";
            break;
        }
        toFile << std::endl;
    }
    if ( info.sidModel != SIDTUNE_SIDMODEL_UNKNOWN )
    {
        toFile << keyword_sidModel;
        switch (info.sidModel)
        {
        case SIDTUNE_SIDMODEL_6581:
            toFile << "6581";
            break;
        case SIDTUNE_SIDMODEL_8580:
            toFile << "8580";
            break;
        case SIDTUNE_SIDMODEL_ANY:
            toFile << "ANY";
            break;
        }
        toFile << std::endl;
    }
    if ( info.compatibility == SIDTUNE_COMPATIBILITY_PSID )
    {
        toFile << keyword_compatibility << "C64" << std::endl;
    }
    else if ( info.compatibility == SIDTUNE_COMPATIBILITY_R64 )
    {
        toFile << keyword_compatibility << "R64" << std::endl;
    }

    if ( !toFile )
    {
        return false;
    }
    else
    {
        return true;
    }
}
#endif
