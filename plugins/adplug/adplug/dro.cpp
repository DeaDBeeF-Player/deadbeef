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
 * dro.c - DOSBox Raw OPL Player by Sjoerd van der Berg <harekiet@zophar.net>
 *
 * upgraded by matthew gambrell <zeromus@zeromus.org>
 * Refactored to better match dro2.cpp 
 *  by Laurence Dougal Myers <jestarjokin@jestarjokin.net>
 * 
 * NOTES: 3-oct-04: the DRO format is not yet finalized. beware.
 *        10-jun-12: the DRO 1 format is finalized, but capturing is buggy.
 */

/*
 * Copyright (c) 2012 - 2017 Wraithverge <liam82067@yahoo.com>
 * - Added Member pointers.
 * - Fixed incorrect operator.
 * - Finalized support for displaying arbitrary Tag data.
 */

#include <cstring>
#include <stdio.h>

#include "dro.h"

CPlayer *CdroPlayer::factory(Copl *newopl)
{
  return new CdroPlayer(newopl);
}

CdroPlayer::CdroPlayer(Copl *newopl) :
	CPlayer(newopl),
	data(0)
{
}

CdroPlayer::~CdroPlayer()
{
	if (this->data) delete[] this->data;
}

bool CdroPlayer::load(const std::string &filename, const CFileProvider &fp)
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
	if (version != 0x10000) {
		fp.close(f);
		return false;
	}

	f->ignore(4);	// Length in milliseconds
	this->iLength = f->readInt(4); // stored in file as number of bytes
	if (this->iLength < 3 || this->iLength > fp.filesize(f) - f->pos()) {
		fp.close(f);
		return false;
	}

	this->data = new uint8_t[this->iLength];

	unsigned long i;
	// Some early .DRO files only used one byte for the hardware type, then
  	// later changed to four bytes with no version number change.
	// OPL type (0 == OPL2, 1 == OPL3, 2 == Dual OPL2)
	f->ignore(1);	// Type of opl data this can contain - ignored
	for (i = 0; i < 3; i++) {
  		this->data[i]=f->readInt(1);
	}

	if (this->data[0] == 0 || this->data[1] == 0 || this->data[2] == 0) {
		// If we're here then this is a later (more popular) file with
		// the full four bytes for the hardware-type.
  		i = 0; // so ignore the three bytes we just read and start again
	}

	// Read the OPL data.
	for (; i < this->iLength; i++) {
		this->data[i]=f->readInt(1);
	}

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

bool CdroPlayer::update()
{
	unsigned int iIndex;
	unsigned int iValue;
	while (this->iPos < this->iLength) {
		iIndex = this->data[this->iPos++];

		// Short delay
		if (iIndex == this->iCmdDelayS) {
			if (this->iPos >= this->iLength) return false;
			iValue = this->data[this->iPos++];
			this->iDelay = iValue + 1;
			return true;

		// Long delay
		} else if (iIndex == this->iCmdDelayL) {
			if (this->iPos + 1 >= this->iLength) return false;
			iValue = this->data[this->iPos] | (this->data[this->iPos + 1] << 8);
			this->iPos += 2;
			this->iDelay = (iValue + 1);
			return true;

		// Bank switching
		} else if (iIndex == 0x02 || iIndex == 0x03) {
			this->opl->setchip(iIndex - 0x02);

		// Normal write
		} else {
			if (iIndex == 0x04) {
				if (this->iPos+1 >= this->iLength) return false;
				iIndex = this->data[this->iPos++];
			}
			else if (this->iPos >= this->iLength) return false;
			iValue = this->data[this->iPos++];
			this->opl->write(iIndex, iValue);
		}
	}

	// This won't result in endless-play using Adplay, but IMHO that code belongs
	// in Adplay itself, not here.
	return this->iPos < this->iLength;
}

void CdroPlayer::rewind(int subsong)
{
	this->iDelay = 0;
	this->iPos = 0;
	opl->init();

	// DRO v1 assumes all registers are initialized to 0.
	// Registers not initialized to 0 will be corrected
	//  in the data stream.
	int i;
	opl->setchip(0);
	for(i = 0; i < 256; i++) {
		opl->write(i, 0);
	}
	
	opl->setchip(1);
	for(i = 0; i < 256; i++) {
		opl->write(i, 0);
	}

	opl->setchip(0);
}

float CdroPlayer::getrefresh()
{
	if (this->iDelay > 0) return 1000.0 / this->iDelay;
	else return 1000.0;
}
