/*
  DeaDBeeF -- the music player
  Copyright (C) 2009-2022 Oleksiy Yakovenko and other contributors

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
  claim that you wrote the original software. If you use this software
  in a product, an acknowledgment in the product documentation would be
  appreciated but is not required.

  2. Altered source versions must be plainly marked as such, and must not be
  misrepresented as being the original software.

  3. This notice may not be removed or altered from any source distribution.
*/

/* Opus artwork decoder, based on `artwork_ogg.c'. Made in 2025 by
   Emil Kosz. */

#include <stdlib.h>
#include <string.h>
#include <opus/opusfile.h>
#include <deadbeef/deadbeef.h>

#include "artwork_opus.h"
#include "artwork_internal.h"
#include "base64.h"

#include <stdio.h>

#define trace(...) { deadbeef->log_detailed (&plugin.plugin.plugin, 0, __VA_ARGS__); }

extern DB_functions_t *deadbeef;
extern ddb_artwork_plugin_t plugin;
extern int artwork_disable_cache;

// ---

static int
copus_fread (void *stream, unsigned char *ptr, int nbytes) {
  // It wants the number of bytes. So sizeof(char) should be the size
  // of a byte, no?
  size_t ret = deadbeef->fread (ptr, sizeof(char), nbytes, stream);
  //    trace ("copus_fread %d %d %d\n", size, nmemb, ret);
  return ret;
}

static int
copus_fseek (void *stream, opus_int64 offset, int whence) {
  DB_FILE *f = (DB_FILE *)stream;
  return deadbeef->fseek (f, offset, whence);
}

static int
copus_fclose (void *stream) {
  deadbeef->fclose (stream);
  return 0;
}

static opus_int64
copus_ftell (void *stream) {
  return deadbeef->ftell (stream);
}

// ---

#define READ_UINT32() ({if (buffer_size < 4) goto error_decode_flac_block;  uint32_t _temp32 = (uint32_t)buffer[3] | ((uint32_t)buffer[2]<<8) | ((uint32_t)buffer[1]<<16) | ((uint32_t)buffer[0]<<24); buffer+=4; buffer_size-=4; _temp32;})

#define READ_BUF(buf,size) {if (buffer_size < size) goto error_decode_flac_block; memcpy (buf, buffer, size); buffer += size; buffer_size -= size; }

// ---

int
opus_extract_art (ddb_cover_info_t *cover) {
  int err = -1;
  if (!strcasestr (cover->priv->filepath, ".opus")) {
    return -1;
  }

  DB_FILE *fp = NULL;

  fp = deadbeef->fopen (cover->priv->filepath);
  if (!fp) {
    trace ("opus_extract_art: failed to fopen %s\n", cover->priv->filepath);
    return -1;
  }
  if (fp->vfs->is_streaming ()) {
    trace ("opus_extract_art: failed to fopen %s\n", cover->priv->filepath);
    deadbeef->fclose (fp);
    return -1;
  }
  OpusFileCallbacks ofc = {
    .read = copus_fread,
    .seek = copus_fseek,
    .tell = copus_ftell,
    .close = copus_fclose
  };
  int res = -1;
  OggOpusFile *opus_file = op_open_callbacks
    (fp, &ofc, NULL, 0, &res);
  if (res != 0) {
    trace ("opus_extract_art: op_open_callbacks returned: %d\n", res);
    deadbeef->fclose (fp);
    return -1;
  }

  // Now that we correctly open the Opus file, it's time to extract
  // the comments. They are the same as in Ogg Vorbis, and in fact can
  // be cast freely to `vorbis_comment' structs.
  const OpusTags *ot = op_tags (opus_file, 0);
  if (!ot) {
    trace ("opus_extract_art: op_tags failed\n");
    goto error;
  }

  // TODO: exactly the same as in `artwork_ogg.c'
  
  const char key[] = "METADATA_BLOCK_PICTURE=";
  for (int i = 0; i < ot->comments; i++) {
    uint8_t *decoded_blob = NULL;
    char *mime_type = NULL;
    char *descr = NULL;
    int size = ot->comment_lengths[i];
    if (ot->comment_lengths[i] <= sizeof(key)-1
	|| strncasecmp(ot->user_comments[i], key, sizeof (key)-1)) {
      continue;
    }
    trace ("opus_extract_art: found cover art of %d bytes\n", size);

    // decode base64
    char *blob = ot->user_comments[i] + sizeof (key) - 1;
    const int predicted_decoded_size = Base64decode_len(blob);

    if (predicted_decoded_size <= 0) {
      continue;
    }

    decoded_blob = malloc (predicted_decoded_size);
    if (decoded_blob == NULL) {
      goto error_decode_flac_block;
    }
    const int decoded_size = Base64decode((char *)decoded_blob, blob);

    // decode flac picture block
    uint8_t *buffer = decoded_blob;
    int buffer_size = decoded_size;

    /*int32_t picture_type = */READ_UINT32();
    int32_t mime_size = READ_UINT32();
    mime_type = calloc(1, mime_size + 1);
    READ_BUF(mime_type, mime_size);
    mime_type[mime_size] = 0;

    // skip non-image data
    if (strcasecmp(mime_type, "image/")
	&& strcasecmp(mime_type, "image/png")
	&& strcasecmp(mime_type, "image/jpeg")) {
      goto error_decode_flac_block; // unsupported mime type
    }

    free (mime_type);
    mime_type = NULL;
    int32_t descr_size = READ_UINT32();
    descr = calloc(1, descr_size + 1);
    READ_BUF(descr, descr_size);
    descr[descr_size] = 0;
    free (descr);
    descr = NULL;
    /*uint32_t width = */READ_UINT32();
    /*uint32_t height = */READ_UINT32();
    /*uint32_t depth = */READ_UINT32();
    /*uint32_t palette_size = */READ_UINT32();
    uint32_t picture_data_len = READ_UINT32();

    cover->priv->blob_size = picture_data_len;
    cover->priv->blob_image_size = picture_data_len;
    cover->priv->blob = (char *)decoded_blob;
    cover->priv->blob_image_offset = buffer - decoded_blob;
    err = 0;
    break;
  error_decode_flac_block:
    free (decoded_blob);
    free (mime_type);
    free (descr);
    continue;
  }

 error:
  op_free (opus_file);
  return err;
}
