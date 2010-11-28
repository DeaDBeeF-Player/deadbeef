/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2010 Alexey Yakovenko <waker@users.sourceforge.net>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef __SUPEREQ_H
#define __SUPEREQ_H

typedef struct DB_supereq_dsp_s {
    DB_dsp_t dsp;
    float (*get_band) (DB_dsp_instance_t *inst, int band);
    void (*set_band) (DB_dsp_instance_t *inst, int band, float value);
    float (*get_preamp) (DB_dsp_instance_t *inst);
    void (*set_preamp) (DB_dsp_instance_t *inst, float value);
} DB_supereq_dsp_t;

#endif
