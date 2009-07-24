/***************************************************************************
                          SidUsage.h  -  sidusage file support
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

#ifndef _SidUsage_h_
#define _SidUsage_h_

#include <sidplay/sidusage.h>
#include <sidplay/SidTune.h>


class SID_EXTERN SidUsage
{
protected:
    bool  m_status;
    const char *m_errorString;

public:
    SidUsage ();

    void           read       (const char *filename, sid_usage_t &usage);
    void           write      (const char *filename, const sid_usage_t &usage,
                               SidTuneInfo &tuneInfo);
    const char *   error      (void) { return m_errorString; }

    operator bool () { return m_status; }
};

#endif // _SidUsage_h_
