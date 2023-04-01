#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <deadbeef/deadbeef.h>
#include <deadbeef/common.h>
#include "scriptable_encoder.h"
#include <deadbeef/strdupa.h>

extern DB_functions_t *deadbeef;

static scriptableStringListItem_t *
scriptableEncoderChainItemNames (scriptableItem_t *item);

static scriptableStringListItem_t *
scriptableEncoderChainItemTypes (scriptableItem_t *item);

static scriptableItem_t *
scriptableEncoderCreatePreset (scriptableItem_t *root, const char *type);

static int
scriptableEncoderPresetSave(scriptableItem_t *item);

static int
scriptableEncoderUpdateItem (struct scriptableItem_s *item);

static void
scriptableEncoderPropertyValueWillChangeForKey (struct scriptableItem_s *item, const char *key);

static int
scriptableEncoderDelete (scriptableItem_t *item);

static int
scriptableEncoderRootRemoveSubItem (scriptableItem_t *item, scriptableItem_t *subItem);

static scriptableCallbacks_t scriptableEncoderCallbacks = {
    .readonlyPrefix = "[Built-in] ",
    .save = scriptableEncoderPresetSave,
    .updateItem = scriptableEncoderUpdateItem,
    .propertyValueWillChangeForKey = scriptableEncoderPropertyValueWillChangeForKey,
};

static scriptableCallbacks_t scriptableRootCallbacks = {
    .allowRenaming = 1,
    .factoryItemNames = scriptableEncoderChainItemNames,
    .factoryItemTypes = scriptableEncoderChainItemTypes,
    .createItemOfType = scriptableEncoderCreatePreset,
    .removeSubItem = scriptableEncoderRootRemoveSubItem,
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

static scriptableItem_t *scriptableEncoderCreateBlankPreset(void) {
    scriptableItem_t *item = scriptableItemAlloc();
    item->callbacks = &scriptableEncoderCallbacks;
    return item;
}

static scriptableItem_t *
scriptableEncoderCreatePreset (scriptableItem_t *root, const char *type) {
    // type is ignored, since there's only one preset type
    scriptableItem_t * item = scriptableEncoderCreateBlankPreset();
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
        const char *item = sp + 1;

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

static int
check_dir (const char *dir, mode_t mode)
{
    char *tmp = strdup (dir);
    char *slash = tmp;
    struct stat stat_buf;
    do
    {
        slash = strstr (slash+1, "/");
        if (slash)
            *slash = 0;
        if (-1 == stat (tmp, &stat_buf))
        {
            if (0 != mkdir (tmp, mode))
            {
                trace ("Failed to create %s\n", tmp);
                free (tmp);
                return 0;
            }
        }
        if (slash)
            *slash = '/';
    } while (slash);
    free (tmp);
    return 1;
}

static int
scriptableEncoderPresetSaveAtPath(scriptableItem_t *item, const char *path) {
    char temp_path[PATH_MAX];
    if (snprintf (temp_path, sizeof (temp_path), "%s.tmp", path) >= sizeof (temp_path)) {
        return -1;
    }

    char *dir_path = strdup (path);
    char *slash = strrchr (dir_path, '/');
    if (slash) {
        while (slash >= dir_path && *slash == '/') {
            *slash-- = 0;
        }

        if (slash > dir_path) {
            check_dir(dir_path, 0755);
        }
    }
    free (dir_path);

    FILE *fp = fopen (temp_path, "w+b");
    if (!fp) {
        return -1;
    }

    fprintf (fp, "title %s\n", scriptableItemPropertyValueForKey(item, "name"));
    fprintf (fp, "ext %s\n", scriptableItemPropertyValueForKey(item, "ext"));
    fprintf (fp, "encoder %s\n", scriptableItemPropertyValueForKey(item, "encoder"));
    fprintf (fp, "method %s\n", scriptableItemPropertyValueForKey(item, "method"));
    fprintf (fp, "id3v2_version %s\n", scriptableItemPropertyValueForKey(item, "id3v2_version"));
    fprintf (fp, "tag_id3v2 %s\n", scriptableItemPropertyValueForKey(item, "tag_id3v2"));
    fprintf (fp, "tag_id3v1 %s\n", scriptableItemPropertyValueForKey(item, "tag_id3v1"));
    fprintf (fp, "tag_apev2 %s\n", scriptableItemPropertyValueForKey(item, "tag_apev2"));
    fprintf (fp, "tag_flac %s\n", scriptableItemPropertyValueForKey(item, "tag_flac"));
    fprintf (fp, "tag_oggvorbis %s\n", scriptableItemPropertyValueForKey(item, "tag_oggvorbis"));
    fprintf (fp, "tag_mp4 %s\n", scriptableItemPropertyValueForKey(item, "tag_mp4"));


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
getEncoderPresetFullPathName (const char *fname, char *path, size_t size) {
    int res = snprintf (path, size, "%s/presets/encoders/%s", deadbeef->get_system_dir (DDB_SYS_DIR_CONFIG), fname);
    return res < size ? 0 : -1;
}


static int
scriptableEncoderPresetSave(scriptableItem_t *item) {
    const char *presetName = scriptableItemPropertyValueForKey (item, "name");
    if (!presetName) {
        return -1;
    }

    char fname[PATH_MAX];
    if (snprintf (fname, sizeof (fname), "%s.txt", presetName) >= sizeof (fname)) {
        return -1;
    }

    char path[PATH_MAX];
    if (getEncoderPresetFullPathName (fname, path, sizeof (path)) < 0) {
        return -1;
    }


    return scriptableEncoderPresetSaveAtPath(item, path);
}

static int
scriptableEncoderUpdateItem (struct scriptableItem_s *item) {
    return scriptableItemSave (item);
}

static void
scriptableEncoderPropertyValueWillChangeForKey (struct scriptableItem_s *item, const char *key) {
    if (!strcmp (key, "name")) {
        // FIXME: this deletes the preset during rename.
        // If the next save operation fails - data loss will occur.
        scriptableEncoderDelete(item);
    }
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

static int
scriptableEncoderDelete (scriptableItem_t *item) {
    const char *name = scriptableItemPropertyValueForKey(item, "name");
    if (!name) {
        return -1;
    }

    char fname[PATH_MAX];
    if (snprintf (fname, sizeof (fname), "%s.txt", name) >= sizeof (fname)) {
        return -1;
    }

    char path[PATH_MAX];
    if (getEncoderPresetFullPathName (fname, path, sizeof (path)) < 0) {
        return -1;
    }

    return unlink (path);
}

static int
scriptableEncoderRootRemoveSubItem (scriptableItem_t *item, scriptableItem_t *subItem) {
    return scriptableEncoderDelete (subItem);
}

void
scriptableEncoderLoadPresets (void) {
    scriptableItem_t *root = scriptableEncoderRoot();
    root->isLoading = 1;

    char path[PATH_MAX];
    if (snprintf (path, sizeof (path), "%s/presets/encoders", deadbeef->get_system_dir (DDB_SYS_DIR_CONFIG)) < 0) {
        path[0] = 0;
    }

    char syspath[PATH_MAX];
    if (snprintf (syspath, sizeof (syspath), "%s/convpresets", deadbeef->get_system_dir (DDB_SYS_DIR_PLUGIN_RESOURCES)) < 0) {
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

                scriptableItem_t *preset = scriptableEncoderCreateBlankPreset ();
                preset->isLoading = 1;
                preset->configDialog = configdialog;
                if (scriptableItemLoadEncoderPreset (preset, namelist[i]->d_name, s)) {
                    scriptableItemFree (preset);
                }
                else {
                    if (di == 0) {
                        preset->isReadonly = 1;
                    }
                    scriptableItemAddSubItem(root, preset);
                }
                preset->isLoading = 0;
            }
        }
        for (i = 0; i < n; i++) {
            free (namelist[i]);
        }
        free (namelist);
        namelist = NULL;
    }

    root->isLoading = 0;
}

static const char *
valueForKeyOrDefault (scriptableItem_t *item, const char *key, const char *def) {
    return scriptableItemPropertyValueForKey(item, key) ?: def;
}

void
scriptableEncoderPresetToConverterEncoderPreset (scriptableItem_t *item, ddb_encoder_preset_t *encoder_preset) {
    memset (encoder_preset, 0, sizeof (ddb_encoder_preset_t));
    encoder_preset->ext = strdup (valueForKeyOrDefault(item, "ext", ""));
    encoder_preset->encoder = strdup (valueForKeyOrDefault(item, "encoder", ""));
    encoder_preset->method = atoi (valueForKeyOrDefault(item, "method", "0"));
    encoder_preset->tag_id3v2 = atoi (valueForKeyOrDefault(item, "tag_id3v2", "0"));
    encoder_preset->tag_id3v1 = atoi (valueForKeyOrDefault(item, "tag_id3v1", "0"));
    encoder_preset->tag_apev2 = atoi (valueForKeyOrDefault(item, "tag_apev2", "0"));
    encoder_preset->tag_flac = atoi (valueForKeyOrDefault(item, "tag_flac", "0"));
    encoder_preset->tag_oggvorbis = atoi (valueForKeyOrDefault(item, "tag_oggvorbis", "0"));
    encoder_preset->tag_mp4 = atoi (valueForKeyOrDefault(item, "tag_mp4", "0"));
    encoder_preset->id3v2_version = atoi (valueForKeyOrDefault(item, "id3v2_version", "0"));
}
