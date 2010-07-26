/*
 * AdPlug - Replayer for many OPL2/OPL3 audio file formats.
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * players.h - Players enumeration, by Simon Peter <dn.tlp@gmx.net>
 */

#ifndef H_ADPLUG_PLAYERS
#define H_ADPLUG_PLAYERS

#include "opl.h"
#include "player.h"

class CPlayerDesc
{
public:
  typedef CPlayer *(*Factory)(Copl *);

  Factory	factory;
  char filetype[50];

  CPlayerDesc();
  CPlayerDesc(const CPlayerDesc &pd);
  CPlayerDesc(Factory f, const char *type, const char *ext);

  ~CPlayerDesc();

  void add_extension(const char *ext);
  const char *get_extension(unsigned int n) const;

  CPlayerDesc *next;

private:
  char		*extensions;
  unsigned long	extlength;
};

class CPlayers
{
public:
  CPlayerDesc *head;
  CPlayerDesc *tail;
    CPlayers() {
        head = 0;
        tail = 0;
    }
  const CPlayerDesc *lookup_filetype(const char *ftype) const;
  const CPlayerDesc *lookup_extension(const char *extension) const;

  void push_back (CPlayerDesc *ply) {
      ply->next = 0;
      if (tail) {
          tail->next = ply;
      }
      tail = ply;
      if (!head) {
          head = ply;
      }
  }

};

#endif
