#include <assert.h>
#include <dirent.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "growableBuffer.h"
#include "pluginsettings.h"
#include "scriptable_dsp.h"

extern DB_functions_t *deadbeef;

static scriptableStringListItem_t *
scriptableDspChainItemNames (scriptableItem_t *item);

static scriptableStringListItem_t *
scriptableDspChainItemTypes (scriptableItem_t *item);

static scriptableItem_t *
scriptableDspCreateItemOfType (scriptableItem_t *root, const char *type);

static void
scriptableDspInitContextFromItem(scriptableItem_t *c, ddb_dsp_context_t *ctx, DB_dsp_t *dsp);

static int
scriptableDspPresetUpdateItem (scriptableItem_t *item);

static int
scriptableDspPresetUpdateItemForSubItem (scriptableItem_t *item, scriptableItem_t *subItem);

static scriptableItem_t *
scriptableDspCreatePresetWithType (scriptableItem_t *root, const char *type);

static scriptableStringListItem_t *
scriptableDspPresetItemNames (scriptableItem_t *item);

static scriptableStringListItem_t *
scriptableDspPresetItemTypes (scriptableItem_t *item);

static int
scriptableDspRootRemoveSubItem (scriptableItem_t *item, scriptableItem_t *subItem);

static int
isPresetNameAllowed (scriptableItem_t *preset, const char *name);

static char *
scriptableDspPresetNodeSaveToString (scriptableItem_t *item);

static void
scriptableDspPropertyValueWillChangeForKey (scriptableItem_t *item, const char *key);

static void
scriptableDspPresetNodePropertyValueDidChangeForKey (scriptableItem_t *item, const char *key);

static int
scriptableDspPresetSave(scriptableItem_t *item);

static int
scriptableDspPresetDelete(scriptableItem_t *subItem);

static scriptableOverrides_t
scriptableDspNodeCallbacks = {
    .saveToString = scriptableDspPresetNodeSaveToString,
    .propertyValueDidChangeForKey = scriptableDspPresetNodePropertyValueDidChangeForKey,
};

static const char *
_pbIdentifier(scriptableItem_t *item) {
    return "deadbeef.dspnode";
}

static scriptableOverrides_t
scriptableDspPresetCallbacks = {
    .pasteboardItemIdentifier = _pbIdentifier,
    .factoryItemNames = scriptableDspChainItemNames,
    .factoryItemTypes = scriptableDspChainItemTypes,
    .createItemOfType = scriptableDspCreateItemOfType,
    .didUpdateItem = scriptableDspPresetUpdateItem,
    .didUpdateChildItem = scriptableDspPresetUpdateItemForSubItem,
    .save = scriptableDspPresetSave,
    .propertyValueWillChangeForKey = scriptableDspPropertyValueWillChangeForKey,
};

static scriptableOverrides_t scriptableDspPresetListCallbacks = {
    .createItemOfType = scriptableDspCreatePresetWithType,
    .factoryItemNames = scriptableDspPresetItemNames,
    .factoryItemTypes = scriptableDspPresetItemTypes,
    .willRemoveChildItem = scriptableDspRootRemoveSubItem,
    .isSubItemNameAllowed = isPresetNameAllowed,
};


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
            else if (1 != sscanf (temp, "\t%100[^\n]\n", value)) {
                // skip if empty line
                if (!strcmp (temp, "\n")) {
                    continue;
                }
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

static DB_dsp_t *
dspPluginForId (const char *pluginId) {
    DB_dsp_t **dsps = deadbeef->plug_get_dsp_list ();
    for (int i = 0; dsps[i]; i++) {
        if (!strcmp (dsps[i]->plugin.id, pluginId)) {
            return dsps[i];
        }
    }
    return NULL;
}

static scriptableItem_t *
scriptableDspCreateItemOfType (scriptableItem_t *root, const char *type) {
    scriptableItem_t *item = scriptableItemAlloc();

    scriptableItemSetPropertyValueForKey(item, type, "pluginId");
    scriptableItemSetOverrides(item, &scriptableDspNodeCallbacks);

    DB_dsp_t *dsp = dspPluginForId(type);
    if (dsp) {
        const char *name = dsp->plugin.name;
        scriptableItemSetConfigDialog(item, dsp->configdialog);
        scriptableItemSetPropertyValueForKey(item, name, "name");
    }
    else {
        char missing[200];
        snprintf (missing, sizeof (missing), "%s (Plugin Missing)", type);
        scriptableItemSetPropertyValueForKey(item, missing, "name");
    }

    return item;
}

static void
scriptableDspPresetNodePropertyValueDidChangeForKey (scriptableItem_t *item, const char *key) {
    scriptableItem_t *parent = scriptableItemParent(item);
    if (parent == NULL) {
        return;
    }

    const char *parentName = scriptableItemPropertyValueForKey(parent, "name");
    if (parentName) {
        return; // ignore actual presets
    }

    // Set as current dsp chain

    ddb_dsp_context_t *chain = deadbeef->streamer_get_dsp_chain ();

    for (scriptableItem_t *node = scriptableItemChildren(parent); node; node = scriptableItemNext(node)) {
        if (node == item) {
            int param = atoi (key);
            const char *value = scriptableItemPropertyValueForKey(item, key);
            chain->plugin->set_param (chain, param, value);
            break;
        }
        chain = chain->next;
    }
}

static void
scriptableDspPropertyValueWillChangeForKey (scriptableItem_t *item, const char *key) {
    if (scriptableItemFlags(item) & SCRIPTABLE_FLAG_IS_LOADING) {
        return;
    }
    if (!strcmp (key, "name")) {
        // FIXME: this deletes the preset during rename.
        // If the next save operation fails - data loss will occur.
        scriptableDspPresetDelete(item);
    }
}



static char *
scriptableDspPresetNodeSaveToString (scriptableItem_t *node) {
    const char *pluginId = scriptableItemPropertyValueForKey (node, "pluginId");
    if (!pluginId) {
        return NULL;
    }

    growableBuffer_t buffer;
    growableBufferInitWithSize(&buffer, 1000);

    DB_dsp_t *dsp = dspPluginForId(pluginId);
    if (!dsp) {
        // when a plugin is missing: write out all numeric-name properties, in their original order
        for (scriptableKeyValue_t *kv = scriptableItemProperties(node); kv; kv = kv->next) {
            int intKey = atoi (kv->key);
            char stringKey[10];
            snprintf (stringKey, sizeof (stringKey), "%d", intKey);
            if (!strcmp (stringKey, kv->key)) {
                growableBufferPrintf(&buffer, "\t%s\n", kv->value);
            }
        }
    }
    else {
        // when a plugin is present, create a context, and get the values from plugin
        ddb_dsp_context_t *ctx = dsp->open ();
        if (dsp->num_params) {
            scriptableDspInitContextFromItem (node, ctx, dsp);

            int n = ctx->plugin->num_params();
            for (int p = 0; p < n; p++) {
                char s[10];
                ctx->plugin->get_param (ctx, p, s, sizeof(s));
                growableBufferPrintf(&buffer, "\t%s\n", s);
            }
        }
        dsp->close (ctx);
    }
    return strdup(buffer.buffer);
}


static int
scriptableDspPresetSaveAtPath(scriptableItem_t *item, char *path) {
    char temp_path[PATH_MAX];
    if (snprintf (temp_path, sizeof (temp_path), "%s.tmp", path) >= sizeof (temp_path)) {
        return -1;
    }

    FILE *fp = fopen (temp_path, "w+b");
    if (!fp) {
        return -1;
    }

    for (scriptableItem_t *node = scriptableItemChildren(item); node; node = scriptableItemNext(node)) {
        const char *pluginId = scriptableItemPropertyValueForKey (node, "pluginId");
        if (!pluginId) {
            continue;
        }

        fprintf (fp, "%s {\n", pluginId);

        char *savedNode = scriptableItemSaveToString (node);
        if (savedNode) {
            fprintf (fp, "%s", savedNode);
            free (savedNode);
        }

        fprintf (fp, "}\n");
    }

    if (fclose (fp) != 0) {
        return -1;
    }

    if (!rename(temp_path, path)) {
        return 0;
    }

    (void)unlink (temp_path);

    return -1;
}

static int
scriptableDspPresetSave(scriptableItem_t *item) {
    const char *presetName = scriptableItemPropertyValueForKey (item, "name");
    if (!presetName) {
        // find the corresponding node in the current dsp chain, and update

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


    return scriptableDspPresetSaveAtPath(item, path);
}

static int
scriptableDspPresetUpdateItem (scriptableItem_t *item) {
    return scriptableItemSave(item);
}

static int
scriptableDspPresetUpdateItemForSubItem (scriptableItem_t *item, scriptableItem_t *subItem) {
    return scriptableItemSave(item);
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

static scriptableItem_t *scriptableDspCreateBlankPreset (void) {
    scriptableItem_t *item = scriptableItemAlloc();
    scriptableItemFlagsSet(item, SCRIPTABLE_FLAG_IS_LIST | SCRIPTABLE_FLAG_IS_REORDABLE);
    scriptableItemSetOverrides (item, &scriptableDspPresetCallbacks);
    return item;
}

static scriptableItem_t *
scriptableDspCreatePresetWithType (scriptableItem_t *root, const char *type) {
    // type is ignored, since there's only one preset type
    scriptableItem_t * item = scriptableDspCreateBlankPreset();
    scriptableItemSetUniqueNameUsingPrefixAndRoot (item, "New DSP Preset", root);

    return item;
}

static int
scriptableDspPresetDelete(scriptableItem_t *subItem) {
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

static int
scriptableDspRootRemoveSubItem (scriptableItem_t *item, scriptableItem_t *subItem) {
    return scriptableDspPresetDelete(subItem);
}

static int
isPresetNameAllowed (scriptableItem_t *preset, const char *name) {
    // all space or empty?
    const uint8_t *p = (const uint8_t *)name;
    for (; *p; p++) {
        if (*p != 0x20) {
            break;
        }
    }
    if (*p == 0) {
        return 0;
    }

    // starts with a dot?
    if (*name == '.') {
        return 0;
    }

    return 1;
}

scriptableItem_t *
scriptableDspRoot (scriptableItem_t *scriptableRoot) {
    scriptableItem_t *dspRoot = scriptableItemSubItemForName (scriptableRoot, "DSPPresets");
    if (!dspRoot) {
        dspRoot = scriptableItemAlloc();
        scriptableItemSetPropertyValueForKey(dspRoot, "DSPPresets", "name");
        scriptableItemAddSubItem(scriptableRoot, dspRoot);

        scriptableItem_t *passThroughDspPreset = scriptableDspCreateBlankPreset();
        scriptableItemFlagsAdd(passThroughDspPreset, SCRIPTABLE_FLAG_IS_LOADING | DDB_IS_READONLY);
        scriptableItemSetPropertyValueForKey(passThroughDspPreset, "Pass-through", "name");
        scriptableItemAddSubItem(dspRoot, passThroughDspPreset);
        scriptableItemFlagsRemove(passThroughDspPreset, SCRIPTABLE_FLAG_IS_LOADING);

        scriptableItemFlagsSet(dspRoot, SCRIPTABLE_FLAG_IS_LIST | SCRIPTABLE_FLAG_CAN_RENAME);
        scriptableItemSetOverrides(dspRoot, &scriptableDspPresetListCallbacks);
    }
    return dspRoot;
}

void
scriptableDspLoadPresets (scriptableItem_t *scriptableRoot) {
    scriptableItem_t *root = scriptableDspRoot(scriptableRoot);
    scriptableItemFlagsAdd(root, SCRIPTABLE_FLAG_IS_LOADING);

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

            scriptableItem_t *preset = scriptableDspCreateBlankPreset();
            scriptableItemFlagsAdd(preset, SCRIPTABLE_FLAG_IS_LOADING);
            scriptableItemSetUniqueNameUsingPrefixAndRoot(preset, name, root);

            if (scriptableItemLoadDspPreset (preset, namelist[i]->d_name)) {
                scriptableItemFree (preset);
            }
            else {
                scriptableItemAddSubItem(root, preset);
            }
            scriptableItemFlagsRemove(preset, SCRIPTABLE_FLAG_IS_LOADING);

            free (namelist[i]);
        }
        free (namelist);
    }
    scriptableItemFlagsRemove(root, SCRIPTABLE_FLAG_IS_LOADING);
}

static scriptableItem_t *scriptableDspCreateNodeFromContext (ddb_dsp_context_t *context) {
    scriptableItem_t *node = scriptableItemAlloc();
    scriptableItemSetOverrides(node, &scriptableDspNodeCallbacks);
    scriptableItemSetPropertyValueForKey(node, context->plugin->plugin.id, "pluginId");
    scriptableItemSetPropertyValueForKey(node, context->plugin->plugin.name, "name");
    scriptableItemSetConfigDialog(node, context->plugin->configdialog);
    return node;
}

scriptableItem_t *
scriptableDspNodeItemFromDspContext (ddb_dsp_context_t *context) {
    scriptableItem_t *node = scriptableDspCreateNodeFromContext (context);

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

static void
scriptableDspInitContextFromItem(scriptableItem_t *c, ddb_dsp_context_t *ctx, DB_dsp_t *dsp) {
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
                if (!dt.props[p].key) {
                    continue;
                }
                if (atoi(dt.props[p].key) == i) {
                    value = dt.props[p].def;
                    break;
                }
            }
        }
        assert (value);
        dsp->set_param (ctx, i, value);
    }
    const char *enabled = scriptableItemPropertyValueForKey(c, "enabled");

    // It's assumed that the property always exist,
    // but if it doesn't - it seems that enabling the node is a better idea.
    ctx->enabled = enabled ? atoi(enabled) : 1;

    if (dsp->configdialog) {
        settings_data_free(&dt);
    }
}

ddb_dsp_context_t *
scriptableDspConfigToDspChain (scriptableItem_t *item) {
    ddb_dsp_context_t *head = NULL;
    ddb_dsp_context_t *tail = NULL;
    scriptableItem_t *c = NULL;
    for (c = scriptableItemChildren(item); c != NULL; c = scriptableItemNext(c)) {
        const char *pluginId = scriptableItemPropertyValueForKey(c, "pluginId");
        DB_plugin_t *plugin = deadbeef->plug_get_for_id (pluginId);
        if (plugin->type != DB_PLUGIN_DSP) {
            break;
        }
        DB_dsp_t *dsp = (DB_dsp_t *)plugin;

        ddb_dsp_context_t *ctx = dsp->open ();
        if (dsp->num_params) {
            scriptableDspInitContextFromItem (c, ctx, dsp);

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
    scriptableItem_t *config = scriptableDspCreateBlankPreset();

    while (chain) {
        scriptableItem_t *node = scriptableDspNodeItemFromDspContext (chain);
        scriptableItemAddSubItem(config, node);
        chain = chain->next;
    }

    return config;
}

