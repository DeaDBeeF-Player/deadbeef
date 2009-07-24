/***************************************************************************
                          psiddrv.cpp  -  PSID Driver Installtion
                             -------------------
    begin                : Fri Jul 27 2001
    copyright            : (C) 2001 by Simon White
    email                : s_a_white@email.com
 ***************************************************************************/
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
/***************************************************************************
 *  $Log: psiddrv.cpp,v $
 *  Revision 1.25  2002/12/13 22:07:29  s_a_white
 *  C64 code fixed so a theres no decrement before checking the random value.
 *
 *  Revision 1.24  2002/11/27 00:16:51  s_a_white
 *  Make sure driver info gets reset and exported properly.
 *
 *  Revision 1.23  2002/11/25 21:09:42  s_a_white
 *  Reset address for old sidplay1 modes now directly passed to the CPU.  This
 *  prevents tune corruption and banking issues for the different modes.
 *
 *  Revision 1.22  2002/11/19 22:53:23  s_a_white
 *  Sidplay1 modes modified to make them nolonger require the psid driver.
 *
 *  Revision 1.21  2002/11/01 19:11:21  s_a_white
 *  Export random delay used in song.
 *
 *  Revision 1.20  2002/11/01 17:36:02  s_a_white
 *  Frame based support for old sidplay1 modes.
 *
 *  Revision 1.19  2002/10/02 19:42:59  s_a_white
 *  RSID support.
 *
 *  Revision 1.18  2002/09/12 21:01:32  s_a_white
 *  Added support for simulating the random delay before the user loads a
 *  program on a real C64.
 *
 *  Revision 1.17  2002/09/09 18:01:30  s_a_white
 *  Prevent m_info driver details getting modified when C64 crashes.
 *
 *  Revision 1.16  2002/07/21 19:39:40  s_a_white
 *  Proper error handling of reloc info overlapping load image.
 *
 *  Revision 1.15  2002/07/18 18:37:42  s_a_white
 *  Buffer overflow fixes for tunes providing bad reloc information.
 *
 *  Revision 1.14  2002/07/17 21:48:10  s_a_white
 *  PSIDv2NG reloc exclude region extension.
 *
 *  Revision 1.13  2002/03/12 18:43:59  s_a_white
 *  Tidy up handling of envReset on illegal CPU instructions.
 *
 *  Revision 1.12  2002/03/03 22:02:36  s_a_white
 *  Sidplay2 PSID driver length now exported.
 *
 *  Revision 1.11  2002/02/17 12:41:18  s_a_white
 *  Fixed to not so easily break when C64 code is modified.
 *
 *  Revision 1.10  2002/02/04 23:50:48  s_a_white
 *  Improved compatibilty with older sidplay1 modes.
 *
 *  Revision 1.9  2002/01/29 21:50:33  s_a_white
 *  Auto switching to a better emulation mode.  tuneInfo reloaded after a
 *  config.  Initial code added to support more than two sids.
 *
 *  Revision 1.8  2001/12/21 21:54:14  s_a_white
 *  Improved compatibility if Sidplay1 bankswitching mode.
 *
 *  Revision 1.7  2001/12/13 08:28:08  s_a_white
 *  Added namespace support to fix problems with xsidplay.
 *
 *  Revision 1.6  2001/11/16 19:23:18  s_a_white
 *  Fixed sign of buf for reloc65 call.
 *
 *  Revision 1.5  2001/10/28 21:28:35  s_a_white
 *  For none real mode if play != 0 we now always jump to irqjob instead of
 *  playAddr.
 *
 *  Revision 1.4  2001/10/02 18:31:24  s_a_white
 *  No longer dies if relocStartPages != 0 byr relocPages == 0.  For none real
 *  evironment modes, play is always followed even if interrupt handlers are
 *  installed.
 *
 *  Revision 1.3  2001/09/01 11:13:18  s_a_white
 *  Fixes sidplay1 environment modes.
 *
 *  Revision 1.2  2001/07/27 12:52:12  s_a_white
 *  Removed warning.
 *
 *  Revision 1.1  2001/07/27 12:12:23  s_a_white
 *  Initial release.
 *
 ***************************************************************************/

// --------------------------------------------------------
// The code here is use to support the PSID Version 2NG
// (proposal B) file format for player relocation support.
// --------------------------------------------------------
#include "sidendian.h"
#include "player.h"

#define PSIDDRV_MAX_PAGE 0xff

SIDPLAY2_NAMESPACE_START

const char *Player::ERR_PSIDDRV_NO_SPACE  = "ERROR: No space to install psid driver in C64 ram"; 
const char *Player::ERR_PSIDDRV_RELOC     = "ERROR: Failed whilst relocating psid driver";

extern "C" int reloc65(unsigned char** buf, int* fsize, int addr);

int Player::psidDrvInstall (SidTuneInfo &tuneInfo, sid2_info_t &info)
{
    uint_least16_t relocAddr;
    int startlp = tuneInfo.loadAddr >> 8;
    int endlp   = (tuneInfo.loadAddr + (tuneInfo.c64dataLen - 1)) >> 8;

    if (info.environment != sid2_envR)
    {   // Sidplay1 modes require no psid driver
        info.driverAddr   = 0;
        info.driverLength = 0;
        info.rnddelay     = 0;
        return 0;
    }

    // Check for free space in tune
    if (tuneInfo.relocStartPage == PSIDDRV_MAX_PAGE)
        tuneInfo.relocPages = 0;
    // Check if we need to find the reloc addr
    else if (tuneInfo.relocStartPage == 0)
    {   // Tune is clean so find some free ram around the
        // load image
        psidRelocAddr (tuneInfo, startlp, endlp);
    }
    else
    {   // Check reloc information mode
        int startrp = tuneInfo.relocStartPage;
        int endrp   = startrp + (tuneInfo.relocPages - 1);

        // New relocation implementation (exclude region)
        // to complement existing method rejected as being
        // unnecessary.  From tests in most cases this
        // method increases memory availibility.
        /*************************************************
        if ((startrp <= startlp) && (endrp >= endlp))
        {   // Is describing used space so find some free
            // ram outside this range
            psidRelocAddr (tuneInfo, startrp, endrp);
        }
        *************************************************/
    }

    if (tuneInfo.relocPages < 1)
    {
        m_errorString = ERR_PSIDDRV_NO_SPACE;
        return -1;
    }

    relocAddr = tuneInfo.relocStartPage << 8;

    {   // Place psid driver into ram
        uint8_t psid_driver[] = {
#          include "psiddrv.bin"
        };
        uint8_t *reloc_driver = psid_driver;
        int      reloc_size   = sizeof (psid_driver);

        if (!reloc65 (&reloc_driver, &reloc_size, relocAddr - 13))
        {
            m_errorString = ERR_PSIDDRV_RELOC;
            return -1;
        }

        // Adjust size to not included initialisation data.
        reloc_size -= 13;
        info.driverAddr   = relocAddr;
        info.driverLength = (uint_least16_t) reloc_size;
        // Round length to end of page
        info.driverLength += 0xff;
        info.driverLength &= 0xff00;

        m_ram[0x310] = JMPw;
        memcpy (&m_ram[0x0311], &reloc_driver[4], 9);
        memcpy (&m_rom[0xfffc], &reloc_driver[0], 2); /* RESET */

        {   // Experimental exit to basic support
            uint_least16_t addr;
            addr = endian_little16(&reloc_driver[2]);
            m_rom[0xa7ae] = JMPw;
            endian_little16 (&m_rom[0xa7af], 0xffe1);
            endian_little16 (&m_ram[0x0328], addr);
        }
        memcpy (&m_ram[relocAddr], &reloc_driver[13], reloc_size);
    }

    {   // Setup the Initial entry point
        uint_least16_t playAddr = tuneInfo.playAddr;
        uint_least16_t addr = relocAddr;

        // Tell C64 about song
        m_ram[addr++] = (uint8_t) tuneInfo.currentSong;
        if (tuneInfo.songSpeed == SIDTUNE_SPEED_VBI)
            m_ram[addr] = 0;
        else // SIDTUNE_SPEED_CIA_1A
            m_ram[addr] = 1;

        addr++;
        endian_little16 (&m_ram[addr], tuneInfo.initAddr);
        addr += 2;
        endian_little16 (&m_ram[addr], playAddr);
        addr += 2;
        // Below we limit the delay to something sensible.
        info.rnddelay = (uint_least16_t) (m_rand >> 3) & 0x0FFF;
        endian_little16 (&m_ram[addr], m_info.rnddelay);
        addr += 2;
        m_rand        = m_rand * 13 + 1;
        m_ram[addr++] = iomap (m_tuneInfo.initAddr);
        m_ram[addr++] = iomap (m_tuneInfo.playAddr);
    }
    return 0;
}


void Player::psidRelocAddr (SidTuneInfo &tuneInfo, int startp, int endp)
{   // Used memory ranges.
    bool pages[256];
    int  used[] = {0x00,   0x03,
                   0xa0,   0xbf,
                   0xd0,   0xff,
                   startp, endp};

    // Mark used pages in table.
    memset(pages, false, sizeof(pages));
    for (size_t i = 0; i < sizeof(used)/sizeof(*used); i += 2)
    {
        for (int page = used[i]; page <= used[i + 1]; page++)
            pages[page] = true;
    }

    {   // Find largest free range.
        int relocPages, lastPage = 0;
        tuneInfo.relocPages = 0;
        for (size_t page = 0; page < sizeof(pages)/sizeof(*pages); page++)
        {
            if (pages[page] == false)
                continue;
            relocPages = page - lastPage;
            if (relocPages > tuneInfo.relocPages)
            {
                tuneInfo.relocStartPage = lastPage;
                tuneInfo.relocPages     = relocPages;
            }
            lastPage = page + 1;
        }
    }

    if (tuneInfo.relocPages    == 0)
        tuneInfo.relocStartPage = PSIDDRV_MAX_PAGE;
}

SIDPLAY2_NAMESPACE_STOP
