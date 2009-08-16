/***************************************************************************
                          smm0.h  -  sidusage file support
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

#ifndef _smm0_h_
#define _smm0_h_

#include <string.h>

// IFF IDs
#define BUILD_ID(a, b, c, d) ((uint) a << 24 | \
                              (uint) b << 16 | \
                              (uint) c << 8  | \
                              (uint) d)

#define FORM_ID BUILD_ID('F','O','R','M')
#define SMM0_ID BUILD_ID('S','M','M','0')
#define INF0_ID BUILD_ID('I','N','F','0')
#define ERR0_ID BUILD_ID('E','R','R','0')
#define TIME_ID BUILD_ID('T','I','M','E')
#define MD5_ID  BUILD_ID('M','D','5',' ')
#define BODY_ID BUILD_ID('B','O','D','Y')

// Future Versions
#define SMM1_ID BUILD_ID('S','M','M','1')
#define SMM2_ID BUILD_ID('S','M','M','2')
#define SMM3_ID BUILD_ID('S','M','M','3')
#define SMM4_ID BUILD_ID('S','M','M','4')
#define SMM5_ID BUILD_ID('S','M','M','5')
#define SMM6_ID BUILD_ID('S','M','M','6')
#define SMM7_ID BUILD_ID('S','M','M','7')
#define SMM8_ID BUILD_ID('S','M','M','8')
#define SMM9_ID BUILD_ID('S','M','M','9')

struct Chunk
{
    uint_least32_t length;
};

struct IffHeader: public Chunk
{
    uint8_t type[4]; // Should be SMM0

    IffHeader()
    {
        memset (this, 0, sizeof (IffHeader));
        length = sizeof(IffHeader) - sizeof(Chunk);
    }
};

struct Inf_v0: public Chunk
{
    uint8_t startAddr[2];
    uint8_t stopAddr[2];

    Inf_v0()
    {
        memset (this, 0, sizeof (Inf_v0));
        length = sizeof(Inf_v0) - sizeof(Chunk);
    }
};

struct Err_v0: public Chunk
{
    uint8_t flags[2];

    Err_v0()
    {
        memset (this, 0, sizeof (Err_v0));
        length = sizeof(Err_v0) - sizeof(Chunk);
    }
};

struct Md5: public Chunk
{
    uint8_t key[32];

    Md5()
    {
        memset (this, 0, sizeof (Md5));
        length = sizeof(Md5) - sizeof(Chunk);
    }
};

struct Time: public Chunk
{
    uint8_t stamp[2];

    Time()
    {
        memset (this, 0, sizeof (Time));
        length = sizeof(Time) - sizeof(Chunk);
    }
};

struct Body: public Chunk
{
    struct usage
    {
      uint8_t page;
      uint8_t flags[256];
    } usage[256];

    Body()
    {   // Don't set length as is variable
        memset (this, 0, sizeof (Body));
    }
};

struct Smm_v0
{
    IffHeader header;
    Inf_v0    info;
    Err_v0    error;
    Md5       md5;
    Time      time;
    Body      body;
};

#endif // _smm0_h_
