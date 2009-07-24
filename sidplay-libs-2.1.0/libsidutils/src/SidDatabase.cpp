/***************************************************************************
                          dbget.cpp  -  Get time from database
                             -------------------
    begin                : Fri Jun 2 2000
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

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "SidDatabase.h"
#include "MD5/MD5.h"

const char *SidDatabase::ERR_DATABASE_CORRUPT        = "SID DATABASE ERROR: Database seems to be corrupt.";
const char *SidDatabase::ERR_NO_DATABASE_LOADED      = "SID DATABASE ERROR: Songlength database not loaded.";
const char *SidDatabase::ERR_NO_SELECTED_SONG        = "SID DATABASE ERROR: No song selected for retrieving song length.";
const char *SidDatabase::ERR_MEM_ALLOC               = "SID DATABASE ERROR: Memory Allocation Failure.";
const char *SidDatabase::ERR_UNABLE_TO_LOAD_DATABASE = "SID DATABASE ERROR: Unable to load the songlegnth database.";


SidDatabase::~SidDatabase ()
{
    close ();
}


int_least32_t SidDatabase::parseTimeStamp(const char* arg)
{
    /* Read in m:s format at most.
     * Could use a system function if available.
     */
    int_least32_t seconds = 0;
    int  passes    = 2;  // minutes, seconds
    bool gotDigits = false;
    while ( passes-- )
    {
        if ( isdigit(*arg) )
    {
        int t = atoi(arg);
        seconds += t;
        gotDigits = true;
    }
        while ( *arg && isdigit(*arg) )
    {
            ++arg;
        }
        if ( *arg && *arg==':' )
        {
            seconds *= 60;
            ++arg;
        }
    }
    
    // Handle -:-- time stamps and old 0:00 entries which
    // need to be rounded up by one second.
    if ( !gotDigits )
        seconds = 0;
    else if ( seconds==0 )
        ++seconds;
    
    return seconds;
}


uint_least8_t SidDatabase::timesFound (char *str)
{
    /* Try and determine the number of times read back.
     * Used to check validility of times in database.
    */
    uint_least8_t count = 0;
    while (*str)
    {
        if (*str++ == ':')
        count++;
    }
    return count;
}


int SidDatabase::open (const char *filename)
{
    close ();
    // @FIXME@:  Libini should be changed
    database = ini_open ((char *) filename, "r", ";");
    if (!database)
    {
        errorString = ERR_UNABLE_TO_LOAD_DATABASE;
        return -1;
    }

    return 0;
}

void SidDatabase::close ()
{
    if (database)
        ini_close (database);
}

int_least32_t SidDatabase::length (SidTuneMod &tune)
{
    SidTuneInfo    tuneInfo;
    int_least32_t  time = 0;
    char           timeStamp[10];
    MD5            myMD5;
    char           digest[33];
    uint_least16_t selectedSong = 0;

    if (!database)
    {
        errorString = ERR_NO_DATABASE_LOADED;
        return -1;
    }

    tune.getInfo(tuneInfo);
    selectedSong = tuneInfo.currentSong;
    if (!selectedSong)
    {
        errorString = ERR_NO_SELECTED_SONG;
        return -1;
    }

    // Restart fingerprint
    myMD5.reset();
    tune.createMD5(myMD5);
    myMD5.finish();

    // Construct fingerprint.
    digest[0] = '\0';
    for (int di = 0; di < 16; ++di)
        sprintf (digest, "%s%02x", digest, (int) myMD5.getDigest()[di]);


    // Now set up array access
    if (ini_listDelims  (database, " ") == -1)
    {
        errorString = ERR_MEM_ALLOC;
        return -1;
    }

    // Read Time (and check times before hand)
    (void) ini_locateHeading (database, "Database");
    (void) ini_locateKey     (database, digest);
    // If length return is -1 then no entry found in database
    if (ini_dataLength (database) != -1)
    {
        selectedSong = 0;
        while (selectedSong < tuneInfo.currentSong)
        {
            selectedSong++;
            if (ini_readString (database, timeStamp, sizeof (timeStamp)) == -1)
            {   // No time found
                errorString = ERR_DATABASE_CORRUPT;
                return -1;
            }

            // Validate Time
            if (timesFound (timeStamp) != 1)
            {
                errorString = ERR_DATABASE_CORRUPT;
                return -1;
            }
        }

        // Parse timestamp
        time = parseTimeStamp (timeStamp);
    }

    return time;
}
