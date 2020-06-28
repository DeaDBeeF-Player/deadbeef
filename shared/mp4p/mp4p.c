#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mp4p.h"

static mp4p_atom_t *
_atom_load (mp4p_atom_t *parent_atom, mp4p_file_callbacks_t *fp);

void
mp4p_atom_free (mp4p_atom_t *atom) {
    if (atom->subatoms) {
        mp4p_atom_free_list (atom->subatoms);
    }

    if (atom->free) {
        atom->free (atom->data);
    }
    free (atom);
}

void
mp4p_atom_free_list (mp4p_atom_t *atom) {
    while (atom) {
        mp4p_atom_t *next = atom->next;
        mp4p_atom_free (atom);
        atom = next;
    }
}

int
mp4p_fourcc_compare (const char *value1, const char *value2) {
    return strncmp (value1, value2, 4);
}

int
mp4p_atom_type_compare (mp4p_atom_t *atom, const char *type) {
    return mp4p_fourcc_compare(atom->type, type);
}

static int _dbg_indent = 0;

static void
_dbg_print_fourcc (const char *fourcc) {
    printf ("%c%c%c%c", fourcc[0], fourcc[1], fourcc[2], fourcc[3]);
}

static void
_dbg_print_indent (void) {
    for (int i = 0; i < _dbg_indent; i++) {
        printf (" ");
    }
}

static void
_dbg_print_atom (mp4p_atom_t *atom) {
#if 1
    _dbg_print_indent();
    _dbg_print_fourcc(atom->type);
    printf (" pos=%x size=%x", (int)atom->pos, (int)atom->size);
    printf ("\n");
#endif
}

static int
_load_subatoms (mp4p_atom_t *atom, mp4p_file_callbacks_t *fp) {
    _dbg_indent += 4;
    mp4p_atom_t *tail = NULL;
    while (fp->tell (fp) < atom->pos + atom->size) {
        mp4p_atom_t *c = _atom_load (atom, fp);
        if (!c) {
            return -1;
        }
        if (!atom->subatoms) {
            atom->subatoms = tail = c;
        }
        else {
            if (tail) {
                tail->next = c;
            }
            tail = c;
        }
    }
    _dbg_indent -= 4;
    return 0;
}

#if 0
static int
_load_fourcc_atom (mp4p_atom_t *atom, const char *expected, mp4p_file_callbacks_t *fp) {
    char fourcc[4];
    if (4 != fp->fread (fourcc, 1, 4, fp->data)) {
        return -1;
    }
    if (_str_type_compare(fourcc, expected)) {
        return -1;
    }
    return _load_subatoms (atom, fp);
}
#endif

static int
_read_uint32 (mp4p_file_callbacks_t *fp, uint32_t *value) {
    uint8_t csize[4];
    if (4 != fp->read (fp, csize, 4)) {
        return -1;
    }
    *value = (uint32_t)csize[3] | ((uint32_t)csize[2]<<8) | ((uint32_t)csize[1]<<16) | ((uint32_t)csize[0]<<24);
    return 0;
}

#define READ_UINT8(fp) ({uint8_t _temp8; if (1 != fp->read (fp, &_temp8, 1)) return -1; _temp8;})
#define READ_UINT32(fp) ({ uint32_t _temp32; if (_read_uint32 (fp, &_temp32) < 0) return -1; _temp32;})
#define READ_BUF(fp,buf,size) {if (size != fp->read(fp, buf, size)) return -1;}

// read/skip uint8 version and uint24 flags
#define READ_COMMON_HEADER() {atom_data->ch.version_flags = READ_UINT32(fp);}

#define WRITE_UINT8(x) {if (buffer_size < 1) return 0; *buffer++ = x; buffer_size--; }
#define WRITE_UINT16(x) {if (buffer_size < 2) return 0; *buffer++ = (x>>8); *buffer++ = (x & 0xff); buffer_size -= 2;}
#define WRITE_UINT32(x) {if (buffer_size < 4) return 0; *buffer++ = ((x>>24)); *buffer++ = ((x>>16)&0xff); *buffer++ = ((x>>8)&0xff); *buffer++ = (x & 0xff); buffer_size -=4 ;}
#define WRITE_BUF(buf,size) {if (buffer_size < size) return 0; if (!buf) return -1; memcpy (buffer, buf, size); buffer += size; buffer_size -= size; }
#define WRITE_COMMON_HEADER() {WRITE_UINT32(0);}

#define READ_ATOM_BUFFER(headersize) uint8_t *atombuf = (uint8_t *)malloc (headersize); if (fp->read(fp, atombuf, headersize) != headersize) { res = -1; goto error; }
#define FREE_ATOM_BUFFER() free (atombuf);

#define ATOM_DEF_INNER(atomname,headersize)\
    mp4p_##atomname##_t *atom_data = calloc (sizeof (mp4p_##atomname##_t), 1);\
    atom->data = atom_data;\
    atom->free = mp4p_##atomname##_atomdata_free;\
    atom->write = (mp4p_atom_data_write_func_t)mp4p_##atomname##_atomdata_write;\
    READ_ATOM_BUFFER(headersize);\
    res = mp4p_##atomname##_atomdata_read (atom_data, atombuf, headersize);\
    FREE_ATOM_BUFFER();


#define ATOM_DEF(atomname) else if (!mp4p_atom_type_compare(atom, #atomname)) {\
    ATOM_DEF_INNER(atomname,atom->size-8)\
}

#define ATOM_DEF_WITH_SUBATOMS(atomname,headersize) else if (!mp4p_atom_type_compare(atom, #atomname)) {\
    ATOM_DEF_INNER(atomname,headersize)\
    if (!res) {\
        atom->write_data_before_subatoms = 1;\
        res = _load_subatoms(atom, fp);\
    }\
}

#define ATOM_DEF_WITH_SUBATOMS_SYNC_ENTRY_COUNT(atomname,headersize) else if (!mp4p_atom_type_compare(atom, #atomname)) {\
    ATOM_DEF_INNER(atomname,headersize)\
    if (!res) {\
        atom->write_data_before_subatoms = 1;\
        res = _load_subatoms(atom, fp);\
        uint32_t count = mp4p_atom_subatom_count (atom);\
        if (atom_data->number_of_entries != count) {\
            atom_data->number_of_entries = count;\
        }\
    }\
}

/// Known container atoms, which can contain known sub-atoms.
/// Unknown atoms will be loaded as opaque blobs, even if they're technically containers.
static const char *container_atoms[] = {
    "moov",
    "trak",
    "mdia",
    "minf",
    "dinf",
    "stbl",
    "udta",
    "tref",
    "ilst",
    NULL
};

#define COPYRIGHT_SYM "\xa9"

int
mp4p_atom_type_invalid (mp4p_atom_t *atom) {
    for (int i = 0; i < 4; i++) {
        if (atom->type[i] <= 0 && (uint8_t)atom->type[i] != 0xa9) {
            return 1;
        }
    }
    return 0;
}

uint8_t
_adjust_varstring_len (char *buf, uint8_t len) {
    for (uint8_t i = 0; i < len; i++) {
        if (buf[i] == 0) {
            return i+1;
        }
    }
    return len;
}

uint32_t
mp4p_atom_subatom_count (mp4p_atom_t *atom) {
    uint32_t count = 0;
    for (mp4p_atom_t *subatom = atom->subatoms; subatom; subatom = subatom->next) {
        count++;
    }
    return count;
}

/// @return The function may return -1 on parser failures,
/// but this should not be considered a critical failure.
int
mp4p_atom_init (mp4p_atom_t *parent_atom, mp4p_atom_t *atom, mp4p_file_callbacks_t *fp) {
    int res = 0;

    for (int i = 0; container_atoms[i]; i++) {
        if (!mp4p_atom_type_compare (atom, container_atoms[i])) {
            return _load_subatoms(atom, fp);
        }
    }


    if (mp4p_atom_type_invalid (atom)) {
        return -1;
    }
    else if (!mp4p_atom_type_compare (atom, "ftyp")) {
        mp4p_mtyp_t *mtyp = calloc (atom->size - 8, 1);
        atom->data = mtyp;
        atom->free = free;
        READ_BUF(fp, mtyp, atom->size-8);
    }
    else if (!mp4p_atom_type_compare (atom, "mdat")) {
        return 0; // special atom, that's not loaded into memory
    }
    else if (parent_atom && !mp4p_atom_type_compare(parent_atom, "ilst")) {
        mp4p_ilst_meta_t *atom_data = calloc (1, sizeof (mp4p_ilst_meta_t));
        // custom fields contain extra "mean" and "name" subatoms
        if (!mp4p_atom_type_compare(atom, "----")) {
            atom_data->custom = 1;
        }
        atom->data = atom_data;
        atom->write = (mp4p_atom_data_write_func_t)mp4p_ilst_meta_atomdata_write;
        atom->free = mp4p_ilst_meta_atomdata_free;
        READ_ATOM_BUFFER(atom->size - 8);
        res = mp4p_ilst_meta_atomdata_read (atom_data, atombuf, atom->size - 8);
        if (res < 0) {
            // unknown? load as opaque
            mp4p_ilst_meta_atomdata_free (atom->data);
            atom->write = NULL;
            atom->free = NULL;
            atom->data = atombuf;
            res = 0;
        }
        else {
            FREE_ATOM_BUFFER();
        }
    }
    ATOM_DEF(mvhd)
    ATOM_DEF(tkhd)
    ATOM_DEF(mdhd)
    ATOM_DEF(hdlr)
    ATOM_DEF(smhd)
    ATOM_DEF_WITH_SUBATOMS_SYNC_ENTRY_COUNT(stsd,sizeof (mp4p_stsd_t))
    ATOM_DEF(stts)
    ATOM_DEF(stsc)
    ATOM_DEF(stsz)
    ATOM_DEF(stco)
    ATOM_DEF(co64)
    ATOM_DEF_WITH_SUBATOMS_SYNC_ENTRY_COUNT(dref,sizeof (mp4p_dref_t))
    ATOM_DEF(alac)
    ATOM_DEF_WITH_SUBATOMS(mp4a,28)
    ATOM_DEF_WITH_SUBATOMS(Opus,28)
    ATOM_DEF(dOps)
    ATOM_DEF(esds)
    ATOM_DEF_WITH_SUBATOMS(meta,4)
    ATOM_DEF(chpl)
    ATOM_DEF(chap)
    else {
        atom->data = malloc (atom->size - 8);
        READ_BUF(fp, atom->data, atom->size - 8);
    }

    if (!res) {
        // validate position
        off_t offs = fp->tell (fp);
        if (offs != atom->pos + atom->size) {
//            res = -1;
        }
    }

error:
    return res;
}

static mp4p_atom_t *
_atom_load (mp4p_atom_t *parent_atom, mp4p_file_callbacks_t *fp) {
    size_t fpos = fp->tell (fp);

    mp4p_atom_t *atom = calloc (sizeof (mp4p_atom_t), 1);

    atom->pos = fpos;

    if (_read_uint32 (fp, &atom->size) < 0) {
        goto error;
    }

    if (4 != fp->read (fp, &atom->type, 4)) {
        goto error;
    }

    if (mp4p_atom_init (parent_atom, atom, fp) < 0) {
        goto error;
    }

    fp->seek (fp, fpos + atom->size, SEEK_SET);

    goto success;
error:
    if (atom) {
        mp4p_atom_free (atom);
        atom = NULL;
    }
success:
    return atom;
}

mp4p_atom_t *
mp4p_open (mp4p_file_callbacks_t *callbacks) {
    mp4p_atom_t *head = NULL;
    mp4p_atom_t *tail = NULL;

    for (;;) {
        mp4p_atom_t *atom = _atom_load (NULL, callbacks);
        if (!atom) {
            break;
        }

        if (!head) {
            head = tail = atom;
        }
        else {
            tail->next = atom;
            tail = atom;
        }
    }

    return head;
}

mp4p_atom_t *
mp4p_atom_find (mp4p_atom_t *root, const char *path) {
    if (strlen (path) < 4) {
        return NULL;
    }

    mp4p_atom_t *a = root;
    while (a) {
        if (!strncmp (a->type, path, 4)) {
            break;
        }
        a = a->next;
    }
    if (a && !path[4]) {
        return a;
    }
    if (a && path[4] == '/') {
        return mp4p_atom_find (a->subatoms, path+5);
    }
    return NULL;
}

uint64_t
mp4p_stts_total_num_samples (mp4p_atom_t *stts_atom) {
    mp4p_stts_t *stts = stts_atom->data;
    if (!stts) {
        return 0;
    }
    uint64_t total = 0;
    for (uint32_t i = 0; i < stts->number_of_entries; i++) {
        total += stts->entries[i].sample_count;
    }
    return total;
}

uint32_t
mp4p_stts_sample_duration (mp4p_atom_t *stts_atom, uint32_t mp4sample) {
    mp4p_stts_t *stts = stts_atom->data;
    if (!stts) {
        return 0;
    }
    uint32_t n = 0;
    for (uint32_t i = 0; i < stts->number_of_entries; i++) {
        int nsamples = stts->entries[i].sample_count;
        while (nsamples--) {
            if (n >= mp4sample) {
                return stts->entries[i].sample_duration;
            }
            n++;
        }
    }
    return 0;
}

uint32_t
mp4p_stts_mp4sample_containing_sample (mp4p_atom_t *stts_atom, uint64_t sample, uint64_t *mp4sample_startingsample) {
    mp4p_stts_t *stts = stts_atom->data;
    if (!stts) {
        return 0;
    }
    int mp4sample = 0;
    int pos = 0;
    for (int i = 0; i < stts->number_of_entries; i++) {
        int64_t total = stts->entries[i].sample_duration * stts->entries[i].sample_count;
        if (pos + total >= sample) {
            uint32_t idx = (uint32_t)((sample - pos) / stts->entries[i].sample_duration);
            *mp4sample_startingsample = pos + idx * stts->entries[i].sample_duration;
            return mp4sample + idx;
        }
        pos += total;
        mp4sample += stts->entries[i].sample_count;
    }
    return 0;
}

uint64_t
mp4p_stts_total_sample_duration (mp4p_atom_t *stts_atom) {
    mp4p_stts_t *stts = stts_atom->data;
    if (!stts) {
        return 0;
    }
    uint64_t total = 0;
    for (uint32_t i = 0; i < stts->number_of_entries; i++) {
        total += stts->entries[i].sample_duration * stts->entries[i].sample_count;
    }
    return total;
}

uint32_t
mp4p_sample_size (mp4p_atom_t *stbl_atom, uint32_t sample)
{
    mp4p_atom_t *stsz_atom = mp4p_atom_find(stbl_atom, "stbl/stsz");
    mp4p_stsz_t *stsz = stsz_atom->data;
    if (stsz->sample_size) {
        return stsz->sample_size;
    }
    else if (sample < stsz->number_of_entries) {
        return stsz->entries[sample].sample_size;
    }
    return 0;
}

uint64_t
mp4p_sample_offset (mp4p_atom_t *stbl_atom, uint32_t sample) {
    // get chunk idx from sample (stsc table)
    mp4p_atom_t *stsc_atom = mp4p_atom_find(stbl_atom, "stbl/stsc");
    mp4p_stsc_t *stsc = stsc_atom->data;

    if (!stsc->number_of_entries) {
        return 0;
    }

    // get chunk offset (stco/co64 table)
    mp4p_atom_t *stco_atom = mp4p_atom_find(stbl_atom, "stbl/co64");
    if (!stco_atom) {
        stco_atom = mp4p_atom_find(stbl_atom, "stbl/stco");
    }

    if (!stco_atom) {
        return 0;
    }

    mp4p_stco_t *stco = stco_atom->data;

    // walk over chunk table, and find the chunk containing the sample
    uint32_t chunk = 0;
    uint32_t subchunk = 0; // repeated chunk index within stsc item

    uint32_t chunk_first_sample = 0;
    uint64_t offs = 0;

    for (;;) {
        if (chunk == stsc->number_of_entries-1) {
            // last chunk entry is repeated infinitely
            break;
        }

        if (chunk_first_sample + stsc->entries[chunk].samples_per_chunk > sample) {
            // sample belongs to "chunk"
            break;
        }

        chunk_first_sample += stsc->entries[chunk].samples_per_chunk;
        subchunk++;
        if (subchunk >= stsc->entries[chunk+1].first_chunk - stsc->entries[chunk].first_chunk) {
            subchunk = 0;
            chunk++;
        }
    }

    // skip N samples in the chunk, until we get to the needed one
    mp4p_atom_t *stsz_atom = mp4p_atom_find(stbl_atom, "stbl/stsz");
    mp4p_stsz_t *stsz = stsz_atom->data;

    offs = stco->entries[stsc->entries[chunk].first_chunk+subchunk-1].offset;
    if (stsz->sample_size) {
        offs += stsz->sample_size * (sample-chunk_first_sample);
    }
    else {
        for (int i = chunk_first_sample; i < sample; i++) {
            offs += stsz->entries[i].sample_size;
        }
    }

    return offs;
}

#define _GENRE_COUNT (sizeof(_genretbl) / sizeof (char *) - 1)
static const char *_genretbl[] = {
    "Blues",
    "Classic Rock",
    "Country",
    "Dance",
    "Disco",
    "Funk",
    "Grunge",
    "Hip-Hop",
    "Jazz",
    "Metal",
    "New Age",
    "Oldies",
    "Other",
    "Pop",
    "R&B",
    "Rap",
    "Reggae",
    "Rock",
    "Techno",
    "Industrial",
    "Alternative",
    "Ska",
    "Death Metal",
    "Pranks",
    "Soundtrack",
    "Euro-Techno",
    "Ambient",
    "Trip-Hop",
    "Vocal",
    "Jazz+Funk",
    "Fusion",
    "Trance",
    "Classical",
    "Instrumental",
    "Acid",
    "House",
    "Game",
    "Sound Clip",
    "Gospel",
    "Noise",
    "AlternRock",
    "Bass",
    "Soul",
    "Punk",
    "Space",
    "Meditative",
    "Instrumental Pop",
    "Instrumental Rock",
    "Ethnic",
    "Gothic",
    "Darkwave",
    "Techno-Industrial",
    "Electronic",
    "Pop-Folk",
    "Eurodance",
    "Dream",
    "Southern Rock",
    "Comedy",
    "Cult",
    "Gangsta",
    "Top 40",
    "Christian Rap",
    "Pop/Funk",
    "Jungle",
    "Native American",
    "Cabaret",
    "New Wave",
    "Psychedelic",
    "Rave",
    "Showtunes",
    "Trailer",
    "Lo-Fi",
    "Tribal",
    "Acid Punk",
    "Acid Jazz",
    "Polka",
    "Retro",
    "Musical",
    "Rock & Roll",
    "Hard Rock",
    "Folk",
    "Folk-Rock",
    "National Folk",
    "Swing",
    "Fast Fusion",
    "Bebob",
    "Latin",
    "Revival",
    "Celtic",
    "Bluegrass",
    "Avantgarde",
    "Gothic Rock",
    "Progressive Rock",
    "Psychedelic Rock",
    "Symphonic Rock",
    "Slow Rock",
    "Big Band",
    "Chorus",
    "Easy Listening",
    "Acoustic",
    "Humour",
    "Speech",
    "Chanson",
    "Opera",
    "Chamber Music",
    "Sonata",
    "Symphony",
    "Booty Bass",
    "Primus",
    "Porn Groove",
    "Satire",
    "Slow Jam",
    "Club",
    "Tango",
    "Samba",
    "Folklore",
    "Ballad",
    "Power Ballad",
    "Rhythmic Soul",
    "Freestyle",
    "Duet",
    "Punk Rock",
    "Drum Solo",
    "Acapella",
    "Euro-House",
    "Dance Hall",
    "Goa",
    "Drum & Bass",
    "Club-House",
    "Hardcore",
    "Terror",
    "Indie",
    "BritPop",
    "Negerpunk",
    "Polsk Punk",
    "Beat",
    "Christian Gangsta",
    "Heavy Metal",
    "Black Metal",
    "Crossover",
    "Contemporary C",
    "Christian Rock",
    "Merengue",
    "Salsa",
    "Thrash Metal",
    "Anime",
    "JPop",
    "SynthPop",
    "Abstract",
    "Art Rock",
    "Baroque",
    "Bhangra",
    "Big Beat",
    "Breakbeat",
    "Chillout",
    "Downtempo",
    "Dub",
    "EBM",
    "Eclectic",
    "Electro",
    "Electroclash",
    "Emo",
    "Experimental",
    "Garage",
    "Global",
    "IDM",
    "Illbient",
    "Industro-Goth",
    "Jam Band",
    "Krautrock",
    "Leftfield",
    "Lounge",
    "Math Rock",
    "New Romantic",
    "Nu-Breakz",
    "Post-Punk",
    "Post-Rock",
    "Psytrance",
    "Shoegaze",
    "Space Rock",
    "Trop Rock",
    "World Music",
    "Neoclassical",
    "Audiobook",
    "Audio Theatre",
    "Neue Deutsche Welle",
    "Podcast",
    "Indie Rock",
    "G-Funk",
    "Dubstep",
    "Garage Rock",
    "Psybient",
    NULL
};

const char *
mp4p_genre_name_for_index (uint16_t genreid) {
    if (genreid-1 < _GENRE_COUNT) {
        return _genretbl[genreid-1];
    }
    return NULL;
}

uint16_t
mp4p_genre_index_for_name (const char *name) {
    for (uint16_t i = 0; _genretbl[i]; i++) {
        if (!strcasecmp (name, _genretbl[i])) {
            return i+1;
        }
    }
    return 0;
}

mp4p_atom_t *
mp4p_ilst_create_custom (const char *name, const char *text) {
    mp4p_atom_t *atom = calloc (sizeof (mp4p_atom_t), 1);
    mp4p_ilst_meta_t *meta = calloc (sizeof (mp4p_ilst_meta_t), 1);
    meta->custom = 1;
    atom->data = meta;
    atom->free = mp4p_ilst_meta_atomdata_free;
    atom->write = (mp4p_atom_data_write_func_t)mp4p_ilst_meta_atomdata_write;

    memcpy (atom->type, "----", 4);
    atom->size = 8;
    atom->size += 28; // mean
    atom->size += 12 + (uint32_t)strlen(name); // name
    atom->size += 16 + (uint32_t)strlen(text); // data
    meta->name = strdup (name);
    meta->data_version_flags = 1;
    meta->text = strdup (text);
    meta->data_size = (uint32_t)strlen(text);
    return atom;
}

mp4p_atom_t *
mp4p_ilst_create_genre (const char *text) {
    mp4p_atom_t *atom = calloc (sizeof (mp4p_atom_t), 1);
    mp4p_ilst_meta_t *meta = calloc (sizeof (mp4p_ilst_meta_t), 1);
    atom->data = meta;
    atom->free = mp4p_ilst_meta_atomdata_free;
    atom->write = (mp4p_atom_data_write_func_t)mp4p_ilst_meta_atomdata_write;

    uint16_t genre_id = mp4p_genre_index_for_name (text);
    if (genre_id) {
        memcpy (atom->type, "gnre", 4);
        atom->size = 24+2;
        meta->data_version_flags = 0;
        meta->values = malloc (2);
        meta->values[0] = genre_id;
        meta->data_size = 2;
    }
    else {
        memcpy (atom->type, COPYRIGHT_SYM "gen", 4);
        atom->size = 24 + (uint32_t)strlen(text);
        meta->data_version_flags = 1;
        meta->text = strdup (text);
        meta->data_size = (uint32_t)strlen(text);
    }
    return atom;
}

mp4p_atom_t *
mp4p_ilst_create_track_disc (const char *type, uint16_t index, uint16_t total) {
    mp4p_atom_t *atom = calloc (sizeof (mp4p_atom_t), 1);
    mp4p_ilst_meta_t *meta = calloc (sizeof (mp4p_ilst_meta_t), 1);
    atom->data = meta;
    atom->free = mp4p_ilst_meta_atomdata_free;
    atom->write = (mp4p_atom_data_write_func_t)mp4p_ilst_meta_atomdata_write;
    atom->size = 32;

    memcpy (atom->type, type, 4);
    meta->data_version_flags = 0;
    meta->data_size = 8;
    meta->values = calloc (4, 2);
    meta->values[0] = 0; // FIXME: what is this?
    meta->values[1] = index;
    meta->values[2] = total;
    meta->values[3] = 0; // FIXME: what is this?
    return atom;
}

mp4p_atom_t *
mp4p_ilst_create_text (const char *type, const char *text) {
    mp4p_atom_t *atom = calloc (sizeof (mp4p_atom_t), 1);
    mp4p_ilst_meta_t *meta = calloc (sizeof (mp4p_ilst_meta_t), 1);
    atom->data = meta;
    atom->free = mp4p_ilst_meta_atomdata_free;
    atom->write = (mp4p_atom_data_write_func_t)mp4p_ilst_meta_atomdata_write;
    meta->data_size = (uint32_t)strlen(text);
    atom->size = 24+meta->data_size;

    memcpy (atom->type, type, 4);
    meta->data_version_flags = 1;
    meta->text = strdup (text);
    return atom;
}

void
mp4p_atom_remove_sibling(mp4p_atom_t *atom, mp4p_atom_t *sibling, int free) {
    mp4p_atom_t *prev = NULL;
    mp4p_atom_t *curr = atom;

    while (curr) {
        if (curr == sibling) {
            if (prev) {
                prev->next = sibling->next;
            }
            if (free) {
                mp4p_atom_free (sibling);
            }
            else {
                sibling->next = NULL;
            }
            break;
        }
        prev = curr;
        curr = curr->next;
    }
}

void
mp4p_atom_remove_subatom (mp4p_atom_t *atom, mp4p_atom_t *subatom) {
    mp4p_atom_t *c = atom->subatoms;
    mp4p_atom_t *prev = NULL;
    while (c) {
        mp4p_atom_t *next = c->next;
        if (c == subatom) {
            mp4p_atom_free (subatom);
            if (prev) {
                prev->next = next;
            }
            else {
                atom->subatoms = next;
            }
            return;
        }
        prev = c;
        c = next;
    }
}

mp4p_atom_t *
mp4p_atom_new (const char *type) {
    mp4p_atom_t *atom = calloc (sizeof (mp4p_atom_t), 1);
    memcpy (atom->type, type, 4);
    return atom;
}

// NOTE: the cloned atom's data is a direct pointer to src data.
// The downside is that the source must exist until the dest is deleted.
mp4p_atom_t *
mp4p_atom_clone (mp4p_atom_t *src) {
    mp4p_atom_t *dest = mp4p_atom_new (src->type);
    dest->pos = src->pos;
    dest->size = src->size;
    dest->write = src->write;
    // data is not duplicated, so not freeing it in the clone

    dest->write_data_before_subatoms = src->write_data_before_subatoms;

    if (dest->size > 0) {
        dest->data = src->data;
    }

    mp4p_atom_t *tail = NULL;

    if (src->subatoms) {
        dest->subatoms = mp4p_atom_clone(src->subatoms);
    }

    tail = NULL;
    mp4p_atom_t *next = src->next;
    while (next) {
        mp4p_atom_t *next_copy = mp4p_atom_clone(next);

        if (tail) {
            tail = tail->next = next_copy;
        }
        else {
            tail = dest->next = next_copy;
        }
        next = next->next;
    }

    return dest;
}

void
mp4p_atom_update_size (mp4p_atom_t *atom) {
    if (!atom->write && !atom->subatoms) {
        return;
    }
    atom->size = 8; // type+size = 8 bytes
    if (!atom->subatoms || atom->write_data_before_subatoms) {
        if (atom->write) {
            atom->size += (uint32_t)atom->write (atom->data, NULL, 0);
        }
    }
    for (mp4p_atom_t *subatom = atom->subatoms; subatom; subatom = subatom->next) {
        mp4p_atom_update_size(subatom);
        atom->size += subatom->size;
    }
}

void
mp4p_rebuild_positions (mp4p_atom_t *atom, uint64_t init_pos) {
    atom->pos = init_pos;
    if (atom->data) {
        init_pos += atom->size;
    }
    else {
        init_pos += 8;

        for (mp4p_atom_t *subatom = atom->subatoms; subatom; subatom = subatom->next) {
            mp4p_rebuild_positions(subatom, init_pos);
            init_pos += subatom->size;
        }
    }

    for (mp4p_atom_t *next = atom->next; next; next = next->next) {
        mp4p_rebuild_positions(next, init_pos);
        init_pos += next->size;
    }
}

mp4p_atom_t *
mp4p_atom_insert (mp4p_atom_t *parent, mp4p_atom_t *before, mp4p_atom_t *atom) {
    mp4p_atom_t *prev = NULL;
    mp4p_atom_t *subatom = parent->subatoms;
    while (subatom && subatom != before) {
        prev = subatom;
        subatom = subatom->next;
    }

    if (!subatom) { // `before` not found
        return NULL;
    }

    if (prev) {
        prev->next = atom;
    }
    else {
        parent->subatoms = atom;
    }
    atom->next = before;

    return atom;
}

mp4p_atom_t *
mp4p_atom_append (mp4p_atom_t *parent, mp4p_atom_t *atom) {
    mp4p_atom_t *prev = NULL;
    mp4p_atom_t *c = parent->subatoms;
    while (c) {
        prev = c;
        c = c->next;
    }
    if (prev) {
        prev->next = atom;
    }
    else {
        parent->subatoms = atom;
    }
    return atom;
}

void
mp4p_atom_dump (mp4p_atom_t *atom) {
    _dbg_print_atom(atom);
    _dbg_indent += 4;
    for (mp4p_atom_t *c = atom->subatoms; c; c = c->next) {
        mp4p_atom_dump (c);
    }
    _dbg_indent -= 4;
}

uint32_t
mp4p_atom_to_buffer (mp4p_atom_t *atom, uint8_t *buffer, uint32_t buffer_size) {
    uint32_t init_size = buffer_size;

    // calculate the size including sub-atoms
    if (atom->subatoms) {
        if (!buffer) {
            return atom->size;
        }

        WRITE_UINT32(atom->size);
        WRITE_BUF(atom->type, 4);

        if (atom->write_data_before_subatoms && atom->write) {
            size_t len = atom->write (atom->data, buffer, buffer_size);
            buffer += len;
            buffer_size -= len;
        }

        for (mp4p_atom_t *c = atom->subatoms; c; c = c->next) {
            uint32_t subsize = mp4p_atom_to_buffer (c, buffer, buffer_size);
            if (subsize != c->size) {
                break;
            }
            buffer += subsize;
            buffer_size -= subsize;
        }
    }
    else {
        if (!buffer) {
            return atom->size;
        }
        if (atom->size == 0) {
            _dbg_print_fourcc(atom->type);
            return 0;
        }
        WRITE_UINT32(atom->size);
        WRITE_BUF(atom->type, 4);
        if (!atom->write) {
            if (!memcmp (atom->type, "free", 4)) {
                size_t size = atom->size - 8;
                if (size > buffer_size) { // prevent buffer overflow - this would still cause an error
                    size = buffer_size;
                }
                memset (buffer, 0, size);
                buffer += size;
                buffer_size -= size;
            }
            else if (atom->data) {
                WRITE_BUF(atom->data, atom->size - 8);
            }
        }
        else {
            size_t written_size = atom->write (atom->data, buffer, buffer_size);
            buffer_size -= written_size;
        }
    }
    return init_size - buffer_size;
}

static int
_atom_write(mp4p_file_callbacks_t *file, mp4p_atom_t *atom) {
    uint32_t atom_size = mp4p_atom_to_buffer (atom, NULL, 0);
    uint8_t *buffer = malloc (atom_size);
    uint32_t written_size = mp4p_atom_to_buffer(atom, buffer, atom_size);
    if (written_size != atom_size) {
        free (buffer);
        return -1;
    }
    file->seek (file, atom->pos, SEEK_SET);
    if (file->write (file, buffer, atom_size) != atom_size) {
        free (buffer);
        return -1;
    }

    free (buffer);
    return 0;
}

int
mp4p_update_metadata (mp4p_file_callbacks_t *callbacks, mp4p_atom_t *mp4file) {
    // resize
    for (mp4p_atom_t *tail = mp4file; tail; tail = tail->next) {
        if (!tail->next) {
            if (callbacks->truncate (callbacks, tail->pos + tail->size) < 0) {
                return -1;
            }

        }
    }

    // write moov
    mp4p_atom_t *moov = mp4p_atom_find (mp4file, "moov");
    if (_atom_write(callbacks, moov) < 0) {
        return -1;
    }

    // write padding
    mp4p_atom_t *padding = mp4p_atom_find (mp4file, "free");
    if (padding) {
        if (_atom_write(callbacks, padding) < 0) {
            return -1;
        }
    }

    return 0;
}

int
mp4p_trak_playable (mp4p_atom_t *trak_atom) {
    const char *atom_list[] = {
        "trak/mdia/minf/stbl/stts",
        "trak/mdia/mdhd",
        "trak/mdia/minf/stbl",
        "trak/mdia/minf/stbl/stsz",
        NULL
    };

    for (int i = 0; atom_list[i]; i++) {
        if (!mp4p_atom_find(trak_atom, atom_list[i])) {
            return 0;
        }
    }

    return 1;
}

int
mp4p_trak_has_chapters (mp4p_atom_t *trak_atom) {
    const char *atom_list[] = {
        "trak/mdia/minf/stbl/stsd/text",
        "trak/tkhd",
        "trak/mdia/minf/stbl",
        "trak/mdia/minf/stbl/stts",
        "trak/mdia/minf/stbl/stsz",
        "trak/mdia/mdhd",
        NULL
    };

    for (int i = 0; atom_list[i]; i++) {
        if (!mp4p_atom_find(trak_atom, atom_list[i])) {
            return 0;
        }
    }

    return 1;
}

static void mp4p_dbg_dump_subatoms(mp4p_atom_t *atom) {
    _dbg_print_atom(atom);
    _dbg_indent += 4;
    for (mp4p_atom_t *sub = atom->subatoms; sub; sub = sub->next) {
        mp4p_dbg_dump_subatoms(sub);
    }
    _dbg_indent -= 4;
}

void
mp4p_dbg_dump_atom (mp4p_atom_t *atom) {
    for (; atom; atom = atom->next) {
        mp4p_dbg_dump_subatoms(atom);
    }
}
