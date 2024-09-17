/*
 * Adplug - Replayer for many OPL2/OPL3 audio file formats.
 * Copyright (C) 1999 - 2007 Simon Peter, <dn.tlp@gmx.net>, et al.
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
 * dro2.cpp - DOSBox Raw OPL v2.0 player by Adam Nielsen <malvineous@shikadi.net>
 */

/*
 * Copyright (c) 2017 Wraithverge <liam82067@yahoo.com>
 * - Finalized support for displaying arbitrary Tag data.
 */

#include <cstring>
#include <stdio.h>

#include "dro2.h"

CPlayer *Cdro2Player::factory(Copl *newopl)
{
  return new Cdro2Player(newopl);
}

Cdro2Player::Cdro2Player(Copl *newopl) :
	CPlayer(newopl),
	piConvTable(NULL),
	data(0)
{
}

Cdro2Player::~Cdro2Player()
{
	if (this->data) delete[] this->data;
	if (this->piConvTable) delete[] this->piConvTable;
}

bool Cdro2Player::load(const std::string &filename, const CFileProvider &fp)
{
	binistream *f = fp.open(filename);
	if (!f) return false;

	char id[8];
	f->readString(id, 8);
	if (strncmp(id, "DBRAWOPL", 8)) {
		fp.close(f);
		return false;
	}
	int version = f->readInt(4);
	if (version != 0x2) {
		fp.close(f);
		return false;
	}

	this->iLength = f->readInt(4);
	if (this->iLength >= 1<<30 ||
	    this->iLength > fp.filesize(f) - f->pos()) {
		fp.close(f);
		return false;
	}
	this->iLength *= 2; // stored in file as number of byte p
	f->ignore(4);	// Length in milliseconds
	f->ignore(1);	/// OPL type (0 == OPL2, 1 == Dual OPL2, 2 == OPL3)
	int iFormat = f->readInt(1);
	if (iFormat != 0) {
		fp.close(f);
		return false;
	}
	int iCompression = f->readInt(1);
	if (iCompression != 0) {
		fp.close(f);
		return false;
	}
	this->iCmdDelayS = f->readInt(1);
	this->iCmdDelayL = f->readInt(1);
	this->iConvTableLen = f->readInt(1);

	this->piConvTable = new uint8_t[this->iConvTableLen];
	f->readString((char *)this->piConvTable, this->iConvTableLen);

	// Read the OPL data.
	this->data = new uint8_t[this->iLength];
	f->readString((char *)this->data, this->iLength);

	title[0] = 0;
	author[0] = 0;
	desc[0] = 0;
	int tagsize = fp.filesize(f) - f->pos();
	if (tagsize >= 3)
	{
		// The arbitrary Tag Data section begins here.
		if ((uint8_t)f->readInt(1) != 0xFF ||
			(uint8_t)f->readInt(1) != 0xFF ||
			(uint8_t)f->readInt(1) != 0x1A)
		{
			// Tag data does not present or truncated.
			goto end_section;
		}

		// "title" is maximum 40 characters long.
		f->readString(title, 40, 0);

		// Skip "author" if Tag marker byte is missing.
		if (f->readInt(1) != 0x1B) {
			f->seek(-1, binio::Add);
			goto desc_section;
		}

		// "author" is maximum 40 characters long.
		f->readString(author, 40, 0);

	desc_section:
		// Skip "desc" if Tag marker byte is missing.
		if (f->readInt(1) != 0x1C) {
			goto end_section;
		}

		// "desc" is now maximum 1023 characters long (it was 140).
		f->readString(desc, 1023, 0);
	}

end_section:
	fp.close(f);
	rewind(0);

	return true;
}

bool Cdro2Player::update()
{
	while (this->iPos < this->iLength) {
		unsigned int iIndex = this->data[this->iPos++];
		unsigned int iValue = this->data[this->iPos++];

		// Short delay
		if (iIndex == this->iCmdDelayS) {
			this->iDelay = iValue + 1;
			return true;

		// Long delay
		} else if (iIndex == this->iCmdDelayL) {
			this->iDelay = (iValue + 1) << 8;
			return true;

		// Normal write
		} else {
			if (iIndex & 0x80) {
				// High bit means use second chip in dual-OPL2 config
				this->opl->setchip(1);
			  iIndex &= 0x7F;
			} else {
			  this->opl->setchip(0);
			}
			if (iIndex >= this->iConvTableLen) {
				printf("DRO2: Error - index beyond end of codemap table!  Corrupted .dro?\n");
				return false; // EOF
			}
			int iReg = this->piConvTable[iIndex];
			this->opl->write(iReg, iValue);
		}

	}

	// This won't result in endless-play using Adplay, but IMHO that code belongs
	// in Adplay itself, not here.
	return this->iPos < this->iLength;
}

void Cdro2Player::rewind(int subsong)
{
	this->iDelay = 0;
	this->iPos = 0;
	opl->init(); 
}

float Cdro2Player::getrefresh()
{
	if (this->iDelay > 0) return 1000.0 / this->iDelay;
	else return 1000.0;
}
