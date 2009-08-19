/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009  Alexey Yakovenko

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <mikmod.h>
#include <string.h>
#include "codec.h"
#include "cmod.h"

static char *mikmod_writepointer;
static int mikmod_writesize;
static MODULE *module;

// mikmod fe driver
static BOOL cmod_IsThere(void)
{
    return 1;
}

static BOOL cmod_Init(void)
{
    return VC_Init();
}

static void cmod_Exit(void)
{
    VC_Exit();
}

static void cmod_Update(void)
{
    for (;;) {
        ULONG ret = VC_WriteBytes((SBYTE*)mikmod_writepointer,(ULONG)mikmod_writesize);
        if (ret < mikmod_writesize)
        {
            if (ret <= 0) {
                printf ("WARNING: mikmod VC_WriteBytes returned %d, trying rewind\n", ret);
                Player_SetPosition (0);
                break;
            }
            else {
                mikmod_writesize -= ret;
                mikmod_writepointer += ret;
            }
        }
        else if (ret > mikmod_writesize) {
            printf ("WARNING: mikmod VC_WriteBytes returned %d, trying rewind\n", ret);
            Player_SetPosition (0);
        }
        break;
    }
}

static void cmod_PlayStop(void)
{
    fprintf (stderr, "mikmod called PlayStop\n");
    VC_PlayStop();
}

static MDRIVER cmoddrv={
    NULL,
    (char*)"raboof",
    (char*)"raboof mikmod audio driver v0.1",
    0,255,
    (char*)"cmoddrv",

    NULL,
    cmod_IsThere,
    VC_SampleLoad,
    VC_SampleUnload,
    VC_SampleSpace,
    VC_SampleLength,
    cmod_Init,
    cmod_Exit,
    NULL,
    VC_SetNumVoices,
    VC_PlayStart,
    cmod_PlayStop,
    cmod_Update,
    NULL,
    VC_VoiceSetVolume,
    VC_VoiceGetVolume,
    VC_VoiceSetFrequency,
    VC_VoiceGetFrequency,
    VC_VoiceSetPanning,
    VC_VoiceGetPanning,
    VC_VoicePlay,
    VC_VoiceStop,
    VC_VoiceStopped,
    VC_VoiceGetPosition,
    VC_VoiceRealVolume
};

int
cmod_init (const char *fname, int track, float start, float end) {
//    MikMod_RegisterAllDrivers();
    memset (&cmod.info, 0, sizeof (fileinfo_t));
    MikMod_RegisterAllLoaders();
    MikMod_RegisterDriver (&cmoddrv);
    int drv = MikMod_DriverFromAlias ((char*)"cmoddrv");
    md_device = drv;
    md_mode |= DMODE_HQMIXER;
    MikMod_Init((char*)"");
    module = Player_Load ((char*)fname, 64, 0);
    module->wrap = 1;
    module->loop = 0;
    Player_Start(module);
    Player_SetTempo (1200);
    printf ("module info: songname %s\nmodtype %s\ncomment %s\nnumchannels %d\nnumsamples %d\nsongtime %d\n", module->songname, module->modtype, module->comment, module->numchn, module->numsmp,  module->sngtime);
    printf ("driver info: device %d\nmixfreq: %d\n16bit %d\nhqmixer %d\nstereo %d\n", md_device, md_mixfreq, (md_mode&DMODE_16BITS)?1:0, (md_mode&DMODE_HQMIXER)?1:0, (md_mode&DMODE_STEREO)?1:0);
    cmod.info.bitsPerSample = (md_mode&DMODE_16BITS)? 16 : 8;
    cmod.info.channels = (md_mode&DMODE_STEREO)?2:1;
    cmod.info.samplesPerSecond = md_mixfreq;
    return 0;
}

void
cmod_free (void) {
    Player_SetPosition (0);
    Player_Free(module);
    module = NULL;
    MikMod_Exit();
}

int
cmod_read (char *bytes, int size)
{
    mikmod_writepointer = bytes;
    mikmod_writesize = size;
    if (Player_Active()) {
        MikMod_Update();
    }
    return 0;
}

codec_t cmod = {
    .init = cmod_init,
    .free = cmod_free,
    .read = cmod_read
};

