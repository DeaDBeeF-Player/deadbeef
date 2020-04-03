#include "scriptable_encoder.h"
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include "../deadbeef.h"
#include "strdupa.h"

extern DB_functions_t *deadbeef;

static scriptableStringListItem_t *
scriptableEncoderChainItemNames (scriptableItem_t *item);

static scriptableStringListItem_t *
scriptableEncoderChainItemTypes (scriptableItem_t *item);

static scriptableItem_t *
scriptableEncoderCreatePreset (scriptableItem_t *root, const char *type);

static scriptableCallbacks_t scriptableEncoderCallbacks = {
    .readonlyPrefix = "[Built-in] ",
};

static scriptableCallbacks_t scriptableRootCallbacks = {
    .allowRenaming = 1,
    .factoryItemNames = scriptableEncoderChainItemNames,
    .factoryItemTypes = scriptableEncoderChainItemTypes,
    .createItemOfType = scriptableEncoderCreatePreset,
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

static const char *configdialog =
    "property \"File extension\" entry ext \"\";"
    "property \"Encoder command line\" entry encoder \"\";"
    "property \"Data transfer method\" select[3] method 0 \"Pipe (stdin)\" \"Temporary file\" \"Source file\";"
    "property \"ID3v2 version\" select[2] id3v2_version 0 \"2.3\" \"2.4\";"
    "property \"Write ID3v2 tag\" checkbox tag_id3v2 0;"
    "property \"Write ID3v1 tag\" checkbox tag_id3v1 0;"
    "property \"Write APEv2 tag\" checkbox tag_apev2 0;"
    "property \"Write FLAC tag\" checkbox tag_flac 0;"
    "property \"Write OggVorbis tag\" checkbox tag_oggvorbis 0;"
    "property \"Write MP4 tag\" checkbox tag_mp4 0;"
;

static scriptableStringListItem_t *
scriptableEncoderChainItemNames (scriptableItem_t *item) {
    scriptableStringListItem_t *s = scriptableStringListItemAlloc();
    s->str = strdup("EncoderPreset");
    return s;
}

static scriptableStringListItem_t *
scriptableEncoderChainItemTypes (scriptableItem_t *item) {
    scriptableStringListItem_t *s = scriptableStringListItemAlloc();
    s->str = strdup("EncoderPreset");
    return s;
}

static scriptableItem_t *
scriptableEncoderCreatePreset (scriptableItem_t *root, const char *type) {
    // type is ignored, since there's only one preset type
    scriptableItem_t * item = scriptableItemAlloc();
    scriptableItemSetUniqueNameUsingPrefixAndRoot (item, "New Encoder Preset", root);
    return item;
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

        if (!strcmp (str, "title")) {
            scriptableItemSetPropertyValueForKey(preset, item, "name");
        }
        else {
            scriptableItemSetPropertyValueForKey(preset, item, str);
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
        encoderRoot->callbacks = &scriptableRootCallbacks;
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
        const char *presetspath = preset_dirs[di];
        struct dirent **namelist = NULL;
        int n = scandir (presetspath, &namelist, scandir_preset_filter, dirent_alphasort);
        int i;
        for (i = 0; i < n; i++) {
            char s[PATH_MAX];
            if (snprintf (s, sizeof (s), "%s/%s", presetspath, namelist[i]->d_name) > 0){

                scriptableItem_t *preset = scriptableItemAlloc();
                preset->callbacks = &scriptableEncoderCallbacks;
                preset->configDialog = configdialog;
                if (scriptableItemLoadEncoderPreset (preset, namelist[i]->d_name, s)) {
                    scriptableItemFree (preset);
                }
                else {
                    if (di == 0) {
                        preset->isReadonly = 1;
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

ddb_encoder_preset_t *
scriptableEncoderPresetToConverterEncoderPreset (scriptableItem_t *item, ddb_encoder_preset_t *encoder_preset) {
    return NULL;
}
