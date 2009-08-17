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
        if (str[0] == '#') {
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

