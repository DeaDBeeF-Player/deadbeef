#include "scriptable_dsp.h"
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include "../deadbeef.h"

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
scriptableItemLoadDspPreset (scriptableItem_t *preset, const char *fname) {
    char path[PATH_MAX];
    snprintf (path, sizeof (path), "%s/presets/dsp/%s", deadbeef->get_system_dir (DDB_SYS_DIR_CONFIG), fname);

    int err = -1;
    FILE *fp = fopen (path, "rb");
    if (!fp) {
        return -1;
    }

    char temp[100];
    for (;;) {
        // plugin {
        int err = fscanf (fp, "%99s {\n", temp);
        if (err == EOF) {
            break;
        }
        else if (1 != err) {
            fprintf (stderr, "error plugin name\n");
            goto error;
        }

        scriptableItem_t *plugin = scriptableItemAlloc();
        scriptableItemAddSubItem(preset, plugin);
        scriptableItemSetPropertyValueForKey(plugin, "DSPNode", "type");
        scriptableItemSetPropertyValueForKey(plugin, temp, "pluginId");

        int n = 0;
        for (;;) {
            char value[1000];
            if (!fgets (temp, sizeof (temp), fp)) {
                fprintf (stderr, "unexpected eof while reading plugin params\n");
                goto error;
            }
            if (!strcmp (temp, "}\n")) {
                break;
            }
            else if (1 != sscanf (temp, "\t%1000[^\n]\n", value)) {
                fprintf (stderr, "error loading param %d\n", n);
                goto error;
            }
            char key[100];
            snprintf (key, sizeof (key), "%d", n);
            scriptableItemSetPropertyValueForKey(plugin, value, key);
            n++;
        }
    }

    err = 0;
error:
    if (err) {
        fprintf (stderr, "error loading %s\n", path);
    }
    if (fp) {
        fclose (fp);
    }
    return err;

}

scriptableItem_t *
scriptableDspRoot (void) {
    scriptableItem_t *dspRoot = scriptableItemSubItemForName (scriptableRoot(), "DSPPresets");
    if (!dspRoot) {
        dspRoot = scriptableItemAlloc();
        scriptableItemSetPropertyValueForKey(dspRoot, "DSPPresets", "name");
        scriptableItemAddSubItem(scriptableRoot(), dspRoot);
    }
    return dspRoot;
}

void
scriptableDspLoadPresets (void) {
    struct dirent **namelist = NULL;
    char path[1024];
    if (snprintf (path, sizeof (path), "%s/presets/dsp", deadbeef->get_system_dir (DDB_SYS_DIR_CONFIG)) > 0) {
        int n = scandir (path, &namelist, scandir_preset_filter, dirent_alphasort);
        int i;
        for (i = 0; i < n; i++) {
            scriptableItem_t *preset = scriptableItemAlloc();
            if (scriptableItemLoadDspPreset (preset, namelist[i]->d_name)) {
                scriptableItemFree (preset);
            }
            else {
                scriptableItemAddSubItem(scriptableDspRoot(), preset);
            }

            free (namelist[i]);
        }
        free (namelist);
    }
}
