/*
    $Id: cddb_disc.h,v 1.22 2007/08/07 03:12:53 jcaratzas Exp $

    Copyright (C) 2003, 2004, 2005 Kris Verbeeck <airborne@advalvas.be>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the
    Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA  02111-1307, USA.
*/

#ifndef CDDB_DISC_H
#define CDDB_DISC_H 1

#ifdef __cplusplus
    extern "C" {
#endif


#include <cddb/cddb_track.h>


/**
 * The number of frames that fit into one second.
 */
#define FRAMES_PER_SECOND 75

/**
 * This macro converts an amount of frames into an amount of seconds.
 */
#define FRAMES_TO_SECONDS(f) ((f) / FRAMES_PER_SECOND)

/**
 * This macro converts an amount of seconds into an amount of frames.
 */
#define SECONDS_TO_FRAMES(s) ((s) * FRAMES_PER_SECOND)

/**
 * The different CDDB categories.
 */
typedef enum {
    CDDB_CAT_DATA = 0,          /**< data disc */
    CDDB_CAT_FOLK,              /**< folk music */
    CDDB_CAT_JAZZ,              /**< jazz music */
    CDDB_CAT_MISC,              /**< miscellaneous, use if no other
                                     category matches */
    CDDB_CAT_ROCK,              /**< rock and pop music */
    CDDB_CAT_COUNTRY,           /**< country music */
    CDDB_CAT_BLUES,             /**< blues music */
    CDDB_CAT_NEWAGE,            /**< new age music */
    CDDB_CAT_REGGAE,            /**< reggae music */
    CDDB_CAT_CLASSICAL,         /**< classical music */
    CDDB_CAT_SOUNDTRACK,        /**< soundtracks */
    CDDB_CAT_INVALID,           /**< (internal) invalid category */
    CDDB_CAT_LAST               /**< (internal) category counter */
} cddb_cat_t;

/**
 * String values for the CDDB categories.
 */
extern const char *CDDB_CATEGORY[CDDB_CAT_LAST];

/**
 * The CDDB disc structure.  Contains all information associated with
 * a full CD.
 */
typedef struct cddb_disc_s cddb_disc_t;


/* --- construction / destruction */


/**
 * Creates a new CDDB disc structure.
 *
 * @return The CDDB disc structure or NULL if memory allocation failed.
 */
cddb_disc_t *cddb_disc_new(void);

/**
 * Free all resources associated with the given CDDB disc structure.
 * The tracks will also be freed automatically.
 *
 * @param disc The CDDB disc structure.
 */
void cddb_disc_destroy(cddb_disc_t *disc);

/**
 * Creates a clone of the given disc.
 *
 * @param disc The CDDB disc structure.
 */
cddb_disc_t *cddb_disc_clone(const cddb_disc_t *disc);


/* --- track manipulation */


/**
 * Add a new track to a disc.  The track is added to the end of the
 * existing list of tracks.
 *
 * @param disc The CDDB disc structure.
 * @param track The CDDB track structure.
 */
void cddb_disc_add_track(cddb_disc_t *disc, cddb_track_t *track);

/**
 * Retrieves a numbered track from the disc.  If there is no track
 * with the given number, then NULL will be returned.
 *
 * @param disc The CDDB disc structure.
 * @param track_no The track number; starting at 0.
 */
cddb_track_t *cddb_disc_get_track(const cddb_disc_t *disc, int track_no);

/**
 * Returns the first track of the disc.  If there is no such track
 * then NULL will be returned.  The internal track iterator will also
 * be reset.  This function should be called before the first call to
 * cddb_disc_get_track_next.
 *
 * @see cddb_disc_get_track_next
 *
 * @param disc The CDDB disc structure.
 */
cddb_track_t *cddb_disc_get_track_first(cddb_disc_t *disc);

/**
 * Returns the next track on the disc and advances the internal track
 * iterator.  If there is no such track then NULL will be returned.
 * This function should be called after calling
 * cddb_disc_get_track_first.
 *
 * @see cddb_disc_get_track_first
 *
 * @param disc The CDDB disc structure.
 */
cddb_track_t *cddb_disc_get_track_next(cddb_disc_t *disc);


/* --- setters / getters --- */


/**
 * Get the ID of the disc.  If the disc is invalid or the disc ID is
 * not yet initialized 0 will be returned.
 *
 * @param disc The CDDB disc structure.
 */
unsigned int cddb_disc_get_discid(const cddb_disc_t *disc);

/**
 * Set the ID of the disc.  When the disc ID is not known yet, then it
 * can be calculated with the cddb_disc_calc_discid function (which
 * will automatically initialize the correct field in the disc
 * structure).
 *
 * @see cddb_disc_calc_discid
 *
 * @param disc The CDDB disc structure.
 * @param id The disc ID.
 */
void cddb_disc_set_discid(cddb_disc_t *disc, unsigned int id);

/**
 * Get the disc CDDB category ID.  If the disc is invalid or no
 * category is set then CDDB_CAT_INVALID will be returned.  If you
 * want a string representation of the category use the
 * cddb_disc_get_category_str function.
 *
 * @see cddb_disc_set_category
 * @see cddb_disc_get_category_str
 * @see cddb_disc_set_category_str
 * @see cddb_cat_t
 * @see CDDB_CATEGORY
 *
 * @param disc The CDDB disc structure.
 * @return The CDDB category ID.
 */
cddb_cat_t cddb_disc_get_category(const cddb_disc_t *disc);

/**
 * Set the disc CDDB category ID.
 *
 * @see cddb_disc_get_category
 * @see cddb_disc_get_category_str
 * @see cddb_disc_set_category_str
 * @see cddb_cat_t
 * @see CDDB_CATEGORY
 *
 * @param disc The CDDB disc structure.
 * @param cat  The CDDB category ID.
 */
void cddb_disc_set_category(cddb_disc_t *disc, cddb_cat_t cat);

/**
 * Get the disc CDDB category as a string.  If no category is set for
 * this disc then 'invalid' will be returned.  If the disc structure
 * is invalid NULL is returned.  If you only want the ID of the
 * category use the cddb_disc_get_category function.
 *
 * @see cddb_disc_get_category
 * @see cddb_disc_set_category
 * @see cddb_disc_set_category_str
 *
 * @param disc The CDDB disc structure.
 * @return The CDDB category ID.
 */
const char *cddb_disc_get_category_str(cddb_disc_t *disc);

/**
 * Sets the category of the disc.  If the specified category is
 * an invalid CDDB category, then CDDB_CAT_MISC will be used.
 *
 * @see cddb_disc_get_category
 * @see cddb_disc_set_category
 * @see cddb_disc_get_category_str
 * @see CDDB_CATEGORY
 *
 * @param disc The CDDB disc structure.
 * @param cat The category string.
 */
void cddb_disc_set_category_str(cddb_disc_t *disc, const char *cat);

/**
 * Get the disc genre.  If no genre is set for this disc then NULL
 * will be returned.  As opposed to the disc category, this field is
 * not limited to a predefined set.
 *
 * @param disc The CDDB disc structure.
 * @return The disc genre.
 */
const char *cddb_disc_get_genre(const cddb_disc_t *disc);

/**
 * Set the disc genre.  As opposed to the disc category, this field is
 * not limited to a predefined set.  If the disc already had a genre,
 * then the memory for that string will be freed.  The new genre will
 * be copied into a new chunk of memory.
 *
 * @see cddb_disc_get_category_str
 *
 * @param disc  The CDDB disc structure.
 * @param genre The disc genre.
 */
void cddb_disc_set_genre(cddb_disc_t *disc, const char *genre);

/**
 * Get the disc length.  If no length is set for this disc then 0 will
 * be returned.
 *
 * @param disc The CDDB disc structure.
 * @return The disc length in seconds.
 */
unsigned int cddb_disc_get_length(const cddb_disc_t *disc);

/**
 * Set the disc length.
 *
 * @param disc The CDDB disc structure.
 * @param l    The disc length in seconds.
 */
void cddb_disc_set_length(cddb_disc_t *disc, unsigned int l);

/**
 * Get the revision number of the disc.
 *
 * @param disc The CDDB disc structure.
 */
unsigned int cddb_disc_get_revision(const cddb_disc_t *disc);

/**
 * Set the revision number of the disc.
 *
 * @param disc The CDDB disc structure.
 * @param rev The revision number.
 */
void cddb_disc_set_revision(cddb_disc_t *disc, unsigned int rev);

/**
 * Get the year of publication for this disc.  If no year is defined 0
 * is returned.
 *
 * @param disc The CDDB disc structure.
 * @return The disc year.
 */
unsigned int cddb_disc_get_year(const cddb_disc_t *disc);

/**
 * Set the year of publication for this disc.
 *
 * @param disc The CDDB disc structure.
 * @param y    The disc year.
 */
void cddb_disc_set_year(cddb_disc_t *disc, unsigned int y);

/**
 * Get the number of tracks on the disc.  If the disc is invalid -1 is
 * returned.
 *
 * @param disc The CDDB disc structure.
 * @return The number of tracks.
 */
int cddb_disc_get_track_count(const cddb_disc_t *disc);

/**
 * Get the disc title.  If the disc is invalid or no title is set then
 * NULL will be returned.
 *
 * @param disc The CDDB disc structure.
 * @return The disc title.
 */
const char *cddb_disc_get_title(const cddb_disc_t *disc);

/**
 * Set the disc title.  If the disc already had a title, then the
 * memory for that string will be freed.  The new title will be copied
 * into a new chunk of memory.  If the given title is NULL, then the
 * title of the disc will be deleted.
 *
 * @param disc The CDDB disc structure.
 * @param title The new disc title.
 */
void cddb_disc_set_title(cddb_disc_t *disc, const char *title);

/**
 * Append to the disc title.  If the disc does not have a title yet,
 * then a new one will be created from the given string, otherwise
 * that string will be appended to the existing title.
 *
 * @param disc The CDDB disc structure.
 * @param title Part of the disc title.
 */
void cddb_disc_append_title(cddb_disc_t *disc, const char *title);

/**
 * Get the disc artist name.  If the disc is invalid or no artist is
 * set then NULL will be returned.
 *
 * @param disc The CDDB disc structure.
 * @return The disc artist name.
 */
const char *cddb_disc_get_artist(const cddb_disc_t *disc);

/**
 * Set the disc artist name.  If the disc already had an artist name,
 * then the memory for that string will be freed.  The new artist name
 * will be copied into a new chunk of memory.  If the given artist
 * name is NULL, then the artist name of the disc will be deleted.
 *
 * @param disc   The CDDB disc structure.
 * @param artist The new disc artist name.
 */
void cddb_disc_set_artist(cddb_disc_t *disc, const char *artist);

/**
 * Append to the disc artist.  If the disc does not have an artist
 * yet, then a new one will be created from the given string,
 * otherwise that string will be appended to the existing artist.
 *
 * @param disc  The CDDB disc structure.
 * @param artist Part of the artist name.
 */
void cddb_disc_append_artist(cddb_disc_t *disc, const char *artist);

/**
 * Get the extended disc data.  If the disc is invalid or no extended
 * data is set then NULL will be returned.
 *
 * @param disc The CDDB disc structure.
 * @return The extended data.
 */
const char *cddb_disc_get_ext_data(const cddb_disc_t *disc);

/**
 * Set the extended data for the disc.  If the disc already had
 * extended data, then the memory for that string will be freed.  The
 * new extended data will be copied into a new chunk of memory.  If
 * the given extended data is NULL, then the existing data will be
 * deleted.
 *
 * @param disc     The CDDB disc structure.
 * @param ext_data The new extended data.
 */
void cddb_disc_set_ext_data(cddb_disc_t *disc, const char *ext_data);

/**
 * Append to the extended disc data.  If the disc does not have an
 * extended data section yet, then a new one will be created from the
 * given string, otherwise that string will be appended to the
 * existing data.
 *
 * @param disc     The CDDB disc structure.
 * @param ext_data Part of the extended disc data.
 */
void cddb_disc_append_ext_data(cddb_disc_t *disc, const char *ext_data);


/* --- miscellaneous */


/**
 * Copy all data from one disc to another.  Any fields that are
 * unavailable in the source disc structure will not result in a reset
 * of the same field in the destination disc structure; e.g. if there
 * is no title in the source disc, but there is one in the destination
 * disc, then the destination's title will remain unchanged.
 *
 * @param dst The destination CDDB disc structure.
 * @param src The source CDDB disc structure.
 */
void cddb_disc_copy(cddb_disc_t *dst, cddb_disc_t *src);

/**
 * Calculate the CDDB disc ID.  To calculate a disc ID the provided
 * disc needs to have its length set, and every track in the disc
 * structure needs to have its frame offset initialized.  The disc ID
 * field will be set in the disc structure.
 *
 * @param disc The CDDB disc structure.
 * @return A non-zero value if the calculation succeeded, zero
 *         otherwise.
 */
int cddb_disc_calc_discid(cddb_disc_t *disc);

/**
 * Prints information about the disc on stdout.  This is just a
 * debugging routine to display the structure's content.
 *
 * @param disc The CDDB disc structure.
 */
void cddb_disc_print(cddb_disc_t *disc);


#ifdef __cplusplus
    }
#endif

#endif /* CDDB_DISC_H */
