#include "scriptable_dsp.h"
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

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

static scriptableItem_t *
scriptableDspCreateItemOfType (const char *type) {
    scriptableItem_t *item = scriptableItemAlloc();
    scriptableItemSetPropertyValueForKey(item, type, "pluginId");

    const char *name = NULL;
    DB_dsp_t **dsps = deadbeef->plug_get_dsp_list ();
    for (int i = 0; dsps[i]; i++) {
        if (!strcmp (dsps[i]->plugin.id, type)) {
            name = dsps[i]->plugin.name;
            scriptableItemSetPropertyValueForKey(item, dsps[i]->configdialog, "configDialog");
            break;
        }
    }
    if (name) {
        scriptableItemSetPropertyValueForKey(item, name, "name");
    }
    else {
        char missing[200];
        snprintf (missing, sizeof (missing), "<%s>", type);
    }

    return item;
}

scriptableItem_t *
scriptableDspRoot (void) {
    scriptableItem_t *dspRoot = scriptableItemSubItemForName (scriptableRoot(), "DSPPresets");
    if (!dspRoot) {
        dspRoot = scriptableItemAlloc();
        dspRoot->createItemOfType = scriptableDspCreateItemOfType;
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

scriptableItem_t *
scriptableDspNodeItemFromDspContext (ddb_dsp_context_t *context) {
    scriptableItem_t *node = scriptableItemAlloc();
    scriptableItemSetPropertyValueForKey(node, context->plugin->plugin.id, "pluginId");
    scriptableItemSetPropertyValueForKey(node, context->plugin->plugin.name, "name");
    if (context->plugin->configdialog) {
        scriptableItemSetPropertyValueForKey(node, context->plugin->configdialog, "configDialog");
    }

    if (context->plugin->num_params) {
        for (int i = 0; i < context->plugin->num_params (); i++) {
            char key[100];
            char value[100];
            snprintf (key, sizeof (key), "%d", i);
            context->plugin->get_param(context, i, value, sizeof (value));
            scriptableItemSetPropertyValueForKey(node, value, key);
        }
    }
    scriptableItemSetPropertyValueForKey(node, context->enabled ? "1" : "0", "enabled");

    return node;
}

scriptableItem_t *
scriptableDspConfigFromDspChain (ddb_dsp_context_t *chain) {
    scriptableItem_t *config = scriptableItemAlloc();

    while (chain) {
        scriptableItem_t *node = scriptableDspNodeItemFromDspContext (chain);
        scriptableItemAddSubItem(config, node);
        chain = chain->next;
    }

    return config;
}
