#include "scriptable_encoder.h"
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include "../deadbeef.h"
#include "strdupa.h"

extern DB_functions_t *deadbeef;

static int
dirent_alphasort (const struct dirent **a, const struct dirent **b) {
    return strcmp ((*a)->d_name, (*b)->d_name);
}

static int
scandir_preset_filter (const struct dirent *ent) {
    char *ext = strrchr (ent->d_name, '.');
    if (ext && !strcasecmp (ext, ".txt")) {
        return 1;
    }
    return 0;
}


static int
scriptableItemLoadEncoderPreset (scriptableItem_t *preset, const char *name, const char *path) {
    int err = -1;
    FILE *fp = fopen (path, "rt");
    if (!fp) {
        return -1;
    }

    char str[1024];
    while (fgets (str, sizeof (str), fp)) {
        // chomp
        char *cr = str + strlen (str) - 1;
        while (*cr == '\n') {
            cr--;
        }
        cr++;
        *cr = 0;

        char *sp = strchr (str, ' ');
        if (!sp) {
            continue;
        }

        *sp = 0;
        char *item = sp + 1;

        scriptableItemSetPropertyValueForKey(preset, item, str);
        if (!strcmp (str, "title")) {
            scriptableItemSetPropertyValueForKey(preset, item, "name");
        }
    }

    err = 0;

error:
    if (fp) {
        fclose (fp);
    }
    return err;
}

scriptableItem_t *
scriptableEncoderRoot (void) {
    scriptableItem_t *encoderRoot = scriptableItemSubItemForName (scriptableRoot(), "EncoderPresets");
    if (!encoderRoot) {
        encoderRoot = scriptableItemAlloc();
        scriptableItemSetPropertyValueForKey(encoderRoot, "EncoderPresets", "name");
        scriptableItemAddSubItem(scriptableRoot(), encoderRoot);
    }
    return encoderRoot;
}

void
scriptableEncoderLoadPresets (void) {
    char path[PATH_MAX];
    if (snprintf (path, sizeof (path), "%s/presets/encoders", deadbeef->get_system_dir (DDB_SYS_DIR_CONFIG)) < 0) {
        path[0] = 0;
    }

    char syspath[PATH_MAX];
    if (snprintf (syspath, sizeof (syspath), "%s/convpresets", deadbeef->get_system_dir (DDB_SYS_DIR_PLUGIN)) < 0) {
        syspath[0] = 0;
    }

    const char *preset_dirs[] = {
        syspath, path, NULL
    };

    for (int di = 0; preset_dirs[di]; di++) {
        const char *path = preset_dirs[di];
        struct dirent **namelist = NULL;
        int n = scandir (path, &namelist, scandir_preset_filter, dirent_alphasort);
        int i;
        for (i = 0; i < n; i++) {
            char s[PATH_MAX];
            if (snprintf (s, sizeof (s), "%s/%s", path, namelist[i]->d_name) > 0){

                scriptableItem_t *preset = scriptableItemAlloc();
                if (scriptableItemLoadEncoderPreset (preset, namelist[i]->d_name, s)) {
                    scriptableItemFree (preset);
                }
                else {
                    if (path == syspath) {
                        scriptableItemSetPropertyValueForKey(preset, "true", "readonly");
                    }

                    scriptableItemAddSubItem(scriptableEncoderRoot(), preset);
                }
            }
        }
        for (i = 0; i < n; i++) {
            free (namelist[i]);
        }
        free (namelist);
        namelist = NULL;
    }
}
