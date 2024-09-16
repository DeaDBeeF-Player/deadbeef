/*
 * Adplug - Replayer for many OPL2/OPL3 audio file formats.
 * Copyright (C) 1999 - 2005 Simon Peter, <dn.tlp@gmx.net>, et al.
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
 * dro2.h - DOSBox Raw OPL v2.0 player by Adam Nielsen <malvineous@shikadi.net>
 */

/*
 * Copyright (c) 2017 Wraithverge <liam82067@yahoo.com>
 * - Realigned and re-ordered sections.
 * - Removed unused garbage.
 * - Finalized support for displaying arbitrary Tag data.
 */

#include <stdint.h> // for uintxx_t
#include "player.h"

class Cdro2Player: public CPlayer
{
	protected:
		uint8_t iCmdDelayS, iCmdDelayL;
		uint8_t iConvTableLen;
		uint8_t *piConvTable;

		uint8_t *data;
		unsigned int iLength;
		unsigned int iPos;
		unsigned int iDelay;

	private:
		char title[40];
		char author[40];
		char desc[1023];

	public:
		static CPlayer *factory(Copl *newopl);

		Cdro2Player(Copl *newopl);
		~Cdro2Player();

		bool load(const std::string &filename, const CFileProvider &fp);
		bool update();
		void rewind(int subsong);
		float getrefresh();

		std::string gettype()
		{
			return std::string("DOSBox Raw OPL v2.0");
		}

		std::string gettitle() { return std::string(title, 0, 40); };
		std::string getauthor() { return std::string(author, 0, 40); };
		std::string getdesc() { return std::string(desc, 0, 1023); };
};
