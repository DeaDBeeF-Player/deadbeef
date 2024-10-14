/*
  Adplug - Replayer for many OPL2/OPL3 audio file formats.
  Copyright (C) 1999 - 2004, 2006 Simon Peter, <dn.tlp@gmx.net>, et al.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

  dmo.cpp - TwinTeam loader by Riven the Mage <riven@ok.ru>
*/
/*
  NOTES:
  Panning is ignored.

  A WORD ist 16 bits, a DWORD is 32 bits and a BYTE is 8 bits in this context.
*/

#include <string.h>
#include <binstr.h>

#include "dmo.h"
#include "debug.h"

#define ARRAY_AS_DWORD(a, i) \
((a[i + 3] << 24) + (a[i + 2] << 16) + (a[i + 1] << 8) + a[i])
#define ARRAY_AS_WORD(a, i)	((a[i + 1] << 8) + a[i])

/* -------- Public Methods -------------------------------- */

CPlayer *CdmoLoader::factory(Copl *newopl)
{
  return new CdmoLoader(newopl);
}

bool CdmoLoader::load(const std::string &filename, const CFileProvider &fp)
{
  int i;

  binistream *f = fp.open(filename);
  if (!f) return false;

  // check header
  dmo_unpacker unpacker;
  unsigned char chkhdr[16];

  f->readString((char *)chkhdr, 16);

  if (!unpacker.decrypt(chkhdr, 16))
    {
      fp.close(f);
      return false;
    }

  // get file size
  long packed_length = fp.filesize(f);
  f->seek(0);

  unsigned char *packed_module = new unsigned char [packed_length];

  // load file
  f->readString((char *)packed_module, packed_length);
  fp.close(f);

  // decrypt
  unpacker.decrypt(packed_module, packed_length);

  long unpacked_length = 0x2000 * ARRAY_AS_WORD(packed_module, 12);
  unsigned char *module = new unsigned char [unpacked_length];

  // unpack
  if (!unpacker.unpack(packed_module, packed_length, module, unpacked_length))
    {
      delete [] packed_module;
      delete [] module;
      return false;
    }

  delete [] packed_module;

  // "TwinTeam" - signed ?
  if (memcmp(module,"TwinTeam Module File""\x0D\x0A",22))
    {
      delete [] module;
      return false;
    }

  // load header
  binisstream	uf(module, unpacked_length);
  uf.setFlag(binio::BigEndian, false); uf.setFlag(binio::FloatIEEE);

  memset(&header,0,sizeof(s3mheader));

  uf.ignore(22);				// ignore DMO header ID string
  uf.readString(header.name, 28);
  header.name[27] = 0;				// ensure termination

  uf.ignore(2);				// _unk_1
  header.ordnum  = uf.readInt(2);
  header.insnum  = uf.readInt(2);
  header.patnum  = uf.readInt(2);
  uf.ignore(2);				// _unk_2
  header.is      = uf.readInt(2);
  header.it      = uf.readInt(2);

  if (header.ordnum >= 256 || header.insnum > 99 || header.patnum > 99) {
    delete [] module;
    return false;
  }

  memset(header.chanset,0xFF,32);

  for (i=0;i<9;i++)
    header.chanset[i] = 0x10 + i;

  uf.ignore(32);				// ignore panning settings for all 32 channels

  // load orders
  for(i = 0; i < 256; i++) orders[i] = uf.readInt(1);

  orders[header.ordnum] = 0xFF;

  // load pattern lengths
  unsigned short my_patlen[100];
  for(i = 0; i < 100; i++) my_patlen[i] = uf.readInt(2);

  // load instruments
  for (i = 0; i < header.insnum; i++)
    {
      memset(&inst[i],0,sizeof(s3minst));

      uf.readString(inst[i].name, 28);
      inst[i].name[27] = 0;

      inst[i].volume = uf.readInt(1);
      inst[i].dsk    = uf.readInt(1);
      inst[i].c2spd  = uf.readInt(4);
      inst[i].type   = uf.readInt(1);
      inst[i].d00    = uf.readInt(1);
      inst[i].d01    = uf.readInt(1);
      inst[i].d02    = uf.readInt(1);
      inst[i].d03    = uf.readInt(1);
      inst[i].d04    = uf.readInt(1);
      inst[i].d05    = uf.readInt(1);
      inst[i].d06    = uf.readInt(1);
      inst[i].d07    = uf.readInt(1);
      inst[i].d08    = uf.readInt(1);
      inst[i].d09    = uf.readInt(1);
      inst[i].d0a    = uf.readInt(1);
      /*
       * Originally, riven sets d0b = d0a and ignores 1 byte in the
       * stream, but i guess this was a typo, so i read it here.
       */
      inst[i].d0b    = uf.readInt(1);
    }

  // load patterns
  for (i = 0; i < header.patnum; i++) {
    long cur_pos = uf.pos();

    load_pattern(i, &uf, my_patlen[i]);
    uf.seek(cur_pos + my_patlen[i]);
  }

  delete [] module;
  rewind(0);
  return true;
}

std::string CdmoLoader::gettype()
{
  return std::string("TwinTeam (packed S3M)");
}

std::string CdmoLoader::getauthor()
{
  /*
    All available .DMO modules written by one composer. And because all .DMO
    stuff was lost due to hd crash (TwinTeam guys said this), there are
    never(?) be another.
  */
  return std::string("Benjamin GERARDIN");
}

/* -------- Private Methods ------------------------------- */

unsigned short CdmoLoader::dmo_unpacker::brand(unsigned short range)
{
  bseed *= 0x08088405U;
  bseed++;

  return (uint64_t)bseed * range >> 32;
}

bool CdmoLoader::dmo_unpacker::decrypt(unsigned char *buf, size_t len)
{
  if (len < headersize)
    return false;

  bseed = ARRAY_AS_DWORD(buf, 0);

  uint32_t seed = 0;
  for (int i = 0; i <= ARRAY_AS_WORD(buf, 4); i++)
    seed += brand(0xffff);

  bseed = seed ^ ARRAY_AS_DWORD(buf, 6);

  if (ARRAY_AS_WORD(buf, 10) != brand(0xffff))
    return false;

  for (size_t i = headersize; i < len; i++)
    buf[i] ^= brand(0x100);

  buf[len - 2] = buf[len - 1] = 0;

  return true;
}

long CdmoLoader::dmo_unpacker::unpack_block(unsigned char *ibuf, size_t ilen,
					    unsigned char *obuf, size_t olen)
{
  size_t ipos = 0, opos = 0;

  // LZ77 child
  while (ipos < ilen) {
    size_t cpy = 0, ofs = 0, lit = 0;
    unsigned char code, par1, par2;

    code = ibuf[ipos++];
    par1 = ipos < ilen ? ibuf[ipos] : 0;
    par2 = ipos + 1 < ilen ? ibuf[ipos + 1] : 0;
    switch (code >> 6) {
    case 0:
      // 00xxxxxx: use (X + 1) literal bytes
      lit = (code & 0x3F) + 1;
      break;

    case 1:
      // 01xxxxxx xxxyyyyy: copy (Y + 3) bytes from offset (X + 1)
      ipos++; // for par1
      ofs = ((code & 0x3F) << 3) + ((par1 & 0xE0) >> 5) + 1;
      cpy = (par1 & 0x1F) + 3;
      break;

    case 2:
      // 10xxxxxx xyyyzzzz: copy (Y + 3) bytes from offset(X + 1);
      // use Z literal bytes
      ipos++; // for par1
      ofs = ((code & 0x3F) << 1) + (par1 >> 7) + 1;
      cpy = ((par1 & 0x70) >> 4) + 3;
      lit = par1 & 0x0F;
      break;

    case 3:
      // 11xxxxxx xxxxxxxy yyyyzzzz: copy (Y + 4) from offset X;
      // use Z literal bytes
      ipos += 2; // for par1 and par2
      ofs = ((code & 0x3F) << 7) + (par1 >> 1);
      cpy = ((par1 & 0x01) << 4) + (par2 >> 4) + 4;
      lit = par2 & 0x0F;
      break;
    }

    // check lengths and offset
    if (ipos + lit > ilen || opos + cpy + lit > olen || ofs > opos)
      return -1;

    // copy - can't use memcpy() because source and destination may overlap
    for (size_t i = 0; i < cpy; i++)
      obuf[opos + i] = obuf[opos - ofs + i];
    opos += cpy;

    // copy literal bytes
    while (lit--)
      obuf[opos++] = ibuf[ipos++];
  }

  return opos;
}

size_t CdmoLoader::dmo_unpacker::unpack(unsigned char *ibuf, size_t inputsize,
					unsigned char *obuf, size_t outputsize)
{
  if (inputsize < headersize + 2)
    return 0;

  unsigned short block_count = ARRAY_AS_WORD(ibuf, headersize);
  unsigned block_start = headersize + 2 + 2 * block_count;

  if (inputsize < block_start)
    return 0;

  unsigned char *block_length = &ibuf[headersize + 2];
  ibuf += block_start;
  inputsize -= block_start;

  size_t olen = 0;
  for (int i = 0; i < block_count; i++) {
    unsigned short blen = ARRAY_AS_WORD(block_length, 2 * i);
    if (blen < 2 || blen > inputsize)
      return 0;

    unsigned short bul = ARRAY_AS_WORD(ibuf, 0);

    if (unpack_block(ibuf + 2, blen - 2, obuf, outputsize - olen) != bul)
      return 0;

    obuf += bul;
    olen += bul;

    ibuf += blen;
    inputsize -= blen;
  }

  return olen;
}
