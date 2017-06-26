/***************************************************************************
                          player.cpp  -  Main Library Code
                             -------------------
    begin                : Fri Jun 9 2000
    copyright            : (C) 2000 by Simon White
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
 *  $Log: player.cpp,v $
 *  Revision 1.57  2002/12/14 10:10:30  s_a_white
 *  Kernal Mod: Bypass screen clear as we don't have a screen.  Fix setting
 *  of PAL/NTSC flag for real c64 mode.
 *
 *  Revision 1.56  2002/12/13 22:01:54  s_a_white
 *  Kernel mods:  Memory now bypassed by upping the start address so the
 *  end or ram is found instantly.  Basic cold/warm start address points to a
 *  busy loop to prevent undersiable side effects (this may change later).
 *
 *  Revision 1.55  2002/11/27 00:16:51  s_a_white
 *  Make sure driver info gets reset and exported properly.
 *
 *  Revision 1.54  2002/11/25 21:09:41  s_a_white
 *  Reset address for old sidplay1 modes now directly passed to the CPU.  This
 *  prevents tune corruption and banking issues for the different modes.
 *
 *  Revision 1.53  2002/11/21 19:53:58  s_a_white
 *  CPU nolonger a special case.  It now uses the event scheduler like all the
 *  other components.
 *
 *  Revision 1.52  2002/11/20 21:44:03  s_a_white
 *  Fix fake IRQ to properly obtain next address.
 *
 *  Revision 1.51  2002/11/19 22:55:18  s_a_white
 *  Sidplay1 modes modified to make them nolonger require the psid driver.
 *  Full c64 kernal supported in real c64 mode.
 *
 *  Revision 1.50  2002/11/01 17:36:01  s_a_white
 *  Frame based support for old sidplay1 modes.
 *
 *  Revision 1.49  2002/10/20 08:58:36  s_a_white
 *  Modify IO map so psiddrv can detect special cases.
 *
 *  Revision 1.48  2002/10/15 18:20:54  s_a_white
 *  Make all addresses from ea31 to ea7d valid IRQ exit points.  This
 *  approximates the functionality of a real C64.
 *
 *  Revision 1.47  2002/10/02 19:43:47  s_a_white
 *  RSID support.
 *
 *  Revision 1.46  2002/09/17 17:02:41  s_a_white
 *  Fixed location of kernel IRQ exit code.
 *
 *  Revision 1.45  2002/09/12 21:01:30  s_a_white
 *  Added support for simulating the random delay before the user loads a
 *  program on a real C64.
 *
 *  Revision 1.44  2002/09/09 18:01:30  s_a_white
 *  Prevent m_info driver details getting modified when C64 crashes.
 *
 *  Revision 1.43  2002/08/20 23:21:41  s_a_white
 *  Setup default sample format.
 *
 *  Revision 1.42  2002/04/14 21:46:50  s_a_white
 *  PlaySID reads fixed to come from RAM only.
 *
 *  Revision 1.41  2002/03/12 18:43:59  s_a_white
 *  Tidy up handling of envReset on illegal CPU instructions.
 *
 *  Revision 1.40  2002/03/11 18:01:30  s_a_white
 *  Prevent lockup if config call fails with existing and old configurations.
 *
 *  Revision 1.39  2002/03/03 22:01:58  s_a_white
 *  New clock speed & sid model interface.
 *
 *  Revision 1.38  2002/02/17 16:33:02  s_a_white
 *  New reset interface for sidbuilders.
 *
 *  Revision 1.37  2002/02/17 12:42:45  s_a_white
 *  envReset now sets playerStopped indicators.  This means the player
 *  nolonger locks up when a HLT instruction is encountered.
 *
 *  Revision 1.36  2002/02/06 20:12:03  s_a_white
 *  Added sidplay1 random extension for vic reads.
 *
 *  Revision 1.35  2002/01/29 21:50:33  s_a_white
 *  Auto switching to a better emulation mode.  m_tuneInfo reloaded after a
 *  config.  Initial code added to support more than two sids.
 *
 *  Revision 1.34  2002/01/14 23:16:27  s_a_white
 *  Prevent multiple initialisations if already stopped.
 *
 *  Revision 1.33  2001/12/13 08:28:08  s_a_white
 *  Added namespace support to fix problems with xsidplay.
 *
 *  Revision 1.32  2001/11/16 19:25:33  s_a_white
 *  Removed m_context as where getting mixed with parent class.
 *
 *  Revision 1.31  2001/10/18 22:33:40  s_a_white
 *  Initialisation order fixes.
 *
 *  Revision 1.30  2001/10/02 18:26:36  s_a_white
 *  Removed ReSID support and updated for new scheduler.
 *
 *  Revision 1.29  2001/09/17 19:02:38  s_a_white
 *  Now uses fixed point maths for sample output and rtc.
 *
 *  Revision 1.28  2001/09/04 18:50:57  s_a_white
 *  Fake CIA address now masked.
 *
 *  Revision 1.27  2001/09/01 11:15:46  s_a_white
 *  Fixes sidplay1 environment modes.
 *
 *  Revision 1.26  2001/08/10 20:04:46  s_a_white
 *  Initialise requires rtc reset for correct use with stop operation.
 *
 *  Revision 1.25  2001/07/27 12:51:55  s_a_white
 *  Removed warning.
 *
 *  Revision 1.24  2001/07/25 17:01:13  s_a_white
 *  Support for new configuration interface.
 *
 *  Revision 1.23  2001/07/14 16:46:16  s_a_white
 *  Sync with sidbuilder class project.
 *
 *  Revision 1.22  2001/07/14 12:56:15  s_a_white
 *  SID caching no longer needed. IC  components now run using event
 *  generation (based on VICE).  Handling of IRQs now more effecient.  All
 *  sidplay1 hacks either removed or moved to sid6510.  Fixed PAL/NTSC
 *  speeding fixing.  Now uses new component and sidbuilder classes.
 *
 *  Revision 1.21  2001/04/23 17:09:56  s_a_white
 *  Fixed video speed selection using unforced/forced and NTSC clockSpeeds.
 *
 *  Revision 1.20  2001/03/26 21:46:43  s_a_white
 *  Removed unused #include.
 *
 *  Revision 1.19  2001/03/25 19:48:13  s_a_white
 *  xsid.reset added.
 *
 *  Revision 1.18  2001/03/22 22:45:20  s_a_white
 *  Re-ordered initialisations to match defintions.
 *
 *  Revision 1.17  2001/03/21 22:32:34  s_a_white
 *  Filter redefinition support.  VIC & NMI support added.  Moved fake interrupts
 *  to sid6510 class.
 *
 *  Revision 1.16  2001/03/09 22:26:36  s_a_white
 *  Support for updated C64 player.
 *
 *  Revision 1.15  2001/03/08 22:46:42  s_a_white
 *  playAddr = 0xffff now better supported.
 *
 *  Revision 1.14  2001/03/01 23:46:37  s_a_white
 *  Support for sample mode to be selected at runtime.
 *
 *  Revision 1.13  2001/02/28 18:55:27  s_a_white
 *  Removed initBank* related stuff.  IRQ terminating ROM jumps at 0xea31,
 *  0xea7e and 0xea81 now handled.
 *
 *  Revision 1.12  2001/02/21 21:43:10  s_a_white
 *  Now use VSID code and this handles interrupts much better!  The whole
 *  initialise sequence has been modified to support this.
 *
 *  Revision 1.11  2001/02/13 21:01:14  s_a_white
 *  Support for real interrupts.  C64 Initialisation routine now run from Player::play
 *  instead of Player::initialise.  Prevents lockups if init routine does not return.
 *
 *  Revision 1.10  2001/02/08 17:21:14  s_a_white
 *  Initial SID volumes not being stored in cache.  Fixes Dulcedo Cogitationis.
 *
 *  Revision 1.9  2001/02/07 20:56:46  s_a_white
 *  Samples now delayed until end of simulated frame.
 *
 *  Revision 1.8  2001/01/23 21:26:28  s_a_white
 *  Only way to load a tune now is by passing in a sidtune object.  This is
 *  required for songlength database support.
 *
 *  Revision 1.7  2001/01/07 15:13:39  s_a_white
 *  Hardsid update to mute sids when program exits.
 *
 *  Revision 1.6  2000/12/21 22:48:27  s_a_white
 *  Re-order voices for mono to stereo conversion to match sidplay1.
 *
 *  Revision 1.5  2000/12/14 23:53:36  s_a_white
 *  Small optimisation update, and comment revision.
 *
 *  Revision 1.4  2000/12/13 17:56:24  s_a_white
 *  Interrupt vector address changed from 0x315 to 0x314.
 *
 *  Revision 1.3  2000/12/13 12:00:25  mschwendt
 *  Corrected order of members in member initializer-list.
 *
 *  Revision 1.2  2000/12/12 22:50:15  s_a_white
 *  Bug Fix #122033.
 *
 ***************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "config.h"
#include "sidendian.h"
#include "player.h"

#ifdef HAVE_EXCEPTIONS
#   include <new>
#endif

static const uint8_t kernal[] = {
#include "kernal.bin"
};

SIDPLAY2_NAMESPACE_START

const double Player::CLOCK_FREQ_NTSC = 1022727.14;
const double Player::CLOCK_FREQ_PAL  = 985248.4;
const double Player::VIC_FREQ_PAL    = 50.0;
const double Player::VIC_FREQ_NTSC   = 60.0;

// These texts are used to override the sidtune settings.
const char  *Player::TXT_PAL_VBI        = "50 Hz VBI (PAL)";
const char  *Player::TXT_PAL_VBI_FIXED  = "60 Hz VBI (PAL FIXED)";
const char  *Player::TXT_PAL_CIA        = "CIA (PAL)";
const char  *Player::TXT_PAL_UNKNOWN    = "UNKNOWN (PAL)";
const char  *Player::TXT_NTSC_VBI       = "60 Hz VBI (NTSC)";
const char  *Player::TXT_NTSC_VBI_FIXED = "50 Hz VBI (NTSC FIXED)";
const char  *Player::TXT_NTSC_CIA       = "CIA (NTSC)";
const char  *Player::TXT_NTSC_UNKNOWN   = "UNKNOWN (NTSC)";
const char  *Player::TXT_NA             = "NA";

// Error Strings
const char  *Player::ERR_CONF_WHILST_ACTIVE    = "SIDPLAYER ERROR: Trying to configure player whilst active.";
const char  *Player::ERR_UNSUPPORTED_FREQ      = "SIDPLAYER ERROR: Unsupported sampling frequency.";
const char  *Player::ERR_UNSUPPORTED_PRECISION = "SIDPLAYER ERROR: Unsupported sample precision.";
const char  *Player::ERR_MEM_ALLOC             = "SIDPLAYER ERROR: Memory Allocation Failure.";
const char  *Player::ERR_UNSUPPORTED_MODE      = "SIDPLAYER ERROR: Unsupported Environment Mode (Coming Soon).";

const char  *Player::credit[];


// Set the ICs environment variable to point to
// this player
Player::Player (void)
// Set default settings for system
:c64env  (&m_scheduler),
 m_scheduler ("SIDPlay 2"),
 sid6510 (&m_scheduler),
 mos6510 (&m_scheduler),
 cpu     (&sid6510),
 xsid    (this, &nullsid),
 cia     (this),
 cia2    (this),
 sid6526 (this),
 vic     (this),
 mixerEvent (this),
 rtc        (&m_scheduler),
 m_tune (NULL),
 m_ram  (NULL),
 m_rom  (NULL),
 m_errorString       (TXT_NA),
 m_fastForwardFactor (1.0),
 m_mileage           (0),
 m_playerState       (sid2_stopped),
 m_running           (false),
 m_sampleCount       (0)
{
//    srand ((uint) ::time(NULL));
    m_rand = (uint_least32_t) rand ();
    
    // Set the ICs to use this environment
    sid6510.setEnvironment (this);
    mos6510.setEnvironment (this);

    // SID Initialise
    for (int i = 0; i < SID2_MAX_SIDS; i++)
        sid[i] = &nullsid;
    xsid.emulation(sid[0]);
    sid[0] = &xsid;

    // Setup exported info
    m_info.credits         = credit;
    m_info.channels        = 1;
    m_info.driverAddr      = 0;
    m_info.driverLength    = 0;
    m_info.name            = PACKAGE;
    m_info.tuneInfo        = NULL;
    m_info.version         = VERSION;
    m_info.eventContext    = &context();
    // Number of SIDs support by this library
    m_info.maxsids         = SID2_MAX_SIDS;
    m_info.environment     = sid2_envR;

    // Configure default settings
    m_cfg.clockDefault    = SID2_CLOCK_CORRECT;
    m_cfg.clockForced     = false;
    m_cfg.clockSpeed      = SID2_CLOCK_CORRECT;
    m_cfg.environment     = m_info.environment;
    m_cfg.forceDualSids   = false;
    m_cfg.frequency       = SID2_DEFAULT_SAMPLING_FREQ;
    m_cfg.optimisation    = SID2_DEFAULT_OPTIMISATION;
    m_cfg.playback        = sid2_mono;
    m_cfg.precision       = SID2_DEFAULT_PRECISION;
    m_cfg.sidDefault      = SID2_MODEL_CORRECT;
    m_cfg.sidEmulation    = NULL;
    m_cfg.sidModel        = SID2_MODEL_CORRECT;
    m_cfg.sidSamples      = true;
    m_cfg.leftVolume      = 255;
    m_cfg.rightVolume     = 255;
    m_cfg.sampleFormat    = SID2_LITTLE_SIGNED;

    // Configured by default for Sound Blaster (compatibles)
    if (SID2_DEFAULT_PRECISION == 8)
        m_cfg.sampleFormat = SID2_LITTLE_UNSIGNED;
    config (m_cfg);

    // Get component credits
    credit[0] = PACKAGE " V" VERSION " Engine:\0\tCopyright (C) 2000 Simon White <sidplay2@email.com>\0";
    credit[1] = xsid.credits ();
    credit[2] = "*MOS6510 (CPU) Emulation:\0\tCopyright (C) 2000 Simon White <sidplay2@email.com>\0";
    credit[3] = cia.credits ();
    credit[4] = vic.credits ();
    credit[5] = NULL;
}

Player::~Player() {
    delete m_ram;
    if (m_ram != m_rom) {
        delete m_rom;
    }
}

// Makes the next sequence of notes available.  For sidplay compatibility
// this function should be called from interrupt event
void Player::fakeIRQ (void)
{   // Check to see if the play address has been provided or whether
    // we should pick it up from an IRQ vector
    uint_least16_t playAddr = m_tuneInfo.playAddr;

    // We have to reload the new play address
    if (playAddr)
        evalBankSelect (m_playBank);
    else
    {
        if (isKernal)
        {   // Setup the entry point from hardware IRQ
            playAddr = endian_little16 (&m_ram[0x0314]);
        }
        else
        {   // Setup the entry point from software IRQ
            playAddr = endian_little16 (&m_ram[0xFFFF]);
        }
    }

    // Setup the entry point and restart the cpu
    cpu->triggerIRQ ();
    sid6510.reset   (playAddr, 0, 0, 0);
}

int Player::fastForward (uint percent)
{
    if (percent > 3200)
    {
        m_errorString = "SIDPLAYER ERROR: Percentage value out of range";
        return -1;
    }
    {
        float64_t fastForwardFactor;
        fastForwardFactor   = (float64_t) percent / 100.0;
        // Conversion to fixed point 8.24
        m_samplePeriod      = (event_clock_t) ((float64_t) m_samplePeriod /
                              m_fastForwardFactor * fastForwardFactor);
        m_fastForwardFactor = fastForwardFactor;
    }
    return 0;
}

int Player::initialise ()
{   // Fix the mileage counter if just finished another song.
    mileageCorrect ();
    m_mileage += time ();

    reset ();
    if (psidDrvInstall (m_tuneInfo, m_info) < 0)
        return -1;

    // The Basic ROM sets these values on loading a file.
    {   // Program start address
        uint_least16_t addr = m_tuneInfo.loadAddr;
        endian_little16 (&m_ram[0x2b], addr);
        // Program end address + 1
        addr += m_tuneInfo.c64dataLen;
        endian_little16 (&m_ram[0x2d], addr);
    }

    if (!m_tune->placeSidTuneInC64mem (m_ram))
    {   // Rev 1.6 (saw) - Allow loop through errors
        m_errorString = m_tuneInfo.statusString;
        return -1;
    }

    rtc.reset ();
    envReset  (false);
    return 0;
}

int Player::load (SidTune *tune)
{
    m_tune = tune;
    if (!tune)
    {   // Unload tune
        m_info.tuneInfo = NULL;
        return 0;
    }
    m_info.tuneInfo = &m_tuneInfo;

    // Un-mute all voices
    xsid.mute (false);

    for (int i = 0; i < SID2_MAX_SIDS; i++)
    {
        uint_least8_t v = 3;
        while (v--)
            sid[i]->voice (v, 0, false);
    }

    {   // Must re-configure on fly for stereo support!
        int ret = config (m_cfg);
        // Failed configuration with new tune, reject it
        if (ret < 0)
        {
            m_tune = NULL;
            return -1;
        }
    }
    return 0;
}

void Player::mileageCorrect (void)
{   // If just finished a song, round samples to correct mileage
    if (m_sampleCount >= (m_cfg.frequency / 2))
        m_mileage++;
    m_sampleCount = 0;
}

void Player::pause (void)
{
    if (m_running)
    {
        m_playerState = sid2_paused;
        m_running     = false;
    }
}

uint_least32_t Player::play (void *buffer, uint_least32_t length)
{
    // Make sure a _tune is loaded
    if (!m_tune)
        return 0;

    // Setup Sample Information
    m_sampleIndex  = 0;
    m_sampleCount  = length;
    m_sampleBuffer = (char *) buffer;

    // Start the player loop
    m_playerState = sid2_playing;
    m_running     = true;

    while (m_running)
        m_scheduler.clock ();

    if (m_playerState == sid2_stopped)
        initialise ();
    return m_sampleIndex;
}

void Player::stop (void)
{   // Re-start song
    if (m_tune && (m_playerState != sid2_stopped))
    {
        if (!m_running)
            initialise ();
        else
        {
            m_playerState = sid2_stopped;
            m_running     = false;
        }
    }
}


//-------------------------------------------------------------------------
// Temporary hack till real bank switching code added

//  Input: A 16-bit effective address
// Output: A default bank-select value for $01.
uint8_t Player::iomap (uint_least16_t addr)
{
    if (m_info.environment != sid2_envPS)
    {   // Force Real C64 Compatibility
        if (m_tuneInfo.compatibility == SIDTUNE_COMPATIBILITY_R64)
            return 0;     // Special case, converted to 0x37 later

        if (addr == 0)
            return 0;     // Special case, converted to 0x37 later
        if (addr < 0xa000)
            return 0x37;  // Basic-ROM, Kernal-ROM, I/O
        if (addr  < 0xd000)
            return 0x36;  // Kernal-ROM, I/O
        if (addr >= 0xe000)
            return 0x35;  // I/O only
    }
    return 0x34;  // RAM only (special I/O in PlaySID mode)
}

void Player::evalBankSelect (uint8_t data)
{   // Determine new memory configuration.
    isBasic   = ((data & 3) == 3);
    isIO      = ((data & 7) >  4);
    isKernal  = ((data & 2) != 0);
    m_bankReg = data;
}

uint8_t Player::readMemByte_player (uint_least16_t addr)
{
    if (m_info.environment == sid2_envR)
        return readMemByte_sidplaybs (addr);
    return readMemByte_plain (addr);
}

uint8_t Player::readMemByte_plain (uint_least16_t addr)
{   // Bank Select Register Value DOES NOT get to ram
    if (addr == 0x0001)
        return m_bankReg;
    return m_ram[addr];
}

uint8_t Player::readMemByte_io (uint_least16_t addr)
{
    uint_least16_t tempAddr = (addr & 0xfc1f);

    // Not SID ?
    if (( tempAddr & 0xff00 ) != 0xd400 )
    {
        if (m_info.environment == sid2_envR)
        {
            switch (endian_16hi8 (addr))
            {
            case 0:
                return readMemByte_plain (addr);
            case 0xdc:
                return cia.read (addr&0x0f);
            case 0xdd:
                return cia2.read (addr&0x0f);
            case 0xd0:
                return vic.read (addr&0x3f);
            default:
                return m_rom[addr];
            }
        }
        else
        {
            switch (endian_16hi8 (addr))
            {
            case 0:
                return readMemByte_plain (addr);
            // Sidplay1 Random Extension CIA
            case 0xdc:
                return sid6526.read (addr&0x0f);
            // Sidplay1 Random Extension VIC 
            case 0xd0:
                switch (addr & 0x3f)
                {
                case 0x11:
                case 0x12:
                    return sid6526.read ((addr-13)&0x0f);
                }
                // Deliberate run on
            default:
                return m_rom[addr];
            }
        }
    }

    // Read real sid for these
    if ((addr & 0xff00) == m_sidAddress[1])
        return sid[1]->read ((uint8_t) addr);
    return sid[0]->read ((uint8_t) tempAddr);
}

uint8_t Player::readMemByte_sidplaytp(uint_least16_t addr)
{
    if (addr < 0xD000)
        return readMemByte_plain (addr);
    else
    {
        // Get high-nibble of address.
        switch (addr >> 12)
        {
        case 0xd:
            if (isIO)
                return readMemByte_io (addr);
            else
                return m_ram[addr];
        break;
        case 0xe:
        case 0xf:
        default:  // <-- just to please the compiler
              return m_ram[addr];
        }
    }
}
        
uint8_t Player::readMemByte_sidplaybs (uint_least16_t addr)
{
    if (addr < 0xA000)
        return readMemByte_plain (addr);
    else
    {
        // Get high-nibble of address.
        switch (addr >> 12)
        {
        case 0xa:
        case 0xb:
            if (isBasic)
                return m_rom[addr];
            else
                return m_ram[addr];
        break;
        case 0xc:
            return m_ram[addr];
        break;
        case 0xd:
            if (isIO)
                return readMemByte_io (addr);
            else
                return m_ram[addr];
        break;
        case 0xe:
        case 0xf:
        default:  // <-- just to please the compiler
          if (isKernal)
              return m_rom[addr];
          else
              return m_ram[addr];
        }
    }
}

void Player::writeMemByte_plain (uint_least16_t addr, uint8_t data)
{
    if (addr == 0x0001)
    {   // Determine new memory configuration.
        evalBankSelect (data);
        return;
    }
    m_ram[addr] = data;
}

void Player::writeMemByte_playsid (uint_least16_t addr, uint8_t data)
{
    uint_least16_t tempAddr = (addr & 0xfc1f);

    // Not SID ?
    if (( tempAddr & 0xff00 ) != 0xd400 )
    {
        if (m_info.environment == sid2_envR)
        {
            switch (endian_16hi8 (addr))
            {
            case 0:
                writeMemByte_plain (addr, data);
            return;
            case 0xdc:
                cia.write (addr&0x0f, data);
            return;
            case 0xdd:
                cia2.write (addr&0x0f, data);
            return;
            case 0xd0:
                vic.write (addr&0x3f, data);
            return;
            default:
                m_rom[addr] = data;
            return;
            }
        }
        else
        {
            switch (endian_16hi8 (addr))
            {
            case 0:
                writeMemByte_plain (addr, data);
            return;
            case 0xdc: // Sidplay1 CIA
                sid6526.write (addr&0x0f, data);
            return;
            default:
                m_rom[addr] = data;
            return;
            }
        }
    }

    // $D41D/1E/1F, $D43D/3E/3F, ...
    // Map to real address to support PlaySID
    // Extended SID Chip Registers.
    if (( tempAddr & 0x00ff ) >= 0x001d )
        xsid.write16 (addr & 0x01ff, data);
    else // Mirrored SID.
    {   // SID.
        // Convert address to that acceptable by resid
        // Support dual sid
        if ((addr & 0xff00) == m_sidAddress[1])
        {
            sid[1]->write (addr & 0xff, data);
            // Prevent sid write accessing other sid
            // if not doing mono to stereo conversion.
            if (m_sidAddress[1] != m_sidAddress[0])
                return;
        }
        sid[0]->write (tempAddr & 0xff, data);
    }
}

void Player::writeMemByte_sidplay (uint_least16_t addr, uint8_t data)
{
    if (addr < 0xA000)
        writeMemByte_plain (addr, data);
    else
    {
        // Get high-nibble of address.
        switch (addr >> 12)
        {
        case 0xa:
        case 0xb:
        case 0xc:
            m_ram[addr] = data;
        break;
        case 0xd:
            if (isIO)
                writeMemByte_playsid (addr, data);
            else
                m_ram[addr] = data;
        break;
        case 0xe:
        case 0xf:
        default:  // <-- just to please the compiler
            m_ram[addr] = data;
        }
    }
}

// --------------------------------------------------
// These must be available for use:
void Player::reset (void)
{
    int i;

    m_playerState = sid2_stopped;
    m_running     = false;

    // Select Sidplay1 compatible CPU or real thing
    cpu = &sid6510;
    sid6510.environment (m_info.environment);

    m_scheduler.reset ();
    for (i = 0; i < SID2_MAX_SIDS; i++)
        sid[i]->reset (0x0f);

    if (m_info.environment == sid2_envR)
    {
        cia.reset  ();
        cia2.reset ();
        vic.reset  ();
    }
    else
    {
        sid6526.reset ();
        sid6526.write (0x0e, 1); // Start timer
        if (m_tuneInfo.songSpeed == SIDTUNE_SPEED_VBI)
            sid6526.lock ();
    }

    // Initalise Memory
    memset (m_ram, 0, 0x10000);
    memset (m_rom, 0, 0x10000);
    if (m_info.environment != sid2_envPS)
        memset (m_rom + 0xA000, RTSn, 0x2000);

    if (m_info.environment == sid2_envR)
    {
        memcpy (&m_rom[0xe000], kernal, sizeof (kernal));
        // Since we don't yet run the kernal power up
        // routines, set somethings here.
        endian_little16 (&m_ram[0x028f], 0xEB48); // keyboard poll
        m_rom[0xfd69] = 0x9f; // Bypass memory check
        m_rom[0xe55f] = 0x00; // Bypass screen clear
        endian_little16 (&m_rom[0xa000], 0xA004);
        endian_little16 (&m_rom[0xa002], 0xA004);
        m_rom[0xa004] = JMPw;
        endian_little16 (&m_rom[0xa005], 0xA004);
    }
    else // !sid2_envR
    {
        memset (m_rom + 0xE000, RTSn, 0x2000);    
        // fake VBI-interrupts that do $D019, BMI ...
        m_rom[0x0d019] = 0xff;
        if (m_info.environment == sid2_envPS)
        {
            m_ram[0xff48] = JMPi;
            endian_little16 (&m_ram[0xff49], 0x0314);
        }

        // Software vectors
        endian_little16 (&m_ram[0x0314], 0xEA31); // IRQ
        endian_little16 (&m_ram[0x0316], 0xFE66); // BRK
        endian_little16 (&m_ram[0x0318], 0xFE47); // NMI
        // Hardware vectors
        if (m_info.environment == sid2_envPS)
            endian_little16 (&m_rom[0xfffa], 0xFFFA); // NMI
        else
            endian_little16 (&m_rom[0xfffa], 0xFE43); // NMI
        endian_little16 (&m_rom[0xfffc], 0xFCE2); // RESET
        endian_little16 (&m_rom[0xfffe], 0xFF48); // IRQ
        memcpy (&m_ram[0xfffa], &m_rom[0xfffa], 6);
    }

    // Will get done later if can't now
    if (m_tuneInfo.clockSpeed == SIDTUNE_CLOCK_PAL)
        m_ram[0x02a6] = 1;
    else // SIDTUNE_CLOCK_NTSC
        m_ram[0x02a6] = 0;
}

// This resets the cpu once the program is loaded to begin
// running. Also called when the emulation crashes
void Player::envReset (bool safe)
{
    if (safe)
    {   // Emulation crashed so run in safe mode
        uint8_t prg[] = {LDAb, 0x7f, STAa, 0x0d, 0xdc, RTSn};
        sid2_info_t info;
        SidTuneInfo tuneInfo;
        // Install driver
        tuneInfo.relocStartPage = 0x09;
        tuneInfo.relocPages     = 0x20;
        tuneInfo.initAddr       = 0x0800;
        tuneInfo.songSpeed      = SIDTUNE_SPEED_CIA_1A;
        info.environment        = m_info.environment;
        psidDrvInstall (tuneInfo, info);
        // Install prg
        memcpy (&m_ram[0x0800], prg, sizeof (prg));

        // Make sids silent
        for (int i = 0; i < SID2_MAX_SIDS; i++)
            sid[i]->reset (0);
    }

    m_ram[0] = 0x2F;
    evalBankSelect (0x37);
    // defaults: Basic-ROM on, Kernal-ROM on, I/O on
    if (m_info.environment != sid2_envR)
    {
        uint8_t song = m_tuneInfo.currentSong - 1;
        uint8_t bank = iomap (m_tuneInfo.initAddr);
        if (bank == 0)
            bank = 0x37;
        evalBankSelect (bank);
        m_playBank = iomap (m_tuneInfo.playAddr);
        if (m_info.environment != sid2_envPS)
            sid6510.reset (m_tuneInfo.initAddr, song, 0, 0);
        else
            sid6510.reset (m_tuneInfo.initAddr, song, song, song);
    }
    else
        cpu->reset ();

    mixerReset ();
    xsid.suppress (true);
}

uint8_t Player::envReadMemByte (uint_least16_t addr)
{   // Read from plain only to prevent execution of rom code
    return (this->*(m_readMemByte)) (addr);
}

void Player::envWriteMemByte (uint_least16_t addr, uint8_t data)
{   // Writes must be passed to env version.
    (this->*(m_writeMemByte)) (addr, data);
}

uint8_t Player::envReadMemDataByte (uint_least16_t addr)
{   // Read from plain only to prevent execution of rom code
    return (this->*(m_readMemDataByte)) (addr);
}

bool Player::envCheckBankJump (uint_least16_t addr)
{
    switch (m_info.environment)
    {
    case sid2_envBS:
        if (addr >= 0xA000)
        {
            // Get high-nibble of address.
            switch (addr >> 12)
            {
            case 0xa:
            case 0xb:
                if (isBasic)
                    return false;
            break;

            case 0xc:
            break;

            case 0xd:
                if (isIO)
                    return false;
            break;

            case 0xe:
            case 0xf:
            default:  // <-- just to please the compiler
               if (isKernal)
                    return false;
            break;
            }
        }
    break;

    case sid2_envTP:
        if ((addr >= 0xd000) && isKernal)
            return false;
    break;

    default:
    break;
    }

    return true;
}

SIDPLAY2_NAMESPACE_STOP
