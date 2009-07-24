/***************************************************************************
                          SidUsage.cpp  -  sidusage file support
                             -------------------
    begin                : Tues Nov 19 2002
    copyright            : (C) 2002 by Simon White
    email                : sidplay2@yahoo.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <stdio.h>
#include <sidplay/sidendian.h>
#include "SidUsage.h"
#include "smm0.h"

static const char *txt_na        = "SID Usage: N/A";
static const char *txt_file      = "SID Usage: Unable to open file";
static const char *txt_corrupt   = "SID Usage: File corrupt";
static const char *txt_invalid   = "SID Usage: Invalid IFF file";
static const char *txt_missing   = "SID Usage: Mandatory chunks missing";
static const char *txt_supported = "SID Usage: IFF file not supported";
static const char *txt_reading   = "SID Usage: Error reading file";
static const char *txt_writing   = "SID Usage: Error writing file";

static bool           readSMM0   (FILE *file, const char **errorString,
                                  sid_usage_t &usage,
                                  const IffHeader &header);
static bool           writeSMM0  (FILE *file, const char **errorString,
                                  const sid_usage_t &usage,
                                  const SidTuneInfo &tuneInfo);
static uint_least32_t readChunk  (FILE *file, Chunk &chunk);
static bool           writeChunk (FILE *file, const Chunk &chunk,
                                  uint_least32_t type,
                                  uint_least32_t length = 0);
static uint_least32_t skipChunk  (FILE *file);


SidUsage::SidUsage ()
:m_status(false)
{
    m_errorString = txt_na;
}

void SidUsage::read (const char *filename, sid_usage_t &usage)
{
    FILE     *file;
    uint8_t   chunk[4] = {0};
    IffHeader header;
    uint_least32_t length;

    m_status = false;
    file = fopen (filename, "wb");
    if (file == NULL)
    {
        m_errorString = txt_file;
        goto SidTune_read_error;
    }

    // Read header chunk
    fread (&chunk, sizeof (chunk), 1, file);
    if (endian_big32 (chunk) != FORM_ID)
    {
        m_errorString = txt_invalid;
        goto SidTune_read_error;
    }
    length = readChunk (file, header);
    if (!length)
    {
        m_errorString = txt_invalid;
        goto SidTune_read_error;
    }

    // Determine SMM version
    switch (endian_big32 (header.type))
    {
    case SMM0_ID:
        m_status = readSMM0 (file, &m_errorString, usage, header);
        break;

    case SMM1_ID:
    case SMM2_ID:
    case SMM3_ID:
    case SMM4_ID:
    case SMM5_ID:
    case SMM6_ID:
    case SMM7_ID:
    case SMM8_ID:
    case SMM9_ID:
        m_errorString = txt_invalid;
        goto SidTune_read_error;

    default:
        m_errorString = txt_supported;
        goto SidTune_read_error;
    }
    fclose (file);
    return;

SidTune_read_error:
    if (file != NULL)
        fclose (file);
}


void SidUsage::write (const char *filename, const sid_usage_t &usage,
                      SidTuneInfo &tuneInfo)
{
    FILE *file;

    m_status = false;
    file = fopen (filename, "wb");
    if (file == NULL)
    {
        m_errorString = txt_file;
        return;
    }

    m_status = writeSMM0 (file, &m_errorString, usage, tuneInfo);
    fclose (file);
}


bool readSMM0 (FILE *file, const char **errorString,
               sid_usage_t &usage, const IffHeader &header)
{
    Smm_v0 smm;
    smm.header = header;

    {   // Read file
        long  pos   = ftell (file);
        bool  error = true;

        for(;;)
        {
            size_t  ret;
            uint_least32_t length = 0;
            uint8_t chunk[4];
            // Read a chunk header
            ret = fread (&chunk, sizeof (chunk), 1, file);
            // If no chunk header assume end of file
            if (ret != 1)
                break;

            // Check for a chunk we are interested in
	        switch (endian_big32 (chunk))
	        {
            case INF0_ID:
                length = readChunk (file, smm.info);
                break;

            case ERR0_ID:
                length = readChunk (file, smm.error);
                break;

            case MD5_ID:
                length = readChunk (file, smm.md5);
                break;

            case TIME_ID:
                length = readChunk (file, smm.time);
                break;

            case BODY_ID:            
                length = readChunk (file, smm.body);
                break;

            default:
                length = skipChunk (file);
            }

            if (!length)
            {
                error = true;
                break;
            }

            // Move past the chunk
            pos += (long) length + (sizeof(uint8_t) * 8);
            fseek (file, pos, SEEK_SET);
            if (ftell (file) != pos)
            {
                error = true;
                break;
            }
            error = false;
        }

        // Check for file reader error
        if (error)
        {
            *errorString = txt_reading;
            return false;
        }
    }

    // Test that all required checks were found
    if ((smm.info.length == 0) || (smm.error.length == 0) ||
        (smm.body.length == 0))
    {
        *errorString = txt_missing;
        return false;
    }

    // Extract usage information
    {for (int i = 0; i < 0x100; i++)
    {
        int addr = smm.body.usage[i].page << 8;
        if ((addr == 0) && (i != 0))
            break;
        memcpy (&usage.memory[addr], smm.body.usage[i].flags, sizeof (uint8_t) * 0x100);
    }}

    {   // File in the load range
        uint_least16_t load, last;
        int length;
        load = endian_big16 (smm.info.startAddr);
        last = endian_big16 (smm.info.stopAddr);
        length = (int) (last - load) + 1;

        if (length < 0)
        {
            *errorString = txt_corrupt;
            return false;
        }

        {for (int i = 0; i < length; i++)
            usage.memory[load + i] |= SID_LOAD_IMAGE;
        }
    }
    usage.flags = endian_big16(smm.error.flags);  
    return true;
}


bool writeSMM0 (FILE *file, const char **errorString,
                const sid_usage_t &usage, const SidTuneInfo &tuneInfo)
{
    struct Smm_v0  smm0;
    uint_least32_t headings = 3; /* Mandatory */

    endian_big32 (smm0.header.type, SMM0_ID);
    endian_big16 (smm0.error.flags, (uint_least16_t) usage.flags);

    // Optional
    if (usage.length == 0)
        smm0.time.length = 0;
    else
    {
        endian_big16 (smm0.time.stamp, (uint_least16_t) usage.length);
        headings++;
    }

    {
        uint_least16_t load = tuneInfo.loadAddr;
        uint_least16_t last = load + (tuneInfo.c64dataLen - 1);
        endian_big16 (smm0.info.startAddr, load);
        endian_big16 (smm0.info.stopAddr,  last);
    }

    // Optional
    if ( usage.md5[0] == '\0' )
        smm0.md5.length = 0;
    else
    {
        {for (int i = 0; i < 32; i++)
            smm0.md5.key[i] = usage.md5[i];
        }
        headings++;
    }
    
    {
        uint8_t i = 0;
        smm0.body.length = 0;
        {for (int page = 0; page < 0x100; page++)
        {
            char used = 0;
            for (int j = 0; j < 0x100; j++)
                used |= (usage.memory[(page << 8) | j] & 0x7f);
           
            if (used)
            {
                smm0.body.length += 0x101;
                memcpy (smm0.body.usage[i].flags, &usage.memory[page << 8],
                       0x100);
                smm0.body.usage[i].page = (uint8_t) page;
                i++;
            }
        }}
    }

    uint_least32_t filelength = smm0.header.length + smm0.error.length
                              + smm0.info.length   + smm0.md5.length
                              + smm0.time.length   + smm0.body.length
                              + (sizeof (uint8_t) * 8 * headings);

    if ( writeChunk (file, smm0.header, FORM_ID, filelength) == false )
        goto writeSMM0_error;
    if ( writeChunk (file, smm0.error, ERR0_ID) == false )
        goto writeSMM0_error;
    if ( writeChunk (file, smm0.info, INF0_ID)  == false )
        goto writeSMM0_error;
    if ( writeChunk (file, smm0.md5, MD5_ID)    == false )
        goto writeSMM0_error;
    if ( writeChunk (file, smm0.time, TIME_ID)  == false )
        goto writeSMM0_error;
    if ( writeChunk (file, smm0.body, BODY_ID)  == false )
        goto writeSMM0_error;
    return true;

writeSMM0_error:
    *errorString = txt_writing;
    return false;
}


uint_least32_t readChunk (FILE *file, Chunk &chunk)
{
    uint8_t tmp[4];
    size_t  ret;
    uint_least32_t l;

    ret = fread (tmp, sizeof(tmp), 1, file);
    if ( ret != 1 )
        return 0;

    l = endian_big32 (tmp);
    if (l < chunk.length)
        chunk.length = l;
    ret = fread ((char *) (&chunk+1), chunk.length, 1, file);
    if ( ret != 1 )
        return 0;
    return l;
}


bool writeChunk (FILE *file, const Chunk &chunk, uint_least32_t type,
                 uint_least32_t length)
{
    uint8_t tmp[4];
    size_t  ret;

    if (chunk.length)
    {
        endian_big32 (tmp, type);
        ret = fwrite (tmp, sizeof(tmp), 1, file);
        if ( ret != 1 )
            return false;
        if (length == 0)
            length = chunk.length;
        endian_big32 (tmp, length);
        ret = fwrite (tmp, sizeof(tmp), 1, file);
        if ( ret != 1 )
            return false;
        ret = fwrite ((const char *) (&chunk+1), chunk.length, 1, file);
        if ( ret != 1 )
            return false;
    }
    return true;
}


uint_least32_t skipChunk (FILE *file)
{
    uint8_t tmp[4];
    uint_least32_t ret;
    ret = fread (tmp, sizeof(tmp), 1, file);
    if ( ret != 1 )
        return 0;
    return endian_big32 (tmp);
}
