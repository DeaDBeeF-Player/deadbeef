/*
 * Adplug - Replayer for many OPL2/OPL3 audio file formats.
 * Copyright (C) 1999 - 2003 Simon Peter, <dn.tlp@gmx.net>, et al.
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
 * [xad] HYBRID player, by Riven the Mage <riven@ok.ru>
 */

/*
 * Copyright (c) 2015 - 2017 Wraithverge <liam82067@yahoo.com>
 * - Realigned to Tabs.
 * - Added support for Speed indicator in 'File Info' dialogues.
 */

#include "xad.h"

class CxadhybridPlayer: public CxadPlayer
{
public:
	static CPlayer *factory(Copl *newopl);

	CxadhybridPlayer(Copl *newopl): CxadPlayer(newopl)
	{ }

protected:
	struct hyb_instrument
	{
		char            name[7];
		unsigned char   mod_wave;
		unsigned char   mod_AD;
		unsigned char   mod_SR;
		unsigned char   mod_crtl;
		unsigned char   mod_volume;
		unsigned char   car_wave;
		unsigned char   car_AD;
		unsigned char   car_SR;
		unsigned char   car_crtl;
		unsigned char   car_volume;
		unsigned char   connect;
	};

	struct
	{
		unsigned char   order_pos;
		unsigned char   pattern_pos;

		unsigned char   *order;

		hyb_instrument  *inst;

	struct
	{
		unsigned short  freq;
		unsigned short  freq_slide;
	} channel[9];

		unsigned char   speed;
		unsigned char   speed_counter;
	} hyb;
	//
	bool            xadplayer_load();
	void            xadplayer_rewind(int subsong);
	void            xadplayer_update();
	float           xadplayer_getrefresh();
	std::string     xadplayer_gettype();
	std::string     xadplayer_getinstrument(unsigned int i);
	unsigned int    xadplayer_getinstruments();

	// Wraithverge: added this.
	unsigned int    xadplayer_getspeed();

private:
	static const unsigned char hyb_adlib_registers[99];
	static const unsigned short hyb_notes[98];
	static const unsigned char hyb_default_instrument[11];
};
