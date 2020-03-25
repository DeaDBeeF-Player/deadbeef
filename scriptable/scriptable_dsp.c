#include <assert.h>
#include <dirent.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "scriptable_dsp.h"
#include "pluginsettings.h"

extern DB_functions_t *deadbeef;

static scriptableStringListItem_t *
scriptableDspChainItemNames (scriptableItem_t *item);

static scriptableStringListItem_t *
scriptableDspChainItemTypes (scriptableItem_t *item);

static scriptableItem_t *
scriptableDspCreateItemOfType (struct scriptableItem_s *root, const char *type);

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
getDspPresetFullPathName (const char *fname, char *path, size_t size) {
    int res = snprintf (path, size, "%s/presets/dsp/%s", deadbeef->get_system_dir (DDB_SYS_DIR_CONFIG), fname);
    return res < size ? 0 : -1;
}

static int
scriptableItemLoadDspPreset (scriptableItem_t *preset, const char *fname) {
    char path[PATH_MAX];
    if (getDspPresetFullPathName (fname, path, sizeof (path)) < 0) {
        return -1;
    }

    int err = -1;
    FILE *fp = fopen (path, "rb");
    if (!fp) {
        return -1;
    }

    char temp[100];
    for (;;) {
        // plugin {
        int ret = fscanf (fp, "%99s {\n", temp);
        if (ret == EOF) {
            break;
        }
        else if (1 != ret) {
            fprintf (stderr, "error plugin name\n");
            goto error;
        }

        scriptableItem_t *plugin = scriptableDspCreateItemOfType (preset, temp);
        scriptableItemAddSubItem(preset, plugin);

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
scriptableDspCreateItemOfType (struct scriptableItem_s *root, const char *type) {
    scriptableItem_t *item = scriptableItemAlloc();
    scriptableItemSetPropertyValueForKey(item, "DSPNode", "type");
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
        scriptableItemSetPropertyValueForKey(item, missing, "name");
    }

    return item;
}

static int
scriptableDspPresetUpdateItemForSubItem (struct scriptableItem_s *item, struct scriptableItem_s *subItem) {
    const char *presetName = scriptableItemPropertyValueForKey (item, "name");
    if (!presetName) {
        return -1;
    }

    char fname[PATH_MAX];
    if (snprintf (fname, sizeof (fname), "%s.txt", presetName) >= sizeof (fname)) {
        return -1;
    }

    char path[PATH_MAX];
    if (getDspPresetFullPathName (fname, path, sizeof (path)) < 0) {
        return -1;
    }


    FILE *fp = fopen (path, "w+b");
    if (!fp) {
        return -1;
    }

    for (scriptableItem_t *i = item->children; i; i = i->next) {
        const char *pluginId = scriptableItemPropertyValueForKey (i, "pluginId");
        if (!pluginId) {
            continue;
        }

        fprintf (fp, "%s {\n", pluginId);

        for (scriptableKeyValue_t *kv = i->properties; kv; kv = kv->next) {
            fprintf (fp, "\t%s\n", kv->value);
        }

        fprintf (fp, "}\n");
    }

    if (fclose (fp) != 0) {
        return -1;
    }

    return 0;
}

static scriptableStringListItem_t *
scriptableDspPresetItemNames (scriptableItem_t *item) {
    scriptableStringListItem_t *s = scriptableStringListItemAlloc();
    s->str = strdup ("DSP Preset");
    return s;
}

static scriptableStringListItem_t *
scriptableDspPresetItemTypes (scriptableItem_t *item) {
    scriptableStringListItem_t *s = scriptableStringListItemAlloc();
    s->str = strdup ("DSPPreset");
    return s;
}

static scriptableItem_t *scriptableDspCreateBlankPreset(char *name) {
    scriptableItem_t *item = scriptableItemAlloc();
    scriptableItemSetPropertyValueForKey(item, name, "name");
    item->factoryItemNames = scriptableDspChainItemNames;
    item->factoryItemTypes = scriptableDspChainItemTypes;
    item->createItemOfType = scriptableDspCreateItemOfType;
    item->isList = 1;
    return item;
}

static scriptableItem_t *
scriptableDspCreatePresetWithPluginId (scriptableItem_t *root, const char *pluginId) {
    char name[100] = "New DSP Preset";
    int i;
    const int MAX_ATTEMPTS = 100;
    for (i = 1; i < MAX_ATTEMPTS; i++) {
        scriptableItem_t *c = NULL;
        for (c = root->children; c; c = c->next) {
            const char *cname = scriptableItemPropertyValueForKey(c, "name");
            if (!strcasecmp (name, cname)) {
                break;
            }
        }
        if (!c) {
            break;
        }
        snprintf (name, sizeof (name), "New DSP Preset %02d", i);
    }
    if (i == MAX_ATTEMPTS) {
        return NULL;
    }

    scriptableItem_t * item = scriptableDspCreateBlankPreset(name);

    return item;
}

static int
scriptableDspRootRemoveSubItem (struct scriptableItem_s *item, struct scriptableItem_s *subItem) {
    const char *name = scriptableItemPropertyValueForKey(subItem, "name");
    if (!name) {
        return -1;
    }

    char fname[PATH_MAX];
    if (snprintf (fname, sizeof (fname), "%s.txt", name) >= sizeof (fname)) {
        return -1;
    }

    char path[PATH_MAX];
    if (getDspPresetFullPathName (fname, path, sizeof (path)) < 0) {
        return -1;
    }

    return unlink (path);
}


scriptableItem_t *
scriptableDspRoot (void) {
    scriptableItem_t *dspRoot = scriptableItemSubItemForName (scriptableRoot(), "DSPPresets");
    if (!dspRoot) {
        dspRoot = scriptableItemAlloc();
        dspRoot->createItemOfType = scriptableDspCreatePresetWithPluginId;
        scriptableItemSetPropertyValueForKey(dspRoot, "DSPPresets", "name");
        scriptableItemAddSubItem(scriptableRoot(), dspRoot);
        dspRoot->factoryItemNames = scriptableDspPresetItemNames;
        dspRoot->factoryItemTypes = scriptableDspPresetItemTypes;
        dspRoot->isList = 1;
        dspRoot->removeSubItem = scriptableDspRootRemoveSubItem;
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
            char name[PATH_MAX] = "";
            char *end = strrchr (namelist[i]->d_name, '.');
            if (!end || end == namelist[i]->d_name) {
                continue;
            }

            strncat(name, namelist[i]->d_name, end-namelist[i]->d_name);

            scriptableItem_t *preset = scriptableDspCreateBlankPreset(name);

            if (scriptableItemLoadDspPreset (preset, namelist[i]->d_name)) {
                scriptableItemFree (preset);
            }
            else {
                scriptableItemAddSubItem(scriptableDspRoot(), preset);
            }
            preset->updateItemForSubItem = scriptableDspPresetUpdateItemForSubItem;

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

ddb_dsp_context_t *
scriptableDspConfigToDspChain (scriptableItem_t *item) {
    ddb_dsp_context_t *head = NULL;
    ddb_dsp_context_t *tail = NULL;
    scriptableItem_t *c;
    for (c = item->children; c; c = c->next) {
        const char *pluginId = scriptableItemPropertyValueForKey(c, "pluginId");
        DB_plugin_t *plugin = deadbeef->plug_get_for_id (pluginId);
        if (plugin->type != DB_PLUGIN_DSP) {
            break;
        }
        DB_dsp_t *dsp = (DB_dsp_t *)plugin;

        ddb_dsp_context_t *ctx = dsp->open ();
        if (dsp->num_params) {
            settings_data_t dt;
            memset (&dt, 0, sizeof (dt));
            if (dsp->configdialog) {
                settings_data_init (&dt, dsp->configdialog);
            }

            int n = dsp->num_params();
            for (int i = 0; i < n; i++) {
                char key[10];
                snprintf (key, sizeof (key), "%d", i);
                const char *value = scriptableItemPropertyValueForKey(c, key);
                if (!value) {
                    for (int p = 0; p < dt.nprops; p++) {
                        if (atoi(dt.props[p].key) == i) {
                            value = dt.props[p].def;
                            break;
                        }
                    }
                }
                assert (value);
                dsp->set_param (ctx, i, value);
            }

            if (dsp->configdialog) {
                settings_data_free(&dt);
            }

        }
        if (tail) {
            tail->next = ctx;
        }
        else {
            head = ctx;
        }
        tail = ctx;
    }

    if (c && head) {
        deadbeef->dsp_preset_free (head);
        return NULL;
    }
    return head;
}

static scriptableStringListItem_t *
scriptableDspChainItemNames (scriptableItem_t *item) {
    scriptableStringListItem_t *head = NULL;
    scriptableStringListItem_t *tail = NULL;
    DB_dsp_t **plugs = deadbeef->plug_get_dsp_list ();
    for (int i = 0; plugs[i]; i++) {
        scriptableStringListItem_t *s = scriptableStringListItemAlloc();
        s->str = strdup (plugs[i]->plugin.name);
        if (tail) {
            tail->next = s;
        }
        else {
            head = s;
        }
        tail = s;
    }
    return head;
}

static scriptableStringListItem_t *
scriptableDspChainItemTypes (scriptableItem_t *item) {
    scriptableStringListItem_t *head = NULL;
    scriptableStringListItem_t *tail = NULL;
    DB_dsp_t **plugs = deadbeef->plug_get_dsp_list ();
    for (int i = 0; plugs[i]; i++) {
        scriptableStringListItem_t *s = scriptableStringListItemAlloc();
        s->str = strdup (plugs[i]->plugin.id);
        if (tail) {
            tail->next = s;
        }
        else {
            head = s;
        }
        tail = s;
    }
    return head;
}

scriptableItem_t *
scriptableDspPresetFromDspChain (ddb_dsp_context_t *chain) {
    scriptableItem_t *config = scriptableItemAlloc();

    while (chain) {
        scriptableItem_t *node = scriptableDspNodeItemFromDspContext (chain);
        scriptableItemAddSubItem(config, node);
        chain = chain->next;
    }

    config->isList = 1;
    config->factoryItemNames = scriptableDspChainItemNames;
    config->factoryItemTypes = scriptableDspChainItemTypes;
    config->createItemOfType = scriptableDspCreateItemOfType;

    // CRUD
    config->updateItemForSubItem = scriptableDspPresetUpdateItemForSubItem;

    return config;
}

