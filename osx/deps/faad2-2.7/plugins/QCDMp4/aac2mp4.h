/*
** FAAD2 - Freeware Advanced Audio (AAC) Decoder including SBR decoding
** Copyright (C) 2003 M. Bakker, Ahead Software AG, http://www.nero.com
**  
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
** 
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software 
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
**
** Any non-GPL usage of this software or parts of this software is strictly
** forbidden.
**
** Commercial non-GPL licensing of this software is possible.
** For more info contact Ahead Software through Mpeg4AAClicense@nero.com.
**
** $Id: aac2mp4.h,v 1.3 2003/12/06 04:24:17 rjamorim Exp $
**/

#ifndef AAC2MP4_H__
#define AAC2MP4_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

int covert_aac_to_mp4(char *inputFileName, char *mp4FileName);
MP4TrackId AacCreator(MP4FileHandle mp4File, FILE* inFile);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif