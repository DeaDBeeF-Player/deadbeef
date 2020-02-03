//
//  eqpreset.c
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 1/29/20.
//  Copyright © 2020 Alexey Yakovenko. All rights reserved.
//

#include <string.h>
#include <stdlib.h>
#include "eqpreset.h"
#include "../deadbeef.h"

extern DB_functions_t *deadbeef;

static ddb_dsp_context_t *
get_supereq (void) {
    ddb_dsp_context_t *dsp = deadbeef->streamer_get_dsp_chain ();
    while (dsp) {
        if (!strcmp (dsp->plugin->plugin.id, "supereq")) {
            return dsp;
        }
        dsp = dsp->next;
    }

    return NULL;
}

void
eq_preset_save (const char *fname) {
    FILE *fp = fopen (fname, "w+b");
    if (fp) {
        ddb_dsp_context_t *eq = get_supereq ();
        if (eq) {
            char fv[100];
            float v;
            for (int i = 0; i < 18; i++) {
                eq->plugin->get_param (eq, i+1, fv, sizeof (fv));
                v = (float)atof (fv);
                fprintf (fp, "%f\n", v);
            }
            eq->plugin->get_param (eq, 0, fv, sizeof (fv));
            v = (float)atof (fv);
            fprintf (fp, "%f\n", v);
        }
        fclose (fp);
    }
}

int
eq_preset_load (const char *fname, float *preamp, float values[18]) {
    FILE *fp = fopen (fname, "rt");
    if (fp) {
        int i = 0;
        while (i < 19) {
            char tmp[20];
            char *out = fgets (tmp, sizeof (tmp), fp);
            if (!out) {
                break;
            }
            float val = (float)atof (tmp);
            if (i == 18) {
                *preamp = val;
            }
            else {
                values[i] = val;
            }
            i++;
        }
        fclose (fp);
        if (i != 19) {
            return -1;
        }
    }
    return 0;
}

int
eq_preset_load_fb2k (const char *fname, float values[18]) {
    FILE *fp = fopen (fname, "rt");
    if (fp) {
        int i = 0;
        while (i < 18) {
            char tmp[20];
            char *out = fgets (tmp, sizeof (tmp), fp);
            if (!out) {
                break;
            }
            values[i] = (float)atoi (tmp);
            i++;
        }
        fclose (fp);
        if (i != 18) {
            return -1;
        }
    }
    return 0;
}
