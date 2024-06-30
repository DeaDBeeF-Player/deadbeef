/*
    SHN (Shorten) plugin for DeaDBeeF Player
    Copyright (C) 2009-2014 Oleksiy Yakovenko <waker@users.sourceforge.net>

    Based on xmms-shn, http://www.etree.org/shnutils/xmms-shn/
    Copyright (C) 2000-2007  Jason Jordan <shnutils@freeshell.org>

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

// based on xmms-shn, http://www.etree.org/shnutils/xmms-shn/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <deadbeef/deadbeef.h>
#include <deadbeef/strdupa.h>
#include "bitshift.h"
#include "shorten.h"

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(fmt, ...)

static DB_decoder_t plugin;
DB_functions_t *deadbeef;

shn_file *load_shn (const char *filename);

typedef struct {
    DB_fileinfo_t info;
    shn_file *shnfile;

    slong **buffer, **offset;
    slong lpcqoffset;
    int version, bitshift;
    int ftype;
    char *magic;
    int blocksize, nchan;
    int i, chan, nwrap, nskip;
    int *qlpc, maxnlpc, nmean;
    int cmd;
    int internal_ftype;
    int blk_size;
    int cklen;
    uchar tmp;

    int64_t currentsample;
    int64_t startsample;
    int64_t endsample;

    int skipsamples;
} shn_fileinfo_t;

shn_config shn_cfg;

DB_fileinfo_t *
shn_open (uint32_t hints) {
    shn_fileinfo_t *info = calloc (1, sizeof (shn_fileinfo_t));
    return &info->info;
}

int
ddb_getc (DB_FILE *fp) {
    uint8_t c;
    if (deadbeef->fread (&c, 1, 1, fp) != 1) {
        return EOF;
    }
    return (int)c;
}

int
init_decode_state (shn_file *this_shn) {
    if (this_shn->decode_state) {
        if (this_shn->decode_state->getbuf) {
            free (this_shn->decode_state->getbuf);
            this_shn->decode_state->getbuf = NULL;
        }

        if (this_shn->decode_state->writebuf) {
            free (this_shn->decode_state->writebuf);
            this_shn->decode_state->writebuf = NULL;
        }

        if (this_shn->decode_state->writefub) {
            free (this_shn->decode_state->writefub);
            this_shn->decode_state->writefub = NULL;
        }

        free (this_shn->decode_state);
        this_shn->decode_state = NULL;
    }

    if (!(this_shn->decode_state = malloc (sizeof (shn_decode_state)))) {
        shn_debug ("Could not allocate memory for decode state data structure");
        return 0;
    }

    this_shn->decode_state->getbuf = NULL;
    this_shn->decode_state->getbufp = NULL;
    this_shn->decode_state->nbitget = 0;
    this_shn->decode_state->nbyteget = 0;
    this_shn->decode_state->gbuffer = 0;
    this_shn->decode_state->writebuf = NULL;
    this_shn->decode_state->writefub = NULL;
    this_shn->decode_state->nwritebuf = 0;

    this_shn->vars.bytes_in_buf = 0;

    return 1;
}

int
shn_init_decoder (shn_fileinfo_t *info) {
    int version = FORMAT_VERSION;
    info->ftype = TYPE_EOF;
    info->magic = MAGIC;
    info->blocksize = DEFAULT_BLOCK_SIZE;
    info->nchan = DEFAULT_NCHAN;
    info->nskip = DEFAULT_NSKIP;
    info->maxnlpc = DEFAULT_MAXNLPC;
    info->nmean = UNDEFINED_UINT;

    info->shnfile->vars.bytes_in_buf = 0;
    if (!init_decode_state (info->shnfile)) {
        trace ("shn: init_decode_state failed\n");
        return -1;
    }
    info->shnfile->vars.going = 1;

    info->blk_size = 512 * (info->shnfile->wave_header.bits_per_sample / 8) * info->shnfile->wave_header.channels;

    /* read magic number */
#ifdef STRICT_FORMAT_COMPATABILITY
    if (FORMAT_VERSION < 2) {
        for (i = 0; i < strlen (magic); i++)
            if (getc_exit (this_shn->vars.fd) != magic[i]) {
                shn_error_fatal (this_shn, "Bad magic number");
                goto exit_thread;
            }

        /* get version number */
        version = getc_exit (this_shn->vars.fd);
    }
    else
#endif /* STRICT_FORMAT_COMPATABILITY */
    {
        int nscan = 0;

        version = MAX_VERSION + 1;
        while (version > MAX_VERSION) {
            int byte = ddb_getc (info->shnfile->vars.fd);
            if (byte == EOF) {
                shn_error_fatal (info->shnfile, "No magic number");
                trace ("shn_init: no magic number\n");
                return -1;
            }
            if (info->magic[nscan] != '\0' && byte == info->magic[nscan]) {
                nscan++;
            }
            else {
                if (info->magic[nscan] == '\0' && byte <= MAX_VERSION)
                    version = byte;
                else {
                    if (byte == info->magic[0])
                        nscan = 1;
                    else {
                        nscan = 0;
                    }
                    version = MAX_VERSION + 1;
                }
            }
        }
    }

    /* check version number */
    if (version > MAX_SUPPORTED_VERSION) {
        shn_error_fatal (info->shnfile, "Can't decode version %d", version);
        trace ("shn_init: can't decode version %d\n", version);
        return -1;
    }

    /* set up the default nmean, ignoring the command line state */
    info->nmean = (version < 2) ? DEFAULT_V0NMEAN : DEFAULT_V2NMEAN;

    /* initialise the variable length file read for the compressed stream */
    trace ("decode_state=%p\n", info->shnfile->decode_state);
    var_get_init (info->shnfile);
    if (info->shnfile->vars.fatal_error) {
        trace ("var_get_init failed\n");
        return -1;
    }

    /* initialise the fixed length file write for the uncompressed stream */
    fwrite_type_init (info->shnfile);

    /* get the internal file type */
    info->internal_ftype = UINT_GET (TYPESIZE, info->shnfile);
    trace ("internal_ftype=%d\n", info->internal_ftype);

    /* has the user requested a change in file type? */
    if (info->internal_ftype != info->ftype) {
        if (info->ftype == TYPE_EOF) {
            info->ftype = info->internal_ftype; /*  no problems here */
        }
        else { /* check that the requested conversion is valid */
            if (info->internal_ftype == TYPE_AU1 || info->internal_ftype == TYPE_AU2 ||
                info->internal_ftype == TYPE_AU3 || info->ftype == TYPE_AU1 || info->ftype == TYPE_AU2 || info->ftype == TYPE_AU3) {
                shn_error_fatal (info->shnfile, "Not able to perform requested output format conversion");
                trace ("shn_init_decoder: Not able to perform requested output format conversion\n");
                return -1;
            }
        }
        trace ("ftype=%d\n", info->ftype);
    }

    info->nchan = UINT_GET (CHANSIZE, info->shnfile);

    /* get blocksize if version > 0 */
    if (version > 0) {
        int byte;
        info->blocksize = UINT_GET ((int)(log ((double)DEFAULT_BLOCK_SIZE) / M_LN2), info->shnfile);
        info->maxnlpc = UINT_GET (LPCQSIZE, info->shnfile);
        info->nmean = UINT_GET (0, info->shnfile);
        info->nskip = UINT_GET (NSKIPSIZE, info->shnfile);
        for (int i = 0; i < info->nskip; i++) {
            byte = uvar_get (XBYTESIZE, info->shnfile);
        }
    }
    else
        info->blocksize = DEFAULT_BLOCK_SIZE;

    info->nwrap = MAX (NWRAP, info->maxnlpc);

    /* grab some space for the input buffer */
    info->buffer = long2d ((ulong)info->nchan, (ulong)(info->blocksize + info->nwrap), info->shnfile);
    if (info->shnfile->vars.fatal_error) {
        trace ("failed to alloc buffer\n");
        return -1;
    }
    info->offset = long2d ((ulong)info->nchan, (ulong)MAX (1, info->nmean), info->shnfile);
    if (info->shnfile->vars.fatal_error) {
        if (info->buffer) {
            free (info->buffer);
            info->buffer = NULL;
        }
        trace ("failed to alloc offset\n");
        return -1;
    }

    for (info->chan = 0; info->chan < info->nchan; info->chan++) {
        for (int i = 0; i < info->nwrap; i++)
            info->buffer[info->chan][i] = 0;
        info->buffer[info->chan] += info->nwrap;
    }

    if (info->maxnlpc > 0) {
        info->qlpc = (int *)pmalloc ((ulong)(info->maxnlpc * sizeof (*info->qlpc)), info->shnfile);
        if (info->shnfile->vars.fatal_error) {
            if (info->buffer) {
                free (info->buffer);
                info->buffer = NULL;
            }
            if (info->offset) {
                free (info->offset);
                info->offset = NULL;
            }
            trace ("failed to alloc qlpc\n");
            return -1;
        }
    }

    if (version > 1)
        info->lpcqoffset = V2LPCQOFFSET;

    init_offset (info->offset, info->nchan, MAX (1, info->nmean), info->internal_ftype);

    /* get commands from file and execute them */
    info->chan = 0;
    info->version = version;
    trace ("shn_init: success\n");
    return 0;
}

static void
shn_init_config (void) {
    shn_cfg.error_output_method = ERROR_OUTPUT_DEVNULL;

    deadbeef->conf_get_str ("shn.seektable_path", "", shn_cfg.seek_tables_path, sizeof (shn_cfg.seek_tables_path));
    deadbeef->conf_get_str ("shn.relative_seektable_path", "seektables", shn_cfg.relative_seek_tables_path, sizeof (shn_cfg.relative_seek_tables_path));
    shn_cfg.verbose = 0;
    shn_cfg.swap_bytes = deadbeef->conf_get_int ("shn.swap_bytes", 0);
}

int
shn_init (DB_fileinfo_t *_info, DB_playItem_t *it) {
    trace ("shn_init\n");
    shn_fileinfo_t *info = (shn_fileinfo_t *)_info;
    shn_init_config ();

    char data[4];
    DB_FILE *f;

    deadbeef->pl_lock ();
    const char *uri = strdupa (deadbeef->pl_find_meta (it, ":URI"));
    deadbeef->pl_unlock ();
    f = deadbeef->fopen (uri);
    if (!f) {
        trace ("shn: failed to open %s\n", uri);
        return -1;
    }

    int id3v2_tag_size = deadbeef->junk_get_leading_size (f);
    if (id3v2_tag_size > 0) {
        deadbeef->fseek (f, id3v2_tag_size, SEEK_SET);
    }

    if (deadbeef->fread ((void *)data, 1, 4, f) != 4) {
        deadbeef->fclose (f);
        trace ("shn: failed to read magic from %s\n", deadbeef->pl_find_meta (it, ":URI"));
        return -1;
    }
    deadbeef->fclose (f);

    if (memcmp (data, MAGIC, 4)) {
        trace ("shn: invalid MAGIC\n");
        return -1;
    }

    deadbeef->pl_lock ();
    if (!(info->shnfile = load_shn (deadbeef->pl_find_meta (it, ":URI")))) {
        deadbeef->pl_unlock ();
        trace ("shn: load_shn failed\n");
        return -1;
    }
    deadbeef->pl_unlock ();

    _info->fmt.bps = info->shnfile->wave_header.bits_per_sample;
    _info->fmt.channels = info->shnfile->wave_header.channels;
    _info->fmt.samplerate = info->shnfile->wave_header.samples_per_sec;
    for (int i = 0; i < _info->fmt.channels; i++) {
        _info->fmt.channelmask |= 1 << i;
    }
    _info->plugin = &plugin;

    int totalsamples = info->shnfile->wave_header.length * info->shnfile->wave_header.samples_per_sec;
    trace ("totalsamples: %d\n", totalsamples);

    int64_t endsample = deadbeef->pl_item_get_endsample (it);
    if (endsample > 0) {
        info->startsample = deadbeef->pl_item_get_startsample (it);
        info->endsample = endsample;
        plugin.seek_sample (_info, 0);
    }
    else {
        info->startsample = 0;
        info->endsample = totalsamples - 1;
    }

    if (info->shnfile->wave_header.file_has_id3v2_tag) {
        deadbeef->fseek (info->shnfile->vars.fd, info->shnfile->wave_header.file_has_id3v2_tag, SEEK_SET);
    }
    else {
        deadbeef->rewind (info->shnfile->vars.fd);
    }

    if (shn_init_decoder (info) < 0) {
        trace ("shn_init_decoder failed\n");
        return -1;
    }

    return 0;
}

void
shn_free_decoder (shn_fileinfo_t *info) {
    if (info->shnfile) {
        if (info->shnfile->decode_state) {
            if (info->shnfile->decode_state->writebuf != NULL) {
                free (info->shnfile->decode_state->writebuf);
                info->shnfile->decode_state->writebuf = NULL;
            }
            if (info->shnfile->decode_state->writefub != NULL) {
                free (info->shnfile->decode_state->writefub);
                info->shnfile->decode_state->writefub = NULL;
            }
        }
    }
}

void
shn_free (DB_fileinfo_t *_info) {
    shn_fileinfo_t *info = (shn_fileinfo_t *)_info;
    shn_free_decoder (info);
    if (info->shnfile) {
        shn_unload (info->shnfile);
        info->shnfile = NULL;
    }
    if (info->buffer) {
        free (info->buffer);
        info->buffer = NULL;
    }
    if (info->offset) {
        free (info->offset);
        info->offset = NULL;
    }
    if (info->maxnlpc > 0 && info->qlpc) {
        free (info->qlpc);
        info->qlpc = NULL;
    }
    free (info);
}

void
swap_bytes (shn_file *this_shn, int bytes) {
    int i;
    uchar tmp;

    for (i = 0; i < bytes; i = i + 2) {
        tmp = this_shn->vars.buffer[i + 1];
        this_shn->vars.buffer[i + 1] = this_shn->vars.buffer[i];
        this_shn->vars.buffer[i] = tmp;
    }
}

int
shn_decode (shn_fileinfo_t *info) {
    int i;
    int version = info->version;
    while (1) {
        info->cmd = uvar_get (FNSIZE, info->shnfile);
        if (info->shnfile->vars.fatal_error) {
            trace ("shn_decode: uvar_get error\n");
            return -1;
        }

        switch (info->cmd) {
            case FN_ZERO:
            case FN_DIFF0:
            case FN_DIFF1:
            case FN_DIFF2:
            case FN_DIFF3:
            case FN_QLPC: {
                slong coffset, *cbuffer = info->buffer[info->chan];
                int resn = 0, nlpc, j;

                if (info->cmd != FN_ZERO) {
                    resn = uvar_get (ENERGYSIZE, info->shnfile);
                    if (info->shnfile->vars.fatal_error) {
                        trace ("shn_decode: error 1\n");
                        return -1;
                    }
                    /* this is a hack as version 0 differed in definition of var_get */
                    if (info->version == 0)
                        resn--;
                }

                /* find mean offset : N.B. this code duplicated */
                if (info->nmean == 0)
                    coffset = info->offset[info->chan][0];
                else {
                    slong sum = (info->version < 2) ? 0 : info->nmean / 2;
                    for (i = 0; i < info->nmean; i++)
                        sum += info->offset[info->chan][i];
                    if (info->version < 2)
                        coffset = sum / info->nmean;
                    else
                        coffset = ROUNDEDSHIFTDOWN (sum / info->nmean, info->bitshift);
                }

                switch (info->cmd) {
                    case FN_ZERO:
                        for (i = 0; i < info->blocksize; i++)
                            cbuffer[i] = 0;
                        break;
                    case FN_DIFF0:
                        for (i = 0; i < info->blocksize; i++) {
                            cbuffer[i] = var_get (resn, info->shnfile) + coffset;
                            if (info->shnfile->vars.fatal_error) {
                                trace ("shn_decode: error 2\n");
                                return -1;
                            }
                        }
                        break;
                    case FN_DIFF1:
                        for (i = 0; i < info->blocksize; i++) {
                            cbuffer[i] = var_get (resn, info->shnfile) + cbuffer[i - 1];
                            if (info->shnfile->vars.fatal_error) {
                                trace ("shn_decode: error 3\n");
                                return -1;
                            }
                        }
                        break;
                    case FN_DIFF2:
                        for (i = 0; i < info->blocksize; i++) {
                            cbuffer[i] = var_get (resn, info->shnfile) + (2 * cbuffer[i - 1] - cbuffer[i - 2]);
                            if (info->shnfile->vars.fatal_error) {
                                trace ("shn_decode: error 4\n");
                                return -1;
                            }
                        }
                        break;
                    case FN_DIFF3:
                        for (i = 0; i < info->blocksize; i++) {
                            cbuffer[i] = var_get (resn, info->shnfile) + 3 * (cbuffer[i - 1] - cbuffer[i - 2]) + cbuffer[i - 3];
                            if (info->shnfile->vars.fatal_error) {
                                trace ("shn_decode: error 5\n");
                                return -1;
                            }
                        }
                        break;
                    case FN_QLPC:
                        nlpc = uvar_get (LPCQSIZE, info->shnfile);
                        if (info->shnfile->vars.fatal_error) {
                            trace ("shn_decode: error 6\n");
                            return -1;
                        }

                        for (i = 0; i < nlpc; i++) {
                            info->qlpc[i] = var_get (LPCQUANT, info->shnfile);
                            if (info->shnfile->vars.fatal_error) {
                                trace ("shn_decode: error 7\n");
                                return -1;
                            }
                        }
                        for (i = 0; i < nlpc; i++)
                            cbuffer[i - nlpc] -= coffset;
                        for (i = 0; i < info->blocksize; i++) {
                            slong sum = info->lpcqoffset;

                            for (j = 0; j < nlpc; j++)
                                sum += info->qlpc[j] * cbuffer[i - j - 1];
                            cbuffer[i] = var_get (resn, info->shnfile) + (sum >> LPCQUANT);
                            if (info->shnfile->vars.fatal_error) {
                                trace ("shn_decode: error 8\n");
                                return -1;
                            }
                        }
                        if (coffset != 0)
                            for (i = 0; i < info->blocksize; i++)
                                cbuffer[i] += coffset;
                        break;
                }

                /* store mean value if appropriate : N.B. Duplicated code */
                if (info->nmean > 0) {
                    slong sum = (info->version < 2) ? 0 : info->blocksize / 2;

                    for (i = 0; i < info->blocksize; i++)
                        sum += cbuffer[i];

                    for (i = 1; i < info->nmean; i++)
                        info->offset[info->chan][i - 1] = info->offset[info->chan][i];
                    if (info->version < 2)
                        info->offset[info->chan][info->nmean - 1] = sum / info->blocksize;
                    else
                        info->offset[info->chan][info->nmean - 1] = (sum / info->blocksize) << info->bitshift;
                }

                /* do the wrap */
                for (i = -info->nwrap; i < 0; i++)
                    cbuffer[i] = cbuffer[i + info->blocksize];

                fix_bitshift (cbuffer, info->blocksize, info->bitshift, info->internal_ftype);

                if (info->chan == info->nchan - 1) {
                    if (!info->shnfile->vars.going || info->shnfile->vars.fatal_error) {
                        trace ("shn_decode: error 9\n");
                        return -1;
                    }

                    fwrite_type (info->buffer, info->ftype, info->nchan, info->blocksize, info->shnfile);
                    info->chan = (info->chan + 1) % info->nchan;
                    // now we have buffer of size info->shnfile->vars.bytes_in_buf
                    if (shn_cfg.swap_bytes) {
                        swap_bytes (info->shnfile, info->shnfile->vars.bytes_in_buf);
                    }
                    return info->shnfile->vars.bytes_in_buf;

                    // !!!!!!!!!!!!!!!!!!FIXME
                    //                   write_and_wait(info->shnfile,blk_size);

#if 0 // seeking
                    if (info->shnfile->vars.seek_to != -1)
                    {
                        shn_seek_entry *seek_info;
                        int j;

                        shn_debug("Seeking to %d:%02d",info->shnfile->vars.seek_to/60,info->shnfile->vars.seek_to%60);

                        seek_info = shn_seek_entry_search(info->shnfile->seek_table,info->shnfile->vars.seek_to * (ulong)info->shnfile->wave_header.samples_per_sec,0,
                                (ulong)(info->shnfile->vars.seek_table_entries - 1),info->shnfile->vars.seek_resolution);

                        /* loop through number of channels in this file */
                        for (i=0;i<info->nchan;i++) {
                            /* load the three sample buffer values for this channel */
                            for (j=0;j<3;j++)
                                info->buffer[i][j-3] = shn_uchar_to_slong_le(seek_info->data+32+12*i-4*j);

                            /* load the variable number of offset history values for this channel */
                            for (j=0;j<MAX(1,info->nmean);j++)
                                info->offset[i][j]  = shn_uchar_to_slong_le(seek_info->data+48+16*i+4*j);
                        }

                        info->bitshift = shn_uchar_to_ushort_le(seek_info->data+22);

                        info->seekto_offset = shn_uchar_to_ulong_le(seek_info->data+8) + info->shnfile->vars.seek_offset;

                        deadbeef->fseek(info->shnfile->vars.fd,(slong)seekto_offset,SEEK_SET);
                        deadbeef->fread((uchar*) info->shnfile->decode_state->getbuf, 1, BUFSIZ, info->shnfile->vars.fd);

                        info->shnfile->decode_state->getbufp = info->shnfile->decode_state->getbuf + shn_uchar_to_ushort_le(seek_info->data+14);
                        info->shnfile->decode_state->nbitget = shn_uchar_to_ushort_le(seek_info->data+16);
                        info->shnfile->decode_state->nbyteget = shn_uchar_to_ushort_le(seek_info->data+12);
                        info->shnfile->decode_state->gbuffer = shn_uchar_to_ulong_le(seek_info->data+18);

                        info->shnfile->vars.bytes_in_buf = 0;

                        shn_ip.output->flush(info->shnfile->vars.seek_to * 1000);
                        info->shnfile->vars.seek_to = -1;
                    }
#endif
                }
                info->chan = (info->chan + 1) % info->nchan;
                break;
            }

            break;

            case FN_QUIT:
                /* empty out last of buffer */
                info->shnfile->vars.eof = 1;
                if (shn_cfg.swap_bytes) {
                    swap_bytes (info->shnfile, info->shnfile->vars.bytes_in_buf);
                }
                return info->shnfile->vars.bytes_in_buf;
#if 0
            write_and_wait(info->shnfile,info->shnfile->vars.bytes_in_buf);
            info->shnfile->vars.eof = 1;

            while (1)
            {
                if (!info->shnfile->vars.going)
                    goto finish;
                if (info->shnfile->vars.seek_to != -1)
                {
                    var_get_quit(info->shnfile);
                    fwrite_type_quit(info->shnfile);

                    if (buffer) free((void *) buffer);
                    if (offset) free((void *) offset);
                    if(maxnlpc > 0 && qlpc)
                        free((void *) qlpc);

                    fseek(info->shnfile->vars.fd,0,SEEK_SET);
                    goto restart;
                }
                else
                    xmms_usleep(10000);
            }

            goto cleanup;
#endif
                break;

            case FN_BLOCKSIZE:
                info->blocksize = UINT_GET ((int)(log ((double)info->blocksize) / M_LN2), info->shnfile);
                if (info->shnfile->vars.fatal_error) {
                    trace ("shn_decode: error 10\n");
                    return -1;
                }
                break;
            case FN_BITSHIFT:
                info->bitshift = uvar_get (BITSHIFTSIZE, info->shnfile);
                if (info->shnfile->vars.fatal_error) {
                    trace ("shn_decode: error 11\n");
                    return -1;
                }
                break;
            case FN_VERBATIM:
                info->cklen = uvar_get (VERBATIM_CKSIZE_SIZE, info->shnfile);
                if (info->shnfile->vars.fatal_error) {
                    trace ("shn_decode: error 12\n");
                    return -1;
                }

                while (info->cklen--) {
                    info->tmp = (uchar)uvar_get (VERBATIM_BYTE_SIZE, info->shnfile);
                    if (info->shnfile->vars.fatal_error) {
                        trace ("shn_decode: error 13\n");
                        return -1;
                    }
                }

                break;

            default:
                shn_error_fatal (info->shnfile, "Sanity check fails trying to decode function: %d", info->cmd);
                trace ("shn_decode: error 14\n");
                return -1;
        }
    }
    return 0;
}

int
shn_read (DB_fileinfo_t *_info, char *bytes, int size) {
    shn_fileinfo_t *info = (shn_fileinfo_t *)_info;
    int samplesize = _info->fmt.channels * _info->fmt.bps / 8;
    if (info->currentsample + size / samplesize > info->endsample) {
        size = (int)((info->endsample - info->currentsample + 1) * samplesize);
        if (size <= 0) {
            return 0;
        }
    }
    int initsize = size;

    while (size > 0) {
        if (info->shnfile->vars.bytes_in_buf > 0) {
            int n = size / samplesize;
            int nsamples = info->shnfile->vars.bytes_in_buf / samplesize;
            if (info->skipsamples > 0) {
                int nskip = min (nsamples, info->skipsamples);
                info->skipsamples -= nskip;
                if (nskip == nsamples) {
                    info->shnfile->vars.bytes_in_buf = 0;
                    continue;
                }
                else {
                    memmove (info->shnfile->vars.buffer, info->shnfile->vars.buffer + nskip * samplesize, info->shnfile->vars.bytes_in_buf - nskip * samplesize);
                    nsamples -= nskip;
                    continue;
                }
            }
            n = min (nsamples, n);
            char *src = (char *)info->shnfile->vars.buffer;
            memcpy (bytes, src, samplesize * n);
            src += samplesize * n;
            bytes += samplesize * n;
            size -= n * samplesize;
            if (n == info->shnfile->vars.bytes_in_buf / samplesize) {
                info->shnfile->vars.bytes_in_buf = 0;
            }
            else {
                memmove (info->shnfile->vars.buffer, src, info->shnfile->vars.bytes_in_buf - n * samplesize);
                info->shnfile->vars.bytes_in_buf -= n * samplesize;
            }
            continue;
        }
        if (shn_decode (info) <= 0) {
            trace ("shn_decode returned error\n");
            break;
        }
    }

    info->currentsample += (initsize - size) / samplesize;
    if (size != 0) {
        trace ("shn_read_int16 eof\n");
    }
    return initsize - size;
}

int
shn_seek_sample (DB_fileinfo_t *_info, int sample) {
    shn_fileinfo_t *info = (shn_fileinfo_t *)_info;

    sample += info->startsample;

    info->shnfile->vars.seek_to = sample / _info->fmt.samplerate;

    if (info->shnfile->vars.seek_table_entries == NO_SEEK_TABLE) {
        // seek by skipping samples from the start
        if (sample > info->currentsample) {
            info->skipsamples = (int)(sample - info->currentsample);
        }
        else {
            // restart
            shn_free_decoder (info);
            deadbeef->rewind (info->shnfile->vars.fd);
            if (shn_init_decoder (info) < 0) {
                return -1;
            }
            info->skipsamples = sample;
        }
        info->currentsample = info->shnfile->vars.seek_to * _info->fmt.samplerate;
        _info->readpos = info->shnfile->vars.seek_to;
        return 0;
    }

    ulong seekto_offset;
    int i, j;
    shn_seek_entry *seek_info;

    trace ("Seeking to %d:%02d\n", info->shnfile->vars.seek_to / 60, info->shnfile->vars.seek_to % 60);

    seek_info = shn_seek_entry_search (info->shnfile->seek_table, info->shnfile->vars.seek_to * (ulong)info->shnfile->wave_header.samples_per_sec, 0,
                                       (ulong)(info->shnfile->vars.seek_table_entries - 1), info->shnfile->vars.seek_resolution);

    /* loop through number of channels in this file */
    for (i = 0; i < info->nchan; i++) {
        /* load the three sample buffer values for this channel */
        for (j = 0; j < 3; j++)
            info->buffer[i][j - 3] = shn_uchar_to_slong_le (seek_info->data + 32 + 12 * i - 4 * j);

        /* load the variable number of offset history values for this channel */
        for (j = 0; j < MAX (1, info->nmean); j++)
            info->offset[i][j] = shn_uchar_to_slong_le (seek_info->data + 48 + 16 * i + 4 * j);
    }

    info->bitshift = shn_uchar_to_ushort_le (seek_info->data + 22);

    seekto_offset = (ulong)(shn_uchar_to_ulong_le (seek_info->data + 8) + info->shnfile->vars.seek_offset);

    deadbeef->fseek (info->shnfile->vars.fd, (slong)seekto_offset, SEEK_SET);
    deadbeef->fread ((uchar *)info->shnfile->decode_state->getbuf, 1, BUFSIZ, info->shnfile->vars.fd);

    info->shnfile->decode_state->getbufp = info->shnfile->decode_state->getbuf + shn_uchar_to_ushort_le (seek_info->data + 14);
    info->shnfile->decode_state->nbitget = shn_uchar_to_ushort_le (seek_info->data + 16);
    info->shnfile->decode_state->nbyteget = shn_uchar_to_ushort_le (seek_info->data + 12);
    info->shnfile->decode_state->gbuffer = shn_uchar_to_ulong_le (seek_info->data + 18);

    info->shnfile->vars.bytes_in_buf = 0;

    info->currentsample = info->shnfile->vars.seek_to * _info->fmt.samplerate;
    _info->readpos = info->shnfile->vars.seek_to;
    return 0;
}

int
shn_seek (DB_fileinfo_t *_info, float time) {
    return shn_seek_sample (_info, time * _info->fmt.samplerate);
    return 0;
}

DB_playItem_t *
shn_insert (ddb_playlist_t *plt, DB_playItem_t *after, const char *fname) {
    shn_file *tmp_file;
    DB_FILE *f;
    char data[4];

    f = deadbeef->fopen (fname);
    if (!f) {
        return NULL;
    }
    int64_t fsize = deadbeef->fgetlength (f);

    int id3v2_tag_size = deadbeef->junk_get_leading_size (f);
    if (id3v2_tag_size > 0) {
        deadbeef->fseek (f, id3v2_tag_size, SEEK_SET);
    }

    if (deadbeef->fread ((void *)data, 1, 4, f) != 4) {
        deadbeef->fclose (f);
        return NULL;
    }
    deadbeef->fclose (f);

    if (memcmp (data, MAGIC, 4)) {
        trace ("shn: invalid MAGIC\n");
        return NULL;
    }

    shn_init_config ();
    if (!(tmp_file = load_shn (fname))) {
        trace ("shn: load_shn failed\n");
        return NULL;
    }

    DB_playItem_t *it = deadbeef->pl_item_alloc_init (fname, plugin.plugin.id);
    deadbeef->pl_add_meta (it, ":FILETYPE", "Shorten");
    deadbeef->plt_set_item_duration (plt, it, tmp_file->wave_header.length);

    deadbeef->junk_apev2_read (it, tmp_file->vars.fd);
    deadbeef->junk_id3v2_read (it, tmp_file->vars.fd);
    deadbeef->junk_id3v1_read (it, tmp_file->vars.fd);

    char s[100];
    snprintf (s, sizeof (s), "%lld", (long long)fsize);
    deadbeef->pl_add_meta (it, ":FILE_SIZE", s);
    snprintf (s, sizeof (s), "%d", tmp_file->wave_header.bits_per_sample);
    deadbeef->pl_add_meta (it, ":BPS", s);
    snprintf (s, sizeof (s), "%d", tmp_file->wave_header.channels);
    deadbeef->pl_add_meta (it, ":CHANNELS", s);
    snprintf (s, sizeof (s), "%d", tmp_file->wave_header.samples_per_sec);
    deadbeef->pl_add_meta (it, ":SAMPLERATE", s);
    int br = (int)roundf (fsize / (float)tmp_file->wave_header.length * 8 / 1000);
    snprintf (s, sizeof (s), "%d", br);
    deadbeef->pl_add_meta (it, ":BITRATE", s);
    deadbeef->pl_add_meta (it, "title", NULL);

    shn_unload (tmp_file);

    after = deadbeef->plt_insert_item (plt, after, it);
    deadbeef->pl_item_unref (it);
    return after;
}

#define CAPMAXSCHAR(x) ((x > 127) ? 127 : x)
#define CAPMAXUCHAR(x) ((x > 255) ? 255 : x)
#define CAPMAXSHORT(x) ((x > 32767) ? 32767 : x)
#define CAPMAXUSHORT(x) ((x > 65535) ? 65535 : x)

static int sizeof_sample[TYPE_EOF];

void
init_sizeof_sample (void) {
    sizeof_sample[TYPE_AU1] = sizeof (uchar);
    sizeof_sample[TYPE_S8] = sizeof (schar);
    sizeof_sample[TYPE_U8] = sizeof (uchar);
    sizeof_sample[TYPE_S16HL] = sizeof (ushort);
    sizeof_sample[TYPE_U16HL] = sizeof (ushort);
    sizeof_sample[TYPE_S16LH] = sizeof (ushort);
    sizeof_sample[TYPE_U16LH] = sizeof (ushort);
    sizeof_sample[TYPE_ULAW] = sizeof (uchar);
    sizeof_sample[TYPE_AU2] = sizeof (uchar);
    sizeof_sample[TYPE_AU3] = sizeof (uchar);
    sizeof_sample[TYPE_ALAW] = sizeof (uchar);
}

/***************/
/* fixed write */
/***************/

void
fwrite_type_init (shn_file *this_shn) {
    init_sizeof_sample ();
    this_shn->decode_state->writebuf = (schar *)NULL;
    this_shn->decode_state->writefub = (schar *)NULL;
    this_shn->decode_state->nwritebuf = 0;
}

void
fwrite_type_quit (shn_file *this_shn) {
    if (this_shn->decode_state->writebuf != NULL) {
        free (this_shn->decode_state->writebuf);
        this_shn->decode_state->writebuf = NULL;
    }
    if (this_shn->decode_state->writefub != NULL) {
        free (this_shn->decode_state->writefub);
        this_shn->decode_state->writefub = NULL;
    }
}

/* convert from signed ints to a given type and write */
void
fwrite_type (slong **data, int ftype, int nchan, int nitem, shn_file *this_shn) {
    int hiloint = 1, hilo = !(*((char *)&hiloint));
    int i, nwrite = 0, datasize = sizeof_sample[ftype], chan;
    slong *data0 = data[0];
    int bufAvailable = OUT_BUFFER_SIZE - this_shn->vars.bytes_in_buf;

    if (this_shn->decode_state->nwritebuf < nchan * nitem * datasize) {
        this_shn->decode_state->nwritebuf = nchan * nitem * datasize;
        if (this_shn->decode_state->writebuf != NULL)
            free (this_shn->decode_state->writebuf);
        if (this_shn->decode_state->writefub != NULL)
            free (this_shn->decode_state->writefub);
        this_shn->decode_state->writebuf = (schar *)pmalloc ((ulong)this_shn->decode_state->nwritebuf, this_shn);
        if (!this_shn->decode_state->writebuf)
            return;
        this_shn->decode_state->writefub = (schar *)pmalloc ((ulong)this_shn->decode_state->nwritebuf, this_shn);
        if (!this_shn->decode_state->writefub)
            return;
    }

    switch (ftype) {
        case TYPE_AU1: /* leave the conversion to fix_bitshift() */
        case TYPE_AU2: {
            uchar *writebufp = (uchar *)this_shn->decode_state->writebuf;
            if (nchan == 1)
                for (i = 0; i < nitem; i++)
                    *writebufp++ = data0[i];
            else
                for (i = 0; i < nitem; i++)
                    for (chan = 0; chan < nchan; chan++)
                        *writebufp++ = data[chan][i];
            break;
        }
        case TYPE_U8: {
            uchar *writebufp = (uchar *)this_shn->decode_state->writebuf;
            if (nchan == 1)
                for (i = 0; i < nitem; i++)
                    *writebufp++ = CAPMAXUCHAR (data0[i]);
            else
                for (i = 0; i < nitem; i++)
                    for (chan = 0; chan < nchan; chan++)
                        *writebufp++ = CAPMAXUCHAR (data[chan][i]);
            break;
        }
        case TYPE_S8: {
            schar *writebufp = (schar *)this_shn->decode_state->writebuf;
            if (nchan == 1)
                for (i = 0; i < nitem; i++)
                    *writebufp++ = CAPMAXSCHAR (data0[i]);
            else
                for (i = 0; i < nitem; i++)
                    for (chan = 0; chan < nchan; chan++)
                        *writebufp++ = CAPMAXSCHAR (data[chan][i]);
            break;
        }
        case TYPE_S16HL:
        case TYPE_S16LH: {
            short *writebufp = (short *)this_shn->decode_state->writebuf;
            if (nchan == 1)
                for (i = 0; i < nitem; i++)
                    *writebufp++ = CAPMAXSHORT (data0[i]);
            else
                for (i = 0; i < nitem; i++)
                    for (chan = 0; chan < nchan; chan++)
                        *writebufp++ = CAPMAXSHORT (data[chan][i]);
            break;
        }
        case TYPE_U16HL:
        case TYPE_U16LH: {
            ushort *writebufp = (ushort *)this_shn->decode_state->writebuf;
            if (nchan == 1)
                for (i = 0; i < nitem; i++)
                    *writebufp++ = CAPMAXUSHORT (data0[i]);
            else
                for (i = 0; i < nitem; i++)
                    for (chan = 0; chan < nchan; chan++)
                        *writebufp++ = CAPMAXUSHORT (data[chan][i]);
            break;
        }
        case TYPE_ULAW: {
            uchar *writebufp = (uchar *)this_shn->decode_state->writebuf;
            if (nchan == 1)
                for (i = 0; i < nitem; i++)
                    *writebufp++ = Slinear2ulaw (CAPMAXSHORT ((data0[i] << 3)));
            else
                for (i = 0; i < nitem; i++)
                    for (chan = 0; chan < nchan; chan++)
                        *writebufp++ = Slinear2ulaw (CAPMAXSHORT ((data[chan][i] << 3)));
            break;
        }
        case TYPE_AU3: {
            uchar *writebufp = (uchar *)this_shn->decode_state->writebuf;
            if (nchan == 1)
                for (i = 0; i < nitem; i++)
                    if (data0[i] < 0)
                        *writebufp++ = (127 - data0[i]) ^ 0xd5;
                    else
                        *writebufp++ = (data0[i] + 128) ^ 0x55;
            else
                for (i = 0; i < nitem; i++)
                    for (chan = 0; chan < nchan; chan++)
                        if (data[chan][i] < 0)
                            *writebufp++ = (127 - data[chan][i]) ^ 0xd5;
                        else
                            *writebufp++ = (data[chan][i] + 128) ^ 0x55;
            break;
        }
        case TYPE_ALAW: {
            uchar *writebufp = (uchar *)this_shn->decode_state->writebuf;
            if (nchan == 1)
                for (i = 0; i < nitem; i++)
                    *writebufp++ = Slinear2alaw (CAPMAXSHORT ((data0[i] << 3)));
            else
                for (i = 0; i < nitem; i++)
                    for (chan = 0; chan < nchan; chan++)
                        *writebufp++ = Slinear2alaw (CAPMAXSHORT ((data[chan][i] << 3)));
            break;
        }
    }

    switch (ftype) {
        case TYPE_AU1:
        case TYPE_S8:
        case TYPE_U8:
        case TYPE_ULAW:
        case TYPE_AU2:
        case TYPE_AU3:
        case TYPE_ALAW:
            if (datasize * nchan * nitem <= bufAvailable) {
                memcpy ((void *)&this_shn->vars.buffer[this_shn->vars.bytes_in_buf], (const void *)this_shn->decode_state->writebuf, datasize * nchan * nitem);
                this_shn->vars.bytes_in_buf += datasize * nchan * nitem;
                nwrite = nitem;
            }
            else
                shn_debug ("Buffer overrun in fwrite_type() [case 1]: %d bytes to read, but only %d bytes are available", datasize * nchan * nitem, bufAvailable);
            break;
        case TYPE_S16HL:
        case TYPE_U16HL:
            if (hilo) {
                if (datasize * nchan * nitem <= bufAvailable) {
                    memcpy ((void *)&this_shn->vars.buffer[this_shn->vars.bytes_in_buf], (const void *)this_shn->decode_state->writebuf, datasize * nchan * nitem);
                    this_shn->vars.bytes_in_buf += datasize * nchan * nitem;
                    nwrite = nitem;
                }
                else
                    shn_debug ("Buffer overrun in fwrite_type() [case 2]: %d bytes to read, but only %d bytes are available", datasize * nchan * nitem, bufAvailable);
            }
            else {
                swab (this_shn->decode_state->writebuf, this_shn->decode_state->writefub, datasize * nchan * nitem);
                if (datasize * nchan * nitem <= bufAvailable) {
                    memcpy ((void *)&this_shn->vars.buffer[this_shn->vars.bytes_in_buf], (const void *)this_shn->decode_state->writefub, datasize * nchan * nitem);
                    this_shn->vars.bytes_in_buf += datasize * nchan * nitem;
                    nwrite = nitem;
                }
                else
                    shn_debug ("Buffer overrun in fwrite_type() [case 3]: %d bytes to read, but only %d bytes are available", datasize * nchan * nitem, bufAvailable);
            }
            break;
        case TYPE_S16LH:
        case TYPE_U16LH:
            if (hilo) {
                swab (this_shn->decode_state->writebuf, this_shn->decode_state->writefub, datasize * nchan * nitem);
                if (datasize * nchan * nitem <= bufAvailable) {
                    memcpy ((void *)&this_shn->vars.buffer[this_shn->vars.bytes_in_buf], (const void *)this_shn->decode_state->writefub, datasize * nchan * nitem);
                    this_shn->vars.bytes_in_buf += datasize * nchan * nitem;
                    nwrite = nitem;
                }
                else
                    shn_debug ("Buffer overrun in fwrite_type() [case 4]: %d bytes to read, but only %d bytes are available", datasize * nchan * nitem, bufAvailable);
            }
            else {
                if (datasize * nchan * nitem <= bufAvailable) {
                    memcpy ((void *)&this_shn->vars.buffer[this_shn->vars.bytes_in_buf], (const void *)this_shn->decode_state->writebuf, datasize * nchan * nitem);
                    this_shn->vars.bytes_in_buf += datasize * nchan * nitem;
                    nwrite = nitem;
                }
                else
                    shn_debug ("Buffer overrun in fwrite_type() [case 5]: %d bytes to read, but only %d bytes are available", datasize * nchan * nitem, bufAvailable);
            }
            break;
    }

    if (nwrite != nitem)
        shn_error_fatal (this_shn, "Failed to write decompressed stream -\npossible corrupt or truncated file");
}

/*************/
/* bitshifts */
/*************/

void
fix_bitshift (slong *buffer, int nitem, int bitshift, int ftype) {
    int i;

    if (ftype == TYPE_AU1)
        for (i = 0; i < nitem; i++)
            buffer[i] = ulaw_outward[bitshift][buffer[i] + 128];
    else if (ftype == TYPE_AU2)
        for (i = 0; i < nitem; i++) {
            if (buffer[i] >= 0)
                buffer[i] = ulaw_outward[bitshift][buffer[i] + 128];
            else if (buffer[i] == -1)
                buffer[i] = NEGATIVE_ULAW_ZERO;
            else
                buffer[i] = ulaw_outward[bitshift][buffer[i] + 129];
        }
    else if (bitshift != 0)
        for (i = 0; i < nitem; i++)
            buffer[i] <<= bitshift;
}

int
get_wave_header (shn_file *this_shn) {
    slong **buffer = NULL, **offset = NULL;
    slong lpcqoffset = 0;
    int version = FORMAT_VERSION, bitshift = 0;
    int ftype = TYPE_EOF;
    char *magic = MAGIC;
    int blocksize = DEFAULT_BLOCK_SIZE, nchan = DEFAULT_NCHAN;
    int i, chan, nwrap, nskip = DEFAULT_NSKIP;
    int *qlpc = NULL, maxnlpc = DEFAULT_MAXNLPC, nmean = UNDEFINED_UINT;
    int cmd;
    int internal_ftype;
    int cklen;
    int retval = 0;

    if (!init_decode_state (this_shn))
        return 0;

        /***********************/
        /* EXTRACT starts here */
        /***********************/

        /* read magic number */
#ifdef STRICT_FORMAT_COMPATABILITY
    if (FORMAT_VERSION < 2) {
        for (i = 0; i < strlen (magic); i++) {
            if (getc_exit (this_shn->vars.fd) != magic[i])
                return 0;
            this_shn->vars.bytes_read++;
        }

        /* get version number */
        version = getc_exit (this_shn->vars.fd);
        this_shn->vars.bytes_read++;
    }
    else
#endif /* STRICT_FORMAT_COMPATABILITY */
    {
        int nscan = 0;

        version = MAX_VERSION + 1;
        while (version > MAX_VERSION) {
            int byte = ddb_getc (this_shn->vars.fd);
            this_shn->vars.bytes_read++;
            if (byte == EOF)
                return 0;
            if (magic[nscan] != '\0' && byte == magic[nscan])
                nscan++;
            else if (magic[nscan] == '\0' && byte <= MAX_VERSION)
                version = byte;
            else {
                if (byte == magic[0])
                    nscan = 1;
                else {
                    nscan = 0;
                }
                version = MAX_VERSION + 1;
            }
        }
    }

    /* check version number */
    if (version > MAX_SUPPORTED_VERSION)
        return 0;

    /* set up the default nmean, ignoring the command line state */
    nmean = (version < 2) ? DEFAULT_V0NMEAN : DEFAULT_V2NMEAN;

    /* initialise the variable length file read for the compressed stream */
    var_get_init (this_shn);
    if (this_shn->vars.fatal_error)
        return 0;

    /* initialise the fixed length file write for the uncompressed stream */
    fwrite_type_init (this_shn);

    /* get the internal file type */
    internal_ftype = UINT_GET (TYPESIZE, this_shn);

    /* has the user requested a change in file type? */
    if (internal_ftype != ftype) {
        if (ftype == TYPE_EOF) {
            ftype = internal_ftype; /*  no problems here */
        }
        else { /* check that the requested conversion is valid */
            if (internal_ftype == TYPE_AU1 || internal_ftype == TYPE_AU2 ||
                internal_ftype == TYPE_AU3 || ftype == TYPE_AU1 || ftype == TYPE_AU2 || ftype == TYPE_AU3) {
                retval = 0;
                goto got_enough_data;
            }
        }
    }

    nchan = UINT_GET (CHANSIZE, this_shn);
    this_shn->vars.actual_nchan = nchan;

    /* get blocksize if version > 0 */
    if (version > 0) {
        int byte;
        blocksize = UINT_GET ((int)(log ((double)DEFAULT_BLOCK_SIZE) / M_LN2), this_shn);
        maxnlpc = UINT_GET (LPCQSIZE, this_shn);
        this_shn->vars.actual_maxnlpc = maxnlpc;
        nmean = UINT_GET (0, this_shn);
        this_shn->vars.actual_nmean = nmean;
        nskip = UINT_GET (NSKIPSIZE, this_shn);
        for (i = 0; i < nskip; i++) {
            byte = uvar_get (XBYTESIZE, this_shn);
        }
    }
    else
        blocksize = DEFAULT_BLOCK_SIZE;

    nwrap = MAX (NWRAP, maxnlpc);

    /* grab some space for the input buffer */
    buffer = long2d ((ulong)nchan, (ulong)(blocksize + nwrap), this_shn);
    if (this_shn->vars.fatal_error)
        return 0;
    offset = long2d ((ulong)nchan, (ulong)MAX (1, nmean), this_shn);
    if (this_shn->vars.fatal_error) {
        if (buffer) {
            free (buffer);
            buffer = NULL;
        }
        return 0;
    }

    for (chan = 0; chan < nchan; chan++) {
        for (i = 0; i < nwrap; i++)
            buffer[chan][i] = 0;
        buffer[chan] += nwrap;
    }

    if (maxnlpc > 0) {
        qlpc = (int *)pmalloc ((ulong)(maxnlpc * sizeof (*qlpc)), this_shn);
        if (this_shn->vars.fatal_error) {
            if (buffer) {
                free (buffer);
                buffer = NULL;
            }
            if (offset) {
                free (offset);
                buffer = NULL;
            }
            return 0;
        }
    }

    if (version > 1)
        lpcqoffset = V2LPCQOFFSET;

    init_offset (offset, nchan, MAX (1, nmean), internal_ftype);

    /* get commands from file and execute them */
    chan = 0;
    while (1) {
        this_shn->vars.reading_function_code = 1;
        cmd = uvar_get (FNSIZE, this_shn);
        this_shn->vars.reading_function_code = 0;

        switch (cmd) {
            case FN_ZERO:
            case FN_DIFF0:
            case FN_DIFF1:
            case FN_DIFF2:
            case FN_DIFF3:
            case FN_QLPC: {
                slong coffset, *cbuffer = buffer[chan];
                int resn = 0, nlpc, j;

                if (cmd != FN_ZERO) {
                    resn = uvar_get (ENERGYSIZE, this_shn);
                    if (this_shn->vars.fatal_error) {
                        retval = 0;
                        goto got_enough_data;
                    }
                    /* this is a hack as version 0 differed in definition of var_get */
                    if (version == 0)
                        resn--;
                }

                /* find mean offset : N.B. this code duplicated */
                if (nmean == 0)
                    coffset = offset[chan][0];
                else {
                    slong sum = (version < 2) ? 0 : nmean / 2;
                    for (i = 0; i < nmean; i++)
                        sum += offset[chan][i];
                    if (version < 2)
                        coffset = sum / nmean;
                    else
                        coffset = ROUNDEDSHIFTDOWN (sum / nmean, bitshift);
                }

                switch (cmd) {
                    case FN_ZERO:
                        for (i = 0; i < blocksize; i++)
                            cbuffer[i] = 0;
                        break;
                    case FN_DIFF0:
                        for (i = 0; i < blocksize; i++) {
                            cbuffer[i] = var_get (resn, this_shn) + coffset;
                            if (this_shn->vars.fatal_error) {
                                retval = 0;
                                goto got_enough_data;
                            }
                        }
                        break;
                    case FN_DIFF1:
                        for (i = 0; i < blocksize; i++) {
                            cbuffer[i] = var_get (resn, this_shn) + cbuffer[i - 1];
                            if (this_shn->vars.fatal_error) {
                                retval = 0;
                                goto got_enough_data;
                            }
                        }
                        break;
                    case FN_DIFF2:
                        for (i = 0; i < blocksize; i++) {
                            cbuffer[i] = var_get (resn, this_shn) + (2 * cbuffer[i - 1] - cbuffer[i - 2]);
                            if (this_shn->vars.fatal_error) {
                                retval = 0;
                                goto got_enough_data;
                            }
                        }
                        break;
                    case FN_DIFF3:
                        for (i = 0; i < blocksize; i++) {
                            cbuffer[i] = var_get (resn, this_shn) + 3 * (cbuffer[i - 1] - cbuffer[i - 2]) + cbuffer[i - 3];
                            if (this_shn->vars.fatal_error) {
                                retval = 0;
                                goto got_enough_data;
                            }
                        }
                        break;
                    case FN_QLPC:
                        nlpc = uvar_get (LPCQSIZE, this_shn);
                        if (this_shn->vars.fatal_error) {
                            retval = 0;
                            goto got_enough_data;
                        }

                        for (i = 0; i < nlpc; i++) {
                            qlpc[i] = var_get (LPCQUANT, this_shn);
                            if (this_shn->vars.fatal_error) {
                                retval = 0;
                                goto got_enough_data;
                            }
                        }
                        for (i = 0; i < nlpc; i++)
                            cbuffer[i - nlpc] -= coffset;
                        for (i = 0; i < blocksize; i++) {
                            slong sum = lpcqoffset;

                            for (j = 0; j < nlpc; j++)
                                sum += qlpc[j] * cbuffer[i - j - 1];
                            cbuffer[i] = var_get (resn, this_shn) + (sum >> LPCQUANT);
                            if (this_shn->vars.fatal_error) {
                                retval = 0;
                                goto got_enough_data;
                            }
                        }
                        if (coffset != 0)
                            for (i = 0; i < blocksize; i++)
                                cbuffer[i] += coffset;
                        break;
                }

                /* store mean value if appropriate : N.B. Duplicated code */
                if (nmean > 0) {
                    slong sum = (version < 2) ? 0 : blocksize / 2;

                    for (i = 0; i < blocksize; i++)
                        sum += cbuffer[i];

                    for (i = 1; i < nmean; i++)
                        offset[chan][i - 1] = offset[chan][i];
                    if (version < 2)
                        offset[chan][nmean - 1] = sum / blocksize;
                    else
                        offset[chan][nmean - 1] = (sum / blocksize) << bitshift;
                }

                if (0 == chan) {
                    this_shn->vars.initial_file_position = this_shn->vars.last_file_position_no_really;
                    goto got_enough_data;
                }

                /* do the wrap */
                for (i = -nwrap; i < 0; i++)
                    cbuffer[i] = cbuffer[i + blocksize];

                fix_bitshift (cbuffer, blocksize, bitshift, internal_ftype);

                if (chan == nchan - 1) {
                    fwrite_type (buffer, ftype, nchan, blocksize, this_shn);
                    this_shn->vars.bytes_in_buf = 0;
                }

                chan = (chan + 1) % nchan;
                break;
            } break;

            case FN_BLOCKSIZE:
                UINT_GET ((int)(log ((double)blocksize) / M_LN2), this_shn);
                break;

            case FN_VERBATIM:
                cklen = uvar_get (VERBATIM_CKSIZE_SIZE, this_shn);

                while (cklen--) {
                    if (this_shn->vars.bytes_in_header >= OUT_BUFFER_SIZE) {
                        shn_debug ("Unexpectedly large header - " PACKAGE " can only handle a maximum of %d bytes", OUT_BUFFER_SIZE);
                        goto got_enough_data;
                    }
                    this_shn->vars.bytes_in_buf = 0;
                    this_shn->vars.header[this_shn->vars.bytes_in_header++] = (char)uvar_get (VERBATIM_BYTE_SIZE, this_shn);
                }
                retval = 1;
                break;

            case FN_BITSHIFT:
                bitshift = uvar_get (BITSHIFTSIZE, this_shn);
                this_shn->vars.actual_bitshift = bitshift;
                break;

            default:
                goto got_enough_data;
        }
    }

got_enough_data:

    /* wind up */
    var_get_quit (this_shn);
    fwrite_type_quit (this_shn);

    if (buffer)
        free ((void *)buffer);
    if (offset)
        free ((void *)offset);
    if (maxnlpc > 0 && qlpc)
        free ((void *)qlpc);

    this_shn->vars.bytes_in_buf = 0;

    return retval;
}

void
shn_unload (shn_file *this_shn) {
    if (this_shn) {
        if (this_shn->vars.fd) {
            deadbeef->fclose (this_shn->vars.fd);
            this_shn->vars.fd = NULL;
        }

        if (this_shn->decode_state) {
            if (this_shn->decode_state->getbuf) {
                free (this_shn->decode_state->getbuf);
                this_shn->decode_state->getbuf = NULL;
            }

            if (this_shn->decode_state->writebuf) {
                free (this_shn->decode_state->writebuf);
                this_shn->decode_state->writebuf = NULL;
            }

            if (this_shn->decode_state->writefub) {
                free (this_shn->decode_state->writefub);
                this_shn->decode_state->writefub = NULL;
            }

            free (this_shn->decode_state);
            this_shn->decode_state = NULL;
        }

        if (this_shn->seek_table) {
            free (this_shn->seek_table);
            this_shn->seek_table = NULL;
        }

        free (this_shn);
        this_shn = NULL;
    }
}

shn_file *
load_shn (const char *filename) {
    shn_file *tmp_file;
    shn_seek_entry *first_seek_table;

    shn_debug ("Loading file: '%s'", filename);

    if (!(tmp_file = malloc (sizeof (shn_file)))) {
        shn_debug ("Could not allocate memory for SHN data structure");
        return NULL;
    }

    memset (tmp_file, 0, sizeof (shn_file));

    tmp_file->vars.fd = NULL;
    tmp_file->vars.seek_to = -1;
    tmp_file->vars.eof = 0;
    tmp_file->vars.going = 0;
    tmp_file->vars.seek_table_entries = NO_SEEK_TABLE;
    tmp_file->vars.bytes_in_buf = 0;
    tmp_file->vars.bytes_in_header = 0;
    tmp_file->vars.reading_function_code = 0;
    tmp_file->vars.initial_file_position = 0;
    tmp_file->vars.last_file_position = 0;
    tmp_file->vars.last_file_position_no_really = 0;
    tmp_file->vars.bytes_read = 0;
    tmp_file->vars.actual_bitshift = 0;
    tmp_file->vars.actual_maxnlpc = 0;
    tmp_file->vars.actual_nmean = 0;
    tmp_file->vars.actual_nchan = 0;
    tmp_file->vars.seek_offset = 0;

    tmp_file->decode_state = NULL;

    tmp_file->wave_header.filename = filename;
    tmp_file->wave_header.wave_format = 0;
    tmp_file->wave_header.channels = 0;
    tmp_file->wave_header.block_align = 0;
    tmp_file->wave_header.bits_per_sample = 0;
    tmp_file->wave_header.samples_per_sec = 0;
    tmp_file->wave_header.avg_bytes_per_sec = 0;
    tmp_file->wave_header.rate = 0;
    tmp_file->wave_header.header_size = 0;
    tmp_file->wave_header.data_size = 0;
    tmp_file->wave_header.file_has_id3v2_tag = 0;
    tmp_file->wave_header.id3v2_tag_size = 0;

    tmp_file->seek_header.version = NO_SEEK_TABLE;
    tmp_file->seek_header.shnFileSize = 0;

    tmp_file->seek_trailer.seekTableSize = 0;

    tmp_file->seek_table = NULL;

    tmp_file->vars.fd = deadbeef->fopen (filename);
    if (!tmp_file->vars.fd) {
        shn_debug ("Could not open file: '%s'", filename);
        shn_unload (tmp_file);
        return NULL;
    }

    tmp_file->wave_header.id3v2_tag_size = deadbeef->junk_get_leading_size (tmp_file->vars.fd);
    if (tmp_file->wave_header.id3v2_tag_size > 0) {
        tmp_file->wave_header.file_has_id3v2_tag = 2;
        trace ("found id3v2 tag, size: %d\n", tmp_file->wave_header.id3v2_tag_size);
        if (0 != deadbeef->fseek (tmp_file->vars.fd, (long)tmp_file->wave_header.id3v2_tag_size, SEEK_SET)) {
            shn_debug ("Error while discarding ID3v2 tag in file '%s'.", filename);
            deadbeef->rewind (tmp_file->vars.fd);
        }
    }

    if (0 == get_wave_header (tmp_file)) {
        shn_debug ("Unable to read WAVE header from file '%s'", filename);
        shn_unload (tmp_file);
        return NULL;
    }

    if (tmp_file->wave_header.file_has_id3v2_tag) {
        deadbeef->fseek (tmp_file->vars.fd, tmp_file->wave_header.id3v2_tag_size, SEEK_SET);
        tmp_file->vars.bytes_read += tmp_file->wave_header.id3v2_tag_size;
        tmp_file->vars.seek_offset = tmp_file->wave_header.id3v2_tag_size;
    }
    else {
        deadbeef->fseek (tmp_file->vars.fd, 0, SEEK_SET);
    }

    if (0 == shn_verify_header (tmp_file)) {
        shn_debug ("Invalid WAVE header in file: '%s'", filename);
        shn_unload (tmp_file);
        return NULL;
    }

    if (tmp_file->decode_state) {
        free (tmp_file->decode_state);
        tmp_file->decode_state = NULL;
    }

    shn_load_seek_table (tmp_file, filename);

    if (NO_SEEK_TABLE != tmp_file->vars.seek_table_entries) {
        /* verify seek tables */

        first_seek_table = (shn_seek_entry *)tmp_file->seek_table;

        if (tmp_file->vars.actual_bitshift != shn_uchar_to_ushort_le (first_seek_table->data + 22)) {
            /* initial bitshift value in the file does not match the first bitshift value of the first seektable entry - seeking is broken */
            shn_debug ("Broken seek table detected (invalid bitshift) - seeking disabled for this file.");
            tmp_file->vars.seek_table_entries = NO_SEEK_TABLE;
        }
        else if (tmp_file->vars.actual_nchan > 2) {
            /* nchan is greater than the number of such entries stored in a seek table entry - seeking won't work */
            shn_debug ("Broken seek table detected (nchan %d not in range [1 .. 2]) - seeking disabled for this file.", tmp_file->vars.actual_nchan);
            tmp_file->vars.seek_table_entries = NO_SEEK_TABLE;
        }
        else if (tmp_file->vars.actual_maxnlpc > 3) {
            /* maxnlpc is greater than the number of such entries stored in a seek table entry - seeking won't work */
            shn_debug ("Broken seek table detected (maxnlpc %d not in range [0 .. 3]) - seeking disabled for this file.", tmp_file->vars.actual_maxnlpc);
            tmp_file->vars.seek_table_entries = NO_SEEK_TABLE;
        }
        else if (tmp_file->vars.actual_nmean > 4) {
            /* nmean is greater than the number of such entries stored in a seek table entry - seeking won't work */
            shn_debug ("Broken seek table detected (nmean %d not in range [0 .. 4]) - seeking disabled for this file.", tmp_file->vars.actual_nmean);
            tmp_file->vars.seek_table_entries = NO_SEEK_TABLE;
        }
        else {
            /* seek table appears to be valid - now adjust byte offsets in seek table to match the file */
            tmp_file->vars.seek_offset += tmp_file->vars.initial_file_position - shn_uchar_to_ulong_le (first_seek_table->data + 8);

            if (0 != tmp_file->vars.seek_offset) {
                shn_debug ("Adjusting seek table offsets by %ld bytes due to mismatch between seek table values and input file - seeking might not work correctly.",
                           tmp_file->vars.seek_offset);
            }
        }
    }

    shn_debug ("Successfully loaded file: '%s'", filename);

    return tmp_file;
}

#if 0
void write_and_wait(shn_file *this_shn,int block_size)
{
	int bytes_to_write,bytes_in_block,i;

	if (this_shn->vars.bytes_in_buf < block_size)
		return;

	bytes_in_block = min(this_shn->vars.bytes_in_buf, block_size);

	if (bytes_in_block <= 0)
		return;

	bytes_to_write = bytes_in_block;
	while ((bytes_to_write + bytes_in_block) <= this_shn->vars.bytes_in_buf)
		bytes_to_write += bytes_in_block;

	shn_ip.add_vis_pcm(shn_ip.output->written_time(), (this_shn->wave_header.bits_per_sample == 16) ? FMT_S16_LE : FMT_U8,
		this_shn->wave_header.channels, bytes_to_write, this_shn->vars.buffer);

	while(shn_ip.output->buffer_free() < bytes_to_write && this_shn->vars.going && this_shn->vars.seek_to == -1)
		xmms_usleep(10000);

	if(this_shn->vars.going && this_shn->vars.seek_to == -1) {
		if (shn_cfg.swap_bytes)
			swap_bytes(this_shn, bytes_to_write);
		shn_ip.output->write_audio(this_shn->vars.buffer, bytes_to_write);
	} else
		return;

	/* shift data from end of buffer to the front */
	this_shn->vars.bytes_in_buf -= bytes_to_write;

	for(i=0;i<this_shn->vars.bytes_in_buf;i++)
		this_shn->vars.buffer[i] = this_shn->vars.buffer[i+bytes_to_write];
}
#endif

static const char *exts[] = {"shn", NULL};

static const char settings_dlg[] =
    "property \"Relative seek table path\" entry shn.relative_seektable_path seektables;\n"
    "property \"Absolute seek table path\" entry shn.seektable_path \"\";\n"
    "property \"Swap audio bytes (toggle if all you hear is static)\" checkbox shn.swap_bytes 0;\n";

// define plugin interface
static DB_decoder_t plugin = {
    DDB_PLUGIN_SET_API_VERSION
        .plugin.version_major = 1,
    .plugin.version_minor = 0,
    .plugin.type = DB_PLUGIN_DECODER,
    .plugin.id = "shn",
    .plugin.name = "Shorten player",
    .plugin.descr = "decodes shn files",
    .plugin.copyright =
        "SHN (Shorten) plugin for DeaDBeeF Player\n"
        "Copyright (C) 2009-2014 Oleksiy Yakovenko <waker@users.sourceforge.net>\n"
        "\n"
        "Based on xmms-shn, http://www.etree.org/shnutils/xmms-shn/\n"
        "Copyright (C) 2000-2007  Jason Jordan <shnutils@freeshell.org>\n"
        "\n"
        "This software is provided 'as-is', without any express or implied\n"
        "warranty.  In no event will the authors be held liable for any damages\n"
        "arising from the use of this software.\n"
        "\n"
        "Permission is granted to anyone to use this software for any purpose,\n"
        "including commercial applications, and to alter it and redistribute it\n"
        "freely, subject to the following restrictions:\n"
        "\n"
        "1. The origin of this software must not be misrepresented; you must not\n"
        " claim that you wrote the original software. If you use this software\n"
        " in a product, an acknowledgment in the product documentation would be\n"
        " appreciated but is not required.\n"
        "\n"
        "2. Altered source versions must be plainly marked as such, and must not be\n"
        " misrepresented as being the original software.\n"
        "\n"
        "3. This notice may not be removed or altered from any source distribution.\n",
    .plugin.website = "http://deadbeef.sf.net",
    .plugin.configdialog = settings_dlg,
    .open = shn_open,
    .init = shn_init,
    .free = shn_free,
    .read = shn_read,
    .seek = shn_seek,
    .seek_sample = shn_seek_sample,
    .insert = shn_insert,
    .exts = exts,
};

DB_plugin_t *
ddb_shn_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}
