/*
 * Adplug - Replayer for many OPL2/OPL3 audio file formats.
 * Copyright (C) 1999 - 2008 Simon Peter, <dn.tlp@gmx.net>, et al.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *
 * MIDI & MIDI-like file player - Last Update: 10/15/2005
 *                  by Phil Hassey - www.imitationpickles.org
 *                                   philhassey@hotmail.com
 *
 * Can play the following
 *      .LAA - a raw save of a Lucas Arts Adlib music
 *             or
 *             a raw save of a LucasFilm Adlib music
 *      .MID - a "midi" save of a Lucas Arts Adlib music
 *           - or general MIDI files
 *      .CMF - Creative Music Format
 *      .SCI - the sierra "midi" format.
 *             Files must be in the form
 *             xxxNAME.sci
 *             So that the loader can load the right patch file:
 *             xxxPATCH.003  (patch.003 must be saved from the
 *                            sierra resource from each game.)
 *
 * 6/2/2000:  v1.0 released by phil hassey
 *      Status:  LAA is almost perfect
 *                      - some volumes are a bit off (instrument too quiet)
 *               MID is fine (who wants to listen to MIDI vid adlib anyway)
 *               CMF is okay (still needs the adlib rhythm mode implemented
 *                            for real)
 * 6/6/2000:
 *      Status:  SCI:  there are two SCI formats, original and advanced.
 *                    original:  (Found in SCI/EGA Sierra Adventures)
 *                               played almost perfectly, I believe
 *                               there is one mistake in the instrument
 *                               loader that causes some sounds to
 *                               not be quite right.  Most sounds are fine.
 *                    advanced:  (Found in SCI/VGA Sierra Adventures)
 *                               These are multi-track files.  (Thus the
 *                               player had to be modified to work with
 *                               them.)  This works fine.
 *                               There are also multiple tunes in each file.
 *                               I think some of them are supposed to be
 *                               played at the same time, but I'm not sure
 *                               when.
 * 8/16/2000:
 *      Status:  LAA: now EGA and VGA lucas games work pretty well
 *
 * 10/15/2005: Changes by Simon Peter
 *	Added rhythm mode support for CMF format.
 *
 * 09/13/2008: Changes by Adam Nielsen (malvineous@shikadi.net)
 *      Fixed a couple of CMF rhythm mode bugs
 *      Disabled note velocity for CMF files
 *      Added support for nonstandard CMF AM+VIB controller (for VGFM CMFs)
 *
 * Other acknowledgements:
 *  Allegro - for the midi instruments and the midi volume table
 *  SCUMM Revisited - for getting the .LAA / .MIDs out of those
 *                    LucasArts files.
 *  FreeSCI - for some information on the sci music files
 *  SD - the SCI Decoder (to get all .sci out of the Sierra files)
 */

#include "strnlen.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "mid.h"
#include "mididata.h"
#include "load_helper.h"

/*#define TESTING*/
#ifdef TESTING
#define midiprintf printf
#else
void CmidPlayer::midiprintf(const char *format, ...)
    {
    }
#endif

#include <stdint.h> // for uintxx_t

#define LUCAS_STYLE   1
#define CMF_STYLE     2
#define MIDI_STYLE    4
#define SIERRA_STYLE  8

// AdLib melodic and rhythm mode defines
#define ADLIB_MELODIC	0
#define ADLIB_RHYTHM	1

// File types
#define FILE_LUCAS      1
#define FILE_MIDI       2
#define FILE_CMF        3
#define FILE_SIERRA     4
#define FILE_ADVSIERRA  5
#define FILE_OLDLUCAS   6

// AdLib standard operator table
const unsigned char CmidPlayer::adlib_opadd[] = {0x00  ,0x01 ,0x02  ,0x08  ,0x09  ,0x0A  ,0x10 ,0x11  ,0x12};

// map CMF drum channels 12 - 15 to corresponding AdLib drum operators
// bass drum (channel 11) not mapped, cause it's handled like a normal instrument
const int CmidPlayer::map_chan[] = { 0x14, 0x12, 0x15, 0x11 };

// Standard AdLib frequency table
const int CmidPlayer::fnums[] = { 0x16b,0x181,0x198,0x1b0,0x1ca,0x1e5,0x202,0x220,0x241,0x263,0x287,0x2ae };

// Map CMF drum channels 11 - 15 to corresponding AdLib drum channels
const int CmidPlayer::percussion_map[] = { 6, 7, 8, 8, 7 };

CPlayer *CmidPlayer::factory(Copl *newopl)
{
  return new CmidPlayer(newopl);
}

CmidPlayer::CmidPlayer(Copl *newopl)
  : CPlayer(newopl), author(&emptystr), title(&emptystr), remarks(&emptystr),
    emptystr('\0'), flen(0), data(0)
{
}

unsigned char CmidPlayer::datalook(unsigned long pos)
{
    if (pos >= flen) return(0);
    return(data[pos]);
}

void CmidPlayer::readString(char *dst, unsigned long num)
{
	unsigned long i;

    for (i=0; i<num; i++)
        {
        dst[i]=(char)datalook(pos); pos++;
        }
}

unsigned long CmidPlayer::getnexti(unsigned long num)
{
	unsigned long v=0;
	unsigned long i;

    for (i=0; i<num; i++)
        {
        v+=(datalook(pos)<<(8*i)); pos++;
        }
    return(v);
}

unsigned long CmidPlayer::getnext(unsigned long num)
{
	unsigned long v=0;
	unsigned long i;

    for (i=0; i<num; i++)
        {
        v<<=8;
        v+=datalook(pos); pos++;
        }
    return(v);
}

unsigned long CmidPlayer::getval()
{
	unsigned long b, v = 0;

	do {
		b = getnext(1);
		v = (v << 7) + (b & 0x7f);
	} while (b & 0x80);
	return v & 0x0fffffff; // limit value to allowed range
}

bool CmidPlayer::load_sierra_ins(const std::string &fname, const CFileProvider &fp)
{
    long i,j,k,l,fl;
    unsigned char ins[28];
    char *pfilename;
    binistream *f;

    fl = fname.length()+10;
    pfilename = (char *)malloc(fl);
    strcpy(pfilename,fname.c_str());
    j=0;
    for(i=strlen(pfilename)-1; i >= 0; i--)
      if(pfilename[i] == '/' || pfilename[i] == '\\') {
	j = i+1;
	break;
      }
    for (i = 0; i < 3; i++)
      if (pfilename[j]) j++;
    snprintf(pfilename+j,fl-j,"patch.003");

    f = fp.open(pfilename);
    free(pfilename);
    if(!f) return false;

    f->ignore(2);
    stins = 0;
    for (i=0; i<2; i++)
        {
        for (k=0; k<48; k++)
            {
            l=i*48+k;
            midiprintf ("\n%2ld: ",l);
            for (j=0; j<28; j++)
                ins[j] = f->readInt(1);

            myinsbank[l][0]=
                (ins[9]*0x80) + (ins[10]*0x40) +
                (ins[5]*0x20) + (ins[11]*0x10) +
                ins[1];   //1=ins5
            myinsbank[l][1]=
                (ins[22]*0x80) + (ins[23]*0x40) +
                (ins[18]*0x20) + (ins[24]*0x10) +
                ins[14];  //1=ins18

            myinsbank[l][2]=(ins[0]<<6)+ins[8];
            myinsbank[l][3]=(ins[13]<<6)+ins[21];

            myinsbank[l][4]=(ins[3]<<4)+ins[6];
            myinsbank[l][5]=(ins[16]<<4)+ins[19];
            myinsbank[l][6]=(ins[4]<<4)+ins[7];
            myinsbank[l][7]=(ins[17]<<4)+ins[20];

            myinsbank[l][8]=ins[26];
            myinsbank[l][9]=ins[27];

            myinsbank[l][10]=((ins[2]<<1))+(1-(ins[12]&1));
            //(ins[12] ? 0:1)+((ins[2]<<1));

            for (j=0; j<11; j++)
                midiprintf ("%02X ",myinsbank[l][j]);
			stins++;
            }
		f->ignore(2);
        }

    fp.close(f);
    memcpy(smyinsbank, myinsbank, 128 * 16);
    return true;
}

void CmidPlayer::sierra_next_section()
{
    int i,j;

    for (i=0; i<16; i++)
        track[i].on=0;

    midiprintf("\n\nnext adv sierra section:\n");

    pos=sierra_pos;
    i=0;j=0;
    while (i!=0xff)
       {
       getnext(1);
       curtrack=j; j++;
       if (curtrack >= 16)
         break;
       track[curtrack].on=1;
	   track[curtrack].spos = getnext(1);
	   track[curtrack].spos += (getnext(1) << 8) + 4;	//4 best usually +3? not 0,1,2 or 5
//       track[curtrack].spos=getnext(1)+(getnext(1)<<8)+4;		// dynamite!: doesn't optimize correctly!!
       track[curtrack].tend=flen; //0xFC will kill it
       track[curtrack].iwait=0;
       track[curtrack].pv=0;
       midiprintf ("track %u starts at %lx\n",curtrack,track[curtrack].spos);

       getnext(2);
       i=getnext(1);
       }
    getnext(2);
    deltas=0x20;
    sierra_pos=pos;
    //getch();

    fwait=0;
    doing=1;
}

bool CmidPlayer::load(const std::string &filename, const CFileProvider &fp)
{
    binistream *f = fp.open(filename); if(!f) return false;
    int good;
    unsigned char s[6];
    uint32_t size;

    f->readString((char *)s, 6);
    size = u32_unaligned(s); // size of FILE_OLDLUCAS
    good=0;
    subsongs=0;
    switch(s[0])
        {
        case 'A':
            if (s[1]=='D' && s[2]=='L') good=FILE_LUCAS;
            break;
        case 'M':
            if (s[1]=='T' && s[2]=='h' && s[3]=='d')
            {
                /* "MThd"              header-chunk
                 * 0x00 0x00 0x00 0x06 header size
                 * 0x00 0x0n           midi file type: 0=single-track format, 1=multiple-track format, 2=multiple-song format
                 * 0xnn 0xnn           number of tracks
                 * 0xnn 0xnn           tempo
                 */
                f->seek(-2, binio::Add); // we have already read 6 bytes from the start
                f->setFlag(binio::BigEndian, true);
                if (f->readInt(4) != 6)
                    break;
                midi_type=f->readInt(2);
                if (midi_type > 2 || f->readInt(2) < 1)
                    break;
                good=FILE_MIDI;
                midiprintf ("General MIDI type: %d\n", midi_type);
            }
            break;
        case 'C':
            if (s[1]=='T' && s[2]=='M' && s[3]=='F') good=FILE_CMF;
            break;
        case 0x84:
	  if (s[1]==0x00 && load_sierra_ins(filename, fp)) {
	    if (s[2]==0xf0)
	      good=FILE_ADVSIERRA;
	    else
	      good=FILE_SIERRA;
	  }
	  break;
        default:
            if (size == fp.filesize(f) && s[4]=='A' && s[5]=='D') good=FILE_OLDLUCAS;
            break;
        }

    if (good!=0)
		subsongs=1;
    else {
      fp.close(f);
      return false;
    }

    type=good;
    f->seek(0);
    flen = fp.filesize(f);
    data = new unsigned char [flen];
    f->readString((char *)data, flen);

    fp.close(f);
    rewind(0);
    return true;
}

void CmidPlayer::midi_write_adlib(unsigned int r, unsigned char v)
{
  opl->write(r,v);
  adlib_data[r]=v;
}

void CmidPlayer::midi_fm_instrument(int voice, unsigned char *inst)
{
    if ((adlib_style&SIERRA_STYLE)!=0)
        midi_write_adlib(0xbd,0);  //just gotta make sure this happens..
                                      //'cause who knows when it'll be
                                      //reset otherwise.


    midi_write_adlib(0x20+adlib_opadd[voice],inst[0]);
    midi_write_adlib(0x23+adlib_opadd[voice],inst[1]);

    if (adlib_style & LUCAS_STYLE) {
        midi_write_adlib(0x43+adlib_opadd[voice],0x3f);
        if ((inst[10] & 1)==0)
            midi_write_adlib(0x40+adlib_opadd[voice],inst[2]);
        else
            midi_write_adlib(0x40+adlib_opadd[voice],0x3f);

    } else if ((adlib_style & SIERRA_STYLE) || (adlib_style & CMF_STYLE)) {
        midi_write_adlib(0x40+adlib_opadd[voice],inst[2]);
        midi_write_adlib(0x43+adlib_opadd[voice],inst[3]);

    } else {
        midi_write_adlib(0x40+adlib_opadd[voice],inst[2]);
        if ((inst[10] & 1)==0)
            midi_write_adlib(0x43+adlib_opadd[voice],inst[3]);
        else
            midi_write_adlib(0x43+adlib_opadd[voice],0);
    }

    midi_write_adlib(0x60+adlib_opadd[voice],inst[4]);
    midi_write_adlib(0x63+adlib_opadd[voice],inst[5]);
    midi_write_adlib(0x80+adlib_opadd[voice],inst[6]);
    midi_write_adlib(0x83+adlib_opadd[voice],inst[7]);
    midi_write_adlib(0xe0+adlib_opadd[voice],inst[8]);
    midi_write_adlib(0xe3+adlib_opadd[voice],inst[9]);

    midi_write_adlib(0xc0+voice,inst[10]);
}

void CmidPlayer::midi_fm_percussion(int ch, unsigned char *inst)
{
  int	opadd = map_chan[ch - 12];

  midi_write_adlib(0x20 + opadd, inst[0]);
  midi_write_adlib(0x40 + opadd, inst[2]);
  midi_write_adlib(0x60 + opadd, inst[4]);
  midi_write_adlib(0x80 + opadd, inst[6]);
  midi_write_adlib(0xe0 + opadd, inst[8]);
  if (opadd < 0x13) // only output this for the modulator, not the carrier, as it affects the entire channel
      midi_write_adlib(0xc0 + percussion_map[ch - 11], inst[10]);
}

void CmidPlayer::midi_fm_volume(int voice, int volume)
{
    int vol;

    if ((adlib_style&SIERRA_STYLE)==0)  //sierra likes it loud!
    {
    vol=volume>>2;

    if ((adlib_style&LUCAS_STYLE)!=0)
        {
        if ((adlib_data[0xc0+voice]&1)==1)
            midi_write_adlib(0x40+adlib_opadd[voice], (unsigned char)((63-vol) |
            (adlib_data[0x40+adlib_opadd[voice]]&0xc0)));
        midi_write_adlib(0x43+adlib_opadd[voice], (unsigned char)((63-vol) |
            (adlib_data[0x43+adlib_opadd[voice]]&0xc0)));
        }
        else
        {
        if ((adlib_data[0xc0+voice]&1)==1)
            midi_write_adlib(0x40+adlib_opadd[voice], (unsigned char)((63-vol) |
            (adlib_data[0x40+adlib_opadd[voice]]&0xc0)));
        midi_write_adlib(0x43+adlib_opadd[voice], (unsigned char)((63-vol) |
           (adlib_data[0x43+adlib_opadd[voice]]&0xc0)));
        }
    }
}

void CmidPlayer::midi_fm_playnote(int voice, int note, int volume)
{
    if (note < 0) return; // prevent invalid access
    int freq=fnums[note%12];
    int oct=note/12;
	int c;

    midi_fm_volume(voice,volume);
    midi_write_adlib(0xa0+voice,(unsigned char)(freq&0xff));

	c=((freq&0x300) >> 8)+((oct&7)<<2) + (adlib_mode == ADLIB_MELODIC || voice < 6 ? (1<<5) : 0);
    midi_write_adlib(0xb0+voice,(unsigned char)c);
}

void CmidPlayer::midi_fm_endnote(int voice)
{
    //midi_fm_volume(voice,0);
    //midi_write_adlib(0xb0+voice,0);

    midi_write_adlib(0xb0+voice,(unsigned char)(adlib_data[0xb0+voice]&(255-32)));
}

void CmidPlayer::midi_fm_reset()
{
    int i;

    opl->init();

    for (i=0; i<256; i++)
        midi_write_adlib(i,0);

    midi_write_adlib(0x01, 0x20);
    midi_write_adlib(0xBD,0xc0);
}

bool CmidPlayer::update()
{
    long w,v,note,vel,ctrl,nv,x,l,lnum;
    int i=0,j,c;
    int on,onl,numchan;
    int ret;

    if (doing == 1)
        {
        // just get the first wait and ignore it :>
        for (curtrack=0; curtrack<16; curtrack++)
            if (track[curtrack].on)
                {
                pos=track[curtrack].pos;
                if (type != FILE_SIERRA && type !=FILE_ADVSIERRA)
                    track[curtrack].iwait+=getval();
                    else
                    track[curtrack].iwait+=getnext(1);
                track[curtrack].pos=pos;
                }
        doing=0;
        }

    iwait=0;
    ret=1;

    while (iwait==0 && ret==1)
        {
        for (curtrack=0; curtrack<16; curtrack++)
        if (track[curtrack].on && track[curtrack].iwait==0 &&
            track[curtrack].pos < track[curtrack].tend)
        {
        pos=track[curtrack].pos;

		v=getnext(1);

        //  This is to do implied MIDI events.
        if (v<0x80) {v=track[curtrack].pv; pos--;}
        track[curtrack].pv=(unsigned char)v;

		c=v&0x0f;
        midiprintf ("[%2lX]",v);
        switch(v&0xf0)
            {
			case 0x80: /*note off*/
				note=getnext(1); vel=getnext(1);
                for (i=0; i<9; i++)
                    if (chp[i][0]==c && chp[i][1]==note)
                        {
                        midi_fm_endnote(i);
                        chp[i][0]=-1;
                        }
                break;
            case 0x90: /*note on*/
              //  doing=0;
                note=getnext(1); vel=getnext(1);

		if(adlib_mode == ADLIB_RHYTHM)
		  numchan = 6;
		else
		  numchan = 9;

                if (ch[c].on!=0)
                {
		  for (i=0; i<18; i++)
                    chp[i][2]++;

		  if(c < 11 || adlib_mode == ADLIB_MELODIC) {
		    j=0;
		    on=-1;onl=0;
		    for (i=0; i<numchan; i++)
		      if (chp[i][0]==-1 && chp[i][2]>onl)
			{ onl=chp[i][2]; on=i; j=1; }

		    if (on==-1)
		      {
			onl=0;
			for (i=0; i<numchan; i++)
			  if (chp[i][2]>onl)
			    { onl=chp[i][2]; on=i; }
		      }

		    if (j==0)
		      midi_fm_endnote(on);
		  } else
		    on = percussion_map[c - 11];

                  if (vel!=0 && ch[c].inum>=0 && ch[c].inum<128) {
                    if (adlib_mode == ADLIB_MELODIC || c < 12) // 11 == bass drum, handled like a normal instrument, on == channel 6 thanks to percussion_map[] above
		      midi_fm_instrument(on,ch[c].ins);
		    else
 		      midi_fm_percussion(c, ch[c].ins);

                    if (adlib_style & MIDI_STYLE) {
                        nv=((ch[c].vol*vel)/128);
                        if ((adlib_style&LUCAS_STYLE)!=0) nv*=2;
                        if (nv>127) nv=127;
                        nv=my_midi_fm_vol_table[nv];
                        if ((adlib_style&LUCAS_STYLE)!=0)
                            nv=(int)((float)sqrt((float)nv)*11);
                    } else if (adlib_style & CMF_STYLE) {
                        // CMF doesn't support note velocity (even though some files have them!)
                        nv = 127;
                    } else {
                        nv=vel;
                    }

		    midi_fm_playnote(on,note+ch[c].nshift,nv*2); // sets freq in rhythm mode
                    chp[on][0]=c;
                    chp[on][1]=note;
                    chp[on][2]=0;

		    if(adlib_mode == ADLIB_RHYTHM && c >= 11) {
		      // Still need to turn off the perc instrument before playing it again,
		      // as not all songs send a noteoff.
		      midi_write_adlib(0xbd, adlib_data[0xbd] & ~(0x10 >> (c - 11)));
		      // Play the perc instrument
		      midi_write_adlib(0xbd, adlib_data[0xbd] | (0x10 >> (c - 11)));
		    }

                  } else {
                    if (vel==0) { //same code as end note
		        if (adlib_mode == ADLIB_RHYTHM && c >= 11) {
		            // Turn off the percussion instrument
		            midi_write_adlib(0xbd, adlib_data[0xbd] & ~(0x10 >> (c - 11)));
                            //midi_fm_endnote(percussion_map[c]);
                            chp[percussion_map[c - 11]][0]=-1;
                        } else {
                            for (i=0; i<9; i++) {
                                if (chp[i][0]==c && chp[i][1]==note) {
                                    // midi_fm_volume(i,0);  // really end the note
                                    midi_fm_endnote(i);
                                    chp[i][0]=-1;
                                }
                            }
                        }
                    } else {
                        // i forget what this is for.
                        chp[on][0]=-1;
                        chp[on][2]=0;
                    }
                  }
                  midiprintf(" [%d:%d:%ld:%ld]\n",c,ch[c].inum,note,vel);
                }
                else
                midiprintf ("off");
                break;
            case 0xa0: /*key after touch */
                note=getnext(1); vel=getnext(1);
                /*  //this might all be good
                for (i=0; i<9; i++)
                    if (chp[i][0]==c & chp[i][1]==note)
                        
midi_fm_playnote(i,note+cnote[c],my_midi_fm_vol_table[(cvols[c]*vel)/128]*2);
                */
                break;
            case 0xb0: /*control change .. pitch bend? */
                ctrl=getnext(1); vel=getnext(1);

                switch(ctrl)
                    {
                    case 0x07:
                        midiprintf ("(pb:%d: %ld %ld)",c,ctrl,vel);
                        ch[c].vol=vel;
                        midiprintf("vol");
                        break;
                    case 0x63:
                        if (adlib_style & CMF_STYLE) {
                            // Custom extension to allow CMF files to switch the
                            // AM+VIB depth on and off (officially this is on,
                            // and there's no way to switch it off.)  Controller
                            // values:
                            //   0 == AM+VIB off
                            //   1 == VIB on
                            //   2 == AM on
                            //   3 == AM+VIB on
                            midi_write_adlib(0xbd, (adlib_data[0xbd] & ~0xC0) | (vel << 6));
                            midiprintf(" AM+VIB depth change - AM %s, VIB %s\n",
                                (adlib_data[0xbd] & 0x80) ? "on" : "off",
                                (adlib_data[0xbd] & 0x40) ? "on" : "off"
                            );
                        }
                        break;
                    case 0x67:
                        midiprintf("Rhythm mode: %ld\n", vel);
                        if ((adlib_style&CMF_STYLE)!=0) {
			  adlib_mode=vel;
			  if(adlib_mode == ADLIB_RHYTHM)
			    midi_write_adlib(0xbd, adlib_data[0xbd] | (1 << 5));
			  else
			    midi_write_adlib(0xbd, adlib_data[0xbd] & ~(1 << 5));
			}
                        break;
                    }
                break;
            case 0xc0: /*patch change*/
	      x=getnext(1);
	      ch[c].inum = x & 0x7f;
	      for (j=0; j<11; j++)
		ch[c].ins[j]=myinsbank[ch[c].inum][j];
	      break;
            case 0xd0: /*channel touch*/
                x=getnext(1);
                break;
            case 0xe0: /*pitch wheel*/
                x=getnext(1);
                x=getnext(1);
                break;
            case 0xf0:
                switch(v)
                    {
                    case 0xf0:
                    case 0xf7: /*sysex*/
		      l=getval();
		      if (datalook(pos+l)==0xf7)
			i=1;
		      midiprintf("{%ld}",l);
		      midiprintf("\n");

                        if (datalook(pos)==0x7d &&
                            datalook(pos+1)==0x10 &&
                            datalook(pos+2)<16)
							{
                            adlib_style=LUCAS_STYLE|MIDI_STYLE;
							for (i=0; i<l; i++)
								{
                                midiprintf ("%x ",datalook(pos+i));
                                if ((i-3)%10 == 0) midiprintf("\n");
								}
                            midiprintf ("\n");
                            getnext(1);
                            getnext(1);
                            c = getnext(1) & 0x0f;
							getnext(1);

                          //  getnext(22); //temp
                            unsigned char tmp;
                            tmp = getnext(1) << 4;
                            ch[c].ins[0] = tmp + getnext(1);
                            tmp = getnext(1) << 4;
                            ch[c].ins[2] = 0xff - ((tmp + getnext(1)) & 0x3f);
                            tmp = getnext(1) << 4;
                            ch[c].ins[4] = 0xff - (tmp + getnext(1));
                            tmp = getnext(1) << 4;
                            ch[c].ins[6] = 0xff-(tmp + getnext(1));
                            tmp = getnext(1) << 4;
                            ch[c].ins[8] = tmp + getnext(1);
                            tmp = getnext(1) << 4;
                            ch[c].ins[1] = tmp + getnext(1);
                            tmp = getnext(1) << 4;
                            ch[c].ins[3] = 0xff - ((tmp + getnext(1)) & 0x3f);
                            tmp = getnext(1) << 4;
                            ch[c].ins[5] = 0xff - (tmp + getnext(1));
                            tmp = getnext(1) << 4;
                            ch[c].ins[7] = 0xff - (tmp + getnext(1));
                            tmp = getnext(1) << 4;
                            ch[c].ins[9] = tmp + getnext(1);

                            i = getnext(1) << 4;
                            ch[c].ins[10] = (i += getnext(1));

                            //if ((i&1)==1) ch[c].ins[10]=1;

                            midiprintf ("\n%d: ",c);
							for (i=0; i<11; i++)
                                midiprintf ("%2X ",ch[c].ins[i]);
                            getnext(l-26);
							}
                            else
                            {
                            midiprintf("\n");
                            for (j=0; j<l; j++)
                                midiprintf ("%2lX ",getnext(1));
                            }

                        midiprintf("\n");
						if(i==1)
							getnext(1);
                        break;
                    case 0xf1:
                        break;
                    case 0xf2:
                        getnext(2);
                        break;
                    case 0xf3:
                        getnext(1);
                        break;
                    case 0xf4:
                        break;
                    case 0xf5:
                        break;
                    case 0xf6: /*something*/
                    case 0xf8:
                    case 0xfa:
                    case 0xfb:
                    case 0xfc:
                        //this ends the track for sierra.
                        if (type == FILE_SIERRA ||
                            type == FILE_ADVSIERRA)
                            {
                            track[curtrack].tend=pos;
                            midiprintf ("endmark: %lu -- %lx\n",pos,pos);
                            }
                        break;
                    case 0xfe:
                        break;
                    case 0xfd:
                        break;
                    case 0xff:
                        v=getnext(1);
                        l=getval();
                        midiprintf ("\n");
                        midiprintf("{%lX_%lX}",v,l);
                        if (v==0x51)
                            {
                            lnum=getnext(l);
                            msqtr=lnum; /*set tempo*/
                            midiprintf ("(qtr=%ld)",msqtr);
                            }
                            else
                            {
                            for (i=0; i<l; i++)
                                midiprintf ("%2lX ",getnext(1));
                            }
                        break;
					}
                break;
            default: midiprintf("%lX!",v); /* if we get down here, a error occurred */
			break;
            }

        if (pos < track[curtrack].tend)
            {
            if (type != FILE_SIERRA && type !=FILE_ADVSIERRA)
                w=getval();
                else
                w=getnext(1);
            track[curtrack].iwait=w;
            /*
            if (w!=0)
                {
                midiprintf("\n<%d>",w);
                f = 
((float)w/(float)deltas)*((float)msqtr/(float)1000000);
                if (doing==1) f=0; //not playing yet. don't wait yet
                }
                */
            }
            else
            track[curtrack].iwait=0;

        track[curtrack].pos=pos;
        }


        ret=0; //end of song.
        iwait=0;
        for (curtrack=0; curtrack<16; curtrack++)
            if (track[curtrack].on == 1 &&
                track[curtrack].pos < track[curtrack].tend)
                ret=1;  //not yet..

        if (ret==1)
            {
            iwait = ~0UL;  // bigger than any wait can be!
            for (curtrack=0; curtrack<16; curtrack++)
               if (track[curtrack].on == 1 &&
                   track[curtrack].pos < track[curtrack].tend &&
                   track[curtrack].iwait < iwait)
                   iwait=track[curtrack].iwait;
            }
        }


    if (iwait !=0 && ret==1)
        {
        for (curtrack=0; curtrack<16; curtrack++)
            if (track[curtrack].on)
                track[curtrack].iwait-=iwait;

        
fwait=1.0f/(((float)iwait/(float)deltas)*((float)msqtr/(float)1000000));
        }
        else
        fwait=50;  // 1/50th of a second

    midiprintf ("\n");
    for (i=0; i<16; i++)
      if (track[i].on) {
	if (track[i].pos < track[i].tend)
	  midiprintf ("<%lu>",track[i].iwait);
	else
	  midiprintf("stop");
      }

    /*
    if (ret==0 && type==FILE_ADVSIERRA)
        if (datalook(sierra_pos-2)!=0xff)
            {
            midiprintf ("next sectoin!");
            sierra_next_section(p);
            fwait=50;
            ret=1;
            }
    */

	if(ret)
		return true;
	else
		return false;
}

float CmidPlayer::getrefresh()
{
    return (fwait > 0.01f ? fwait : 0.01f);
}

void CmidPlayer::rewind(int subsong)
{
    long i,j,n,m,l;
    long o_sierra_pos;
    unsigned char ins[16];

    pos=0; tins=0;
    adlib_style=MIDI_STYLE|CMF_STYLE;
    adlib_mode=ADLIB_MELODIC;
    for (i=0; i<128; i++)
        for (j=0; j<14; j++)
            myinsbank[i][j]=midi_fm_instruments[i][j];
    for (i=0; i<16; i++)
        {
        ch[i].inum=0;
        for (j=0; j<11; j++)
            ch[i].ins[j]=myinsbank[ch[i].inum][j];
        ch[i].vol=127;
        ch[i].nshift=-25;
        ch[i].on=1;
        }

    /* General init */
    for (i=0; i<9; i++)
        {
        chp[i][0]=-1;
        chp[i][2]=0;
        }

    deltas=250;  // just a number,  not a standard
    msqtr=500000;
    fwait=123; // gotta be a small thing.. sorta like nothing
    iwait=0;

    subsongs=1;

    for (i=0; i<16; i++)
        {
        track[i].tend=0;
        track[i].spos=0;
        track[i].pos=0;
        track[i].iwait=0;
        track[i].on=0;
        track[i].pv=0;
        }
    curtrack=0;

    /* specific to file-type init */

        pos=0;
        i=getnext(1);
        switch(type)
            {
            case FILE_LUCAS:
                getnext(24);  //skip junk and get to the midi.
                adlib_style=LUCAS_STYLE|MIDI_STYLE;
                //note: no break, we go right into midi headers...
            case FILE_MIDI:
                if (type != FILE_LUCAS)
                    tins=128;
                getnext(11); /* skip past header data until deltas is reached */
                deltas=getnext(2);
                midiprintf ("deltas:%ld\n",deltas);

                /* MIDI files should be played back one octave and a semi-note off in to be played back correctly */
                if (type != FILE_LUCAS)
                {
                    for (i=0; i<16; i++)
                    {
                        ch[i].nshift=-13;
                    }
                }

                curtrack=0;
                while ((curtrack == 0) ||
                       ((midi_type == 1) && (curtrack < 16)))
                {
                    /* MIDI type 0 (and LucasArts AdLib MIDI) stores all the MIDI channels in a single track,
                     * while MIDI type 1 splits each channel into separate tracks */
                    int len;
                    char s[5];
                    readString(s, 4);
                    s[4] = 0;
                    midiprintf ("Offset=0x%08lx\n", pos);
                    midiprintf ("trkHeader: '%s'\n", s);
                    if (strcmp(s, "MTrk"))
                        break;
                    track[curtrack].on=1;
                    len=getnext(4);
                    midiprintf ("tracklen:%lu\n",len);
                    track[curtrack].tend = pos + len;
                    if (track[curtrack].tend > flen) // no music after end of file
                        track[curtrack].tend = flen;
                    track[curtrack].spos=pos;
                    pos+=len;
                    curtrack++;
                }
                break;
            case FILE_CMF:
                getnext(3);  // ctmf
                getnexti(2); //version
                n=getnexti(2); // instrument offset
                m=getnexti(2); // music offset
                deltas=getnexti(2); //ticks/qtr note
                i = getnexti(2); // stuff in cmf is click ticks per second..
                if (i) msqtr = 1000000L / i * deltas;

                i=getnexti(2);
                if (i > 0 && (unsigned long)i < flen &&
                    strnlen((char *)data + i, flen - i) < flen - i)
                  title = (char *)data + i;
                i=getnexti(2);
                if (i > 0 && (unsigned long)i < flen &&
                    strnlen((char *)data + i, flen - i) < flen - i)
                  author = (char *)data + i;
                i=getnexti(2);
                if (i > 0 && (unsigned long)i < flen &&
                    strnlen((char *)data + i, flen - i) < flen - i)
                  remarks = (char *)data + i;

                getnext(16); // channel in use table ..
                i=getnexti(2); // num instr
                if (i>128) i=128; // to ward of bad numbers...
                getnexti(2); //basic tempo

                midiprintf("\nioff:%ld\nmoff%ld\ndeltas:%ld\nmsqtr:%ld\nnumi:%ld\n",
                    n,m,deltas,msqtr,i);
                pos=n;  // jump to instruments
                tins=i;
                for (j=0; j<i; j++)
                    {
                    midiprintf ("\n%ld: ",j);
                    for (l=0; l<16; l++)
                        {
                        myinsbank[j][l]=(unsigned char)getnext(1);
                        midiprintf ("%2X ",myinsbank[j][l]);
                        }
                    }

                for (i=0; i<16; i++)
                    ch[i].nshift=-13;

                adlib_style=CMF_STYLE;

                curtrack=0;
                track[curtrack].on=1;
                track[curtrack].tend=flen;  // music until the end of the file
                track[curtrack].spos=m;  //jump to midi music
                break;
            case FILE_OLDLUCAS:
                msqtr=250000;
                pos=9;
                deltas=getnext(1);

                i=8;
                pos=0x19;  // jump to instruments
                tins=i;
                for (j=0; j<i; j++)
                    {
                    midiprintf ("\n%ld: ",j);
                    for (l=0; l<16; l++)
                        ins[l]=(unsigned char)getnext(1);

                    myinsbank[j][10]=ins[2];
                    myinsbank[j][0]=ins[3];
                    myinsbank[j][2]=ins[4];
                    myinsbank[j][4]=ins[5];
                    myinsbank[j][6]=ins[6];
                    myinsbank[j][8]=ins[7];
                    myinsbank[j][1]=ins[8];
                    myinsbank[j][3]=ins[9];
                    myinsbank[j][5]=ins[10];
                    myinsbank[j][7]=ins[11];
                    myinsbank[j][9]=ins[12];

                    for (l=0; l<11; l++)
                        midiprintf ("%2X ",myinsbank[j][l]);
                    }

                for (i=0; i<16; i++)
                    {
                    if (i<tins)
                        {
                        ch[i].inum=i;
                        for (j=0; j<11; j++)
                            ch[i].ins[j]=myinsbank[ch[i].inum][j];
                        }
                    }

                adlib_style=LUCAS_STYLE|MIDI_STYLE;

                curtrack=0;
                track[curtrack].on=1;
                track[curtrack].tend=flen;  // music until the end of the file
                track[curtrack].spos=0x98;  //jump to midi music
                break;
            case FILE_ADVSIERRA:
	      memcpy(myinsbank, smyinsbank, 128 * 16);
	      tins = stins;
                deltas=0x20;
                getnext(11); //worthless empty space and "stuff" :)

                o_sierra_pos=sierra_pos=pos;
                sierra_next_section();
                while (datalook(sierra_pos-2) != 0xff && pos < flen)
                    {
                    sierra_next_section();
                    subsongs++;
                    }

                if (subsong < 0 || subsong >= subsongs) subsong=0;

                sierra_pos=o_sierra_pos;
                sierra_next_section();
                i=0;
                while (i != subsong)
                    {
                    sierra_next_section();
                    i++;
                    }

                adlib_style=SIERRA_STYLE|MIDI_STYLE;  //advanced sierra tunes use volume
                break;
            case FILE_SIERRA:
	      memcpy(myinsbank, smyinsbank, 128 * 16);
	      tins = stins;
                getnext(2);
                deltas=0x20;

                curtrack=0;
                track[curtrack].on=1;
                track[curtrack].tend=flen;  // music until the end of the file

                for (i=0; i<16; i++)
                    {
                    ch[i].nshift=-13;
                    ch[i].on=getnext(1);
                    ch[i].inum = getnext(1) & 0x7f;
                    for (j=0; j<11; j++)
                        ch[i].ins[j]=myinsbank[ch[i].inum][j];
                    }

                track[curtrack].spos=pos;
                adlib_style=SIERRA_STYLE|MIDI_STYLE;
                break;
            }


/*        sprintf(info,"%s\r\nTicks/Quarter Note: %ld\r\n",info,deltas);
        sprintf(info,"%sms/Quarter Note: %ld",info,msqtr); */

        for (i=0; i<16; i++)
            if (track[i].on)
                {
                track[i].pos=track[i].spos;
                track[i].pv=0;
                track[i].iwait=0;
                }

    doing=1;
    midi_fm_reset();
}

std::string CmidPlayer::gettype()
{
	switch(type) {
	case FILE_LUCAS:
		return std::string("LucasArts AdLib MIDI");
	case FILE_MIDI:
		// avoid using std::to_string(midi_type) which is c++11 and may be broken in mingw32
		return std::string("General MIDI (type " + std::string(1, '0' + midi_type) + ")");
	case FILE_CMF:
		return std::string("Creative Music Format (CMF MIDI)");
	case FILE_OLDLUCAS:
		return std::string("Lucasfilm Adlib MIDI");
	case FILE_ADVSIERRA:
		return std::string("Sierra On-Line VGA MIDI");
	case FILE_SIERRA:
		return std::string("Sierra On-Line EGA MIDI");
	default:
		return std::string("MIDI unknown");
	}
}
