/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009  Alexey Yakovenko

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

char conf_alsa_soundcard[1024] = "default"; // name of soundcard for alsa player
int conf_samplerate = 48000;
int conf_src_quality = 1;
char conf_hvsc_path[1024] = "";
int conf_hvsc_enable = 0;

int
conf_load (void) {
    extern char dbconfdir[1024]; // $HOME/.config/deadbeef
    char str[1024];
    snprintf (str, 1024, "%s/config", dbconfdir);
    FILE *fp = fopen (str, "rt");
    if (!fp) {
        fprintf (stderr, "failed to load config file\n");
        return -1;
    }
    int line = 0;
    while (fgets (str, 1024, fp) != NULL) {
        line++;
        if (str[0] == '#' || str[0] <= 0x20) {
            continue;
        }
        uint8_t *p = (uint8_t *)str;
        while (*p && *p > 0x20) {
            p++;
        }
        if (!*p) {
            fprintf (stderr, "error in config file line %d\n", line);
            continue;
        }
        *p = 0;
        p++;
        // skip whitespace
        while (*p && *p <= 0x20) {
            p++;
        }
        if (!*p) {
            fprintf (stderr, "error in config file line %d\n", line);
            continue;
        }
        char *value = p;
        // remove trailing trash
        while (*p && *p >= 0x20) {
            p++;
        }
        *p = 0;
        if (!strcasecmp (str, "samplerate")) {
            conf_samplerate = atoi (value);
        }
        else if (!strcasecmp (str, "alsa_soundcard")) {
            strncpy (conf_alsa_soundcard, value, 1024);
            conf_alsa_soundcard[1023] = 0;
        }
        else if (!strcasecmp (str, "src_quality")) {
            conf_src_quality = atoi (value);
        }
        else if (!strcasecmp (str, "hvsc_path")) {
            strncpy (conf_hvsc_path, value, 1024);
            conf_hvsc_path[1023] = 0;
        }
        else if (!strcasecmp (str, "hvsc_enable")) {
            conf_hvsc_enable = atoi (value);
        }
        else {
            fprintf (stderr, "error in config file line %d\n", line);
        }
    }
    fclose (fp);
    return 0;
}

