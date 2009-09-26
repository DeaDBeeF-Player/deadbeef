/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009  Alexey Yakovenko

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef __CONF_H
#define __CONF_H

extern char conf_alsa_soundcard[1024];
extern int conf_samplerate;
extern int conf_src_quality;
extern char conf_hvsc_path[1024];
extern int conf_hvsc_enable;
extern char conf_blacklist_plugins[1024];
extern int conf_close_send_to_tray;
extern int conf_replaygain_mode;
extern int conf_replaygain_scale;

int
conf_load (void);

#endif // __CONF_H
