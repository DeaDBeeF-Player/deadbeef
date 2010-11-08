/*
    $Id: cddb_track.h,v 1.20 2006/10/15 06:51:11 airborne Exp $

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

#ifndef CDDB_TRACK_H
#define CDDB_TRACK_H 1

#ifdef __cplusplus
    extern "C" {
#endif


/**
 * The CDDB track structure.  Contains all information associated with
 * a single CD track.  This structure will be used to populate the
 * tracks linked list of the cddb_disc_s structure.
 */
typedef struct cddb_track_s cddb_track_t;


/* --- construction / destruction */


/**
 * Creates a new CDDB track structure.
 *
 * @return The CDDB track structure or NULL if memory allocation failed.
 */
cddb_track_t *cddb_track_new(void);

/**
 * Free all resources associated with the given CDDB track structure.
 * The linked list pointer (next) will not be touched.  So you have to
 * make sure that no other tracks are attached to this one before
 * calling this function.
 *
 * @param track The CDDB track structure.
 */
void cddb_track_destroy(cddb_track_t *track);

/**
 * Creates a clone of the given track.
 *
 * @param track The CDDB track structure.
 */
cddb_track_t *cddb_track_clone(const cddb_track_t *track);


/* --- getters & setters --- */


/**
 * Get the number of this track.  This track number starts counting at
 * 1.  If the track is invalid or the track number is not defined -1
 * will be returned.
 *
 * @param track The CDDB track structure.
 * @return The track number.
 */
int cddb_track_get_number(const cddb_track_t *track);

/**
 * Get the frame offset of this track on the disc.  If the track is
 * invalid -1 will be returned.
 *
 * @param track The CDDB track structure.
 * @return The frame offset.
 */
int cddb_track_get_frame_offset(const cddb_track_t *track);

/**
 * Set the frame offset of this track on the disc.
 *
 * @param track The CDDB track structure.
 * @param offset The frame offset.
 * @return The frame offset.
 */
void cddb_track_set_frame_offset(cddb_track_t *track, int offset);

/**
 * Get the length of the track in seconds.  If the track length is not
 * defined this routine will try to calculate it using the frame
 * offsets of the tracks and the total disc length.  These
 * calculations will do no rounding to the nearest second.  So it is
 * possible that the sum off all track lengths does not add up to the
 * actual disc length.  If the length can not be calculated -1 will be
 * returned.
 *
 * @param track The CDDB track structure.
 * @return The track length.
 */
int cddb_track_get_length(cddb_track_t *track);

/**
 * Set the length of the track.  If no frame offset is yet known for
 * this track, and it is part of a disc, then the frame offset will be
 * calculated.
 *
 * @param track  The CDDB track structure.
 * @param length The track length in seconds.
 */
void cddb_track_set_length(cddb_track_t *track, int length);

/**
 * Get the track title.  If the track is invalid or no title is set
 * for this track then NULL will be returned.
 *
 * @param track The CDDB track structure.
 * @return The track title.
 */
const char *cddb_track_get_title(const cddb_track_t *track);

/**
 * Set the track title.  If the track already had a title, then the
 * memory for that string will be freed.  The new title will be copied
 * into a new chunk of memory.  If the given title is NULL, then the
 * title of the track will be deleted.
 *
 * @param track The CDDB track structure.
 * @param title The new track title.
 */
void cddb_track_set_title(cddb_track_t *track, const char *title);

/**
 * Append to the track title.  If the track does not have a title yet,
 * then a new one will be created from the given string, otherwise
 * that string will be appended to the existing title.
 *
 * @param track The CDDB track structure.
 * @param title Part of the track title.
 */
void cddb_track_append_title(cddb_track_t *track, const char *title);

/**
 * Get the track artist name.  If there is no track artist defined,
 * the disc artist will be returned.  NULL will be returned if neither
 * is defined.
 *
 * @param track  The CDDB track structure.
 */
const char *cddb_track_get_artist(cddb_track_t *track);

/**
 * Set the track artist name.  If the track already had an artist
 * name, then the memory for that string will be freed.  The new
 * artist name will be copied into a new chunk of memory.  If the given artist
 * name is NULL, then the artist name of the track will be deleted.
 *
 * @param track  The CDDB track structure.
 * @param artist The new track artist name.
 */
void cddb_track_set_artist(cddb_track_t *track, const char *artist);

/**
 * Append to the track artist.  If the track does not have an artist
 * yet, then a new one will be created from the given string,
 * otherwise that string will be appended to the existing artist.
 *
 * @param track  The CDDB track structure.
 * @param artist Part of the artist name.
 */
void cddb_track_append_artist(cddb_track_t *track, const char *artist);

/**
 * Get the extended track data.  If no extended data is set for this
 * track then NULL will be returned.
 *
 * @param track The CDDB track structure.
 * @return The extended data.
 */
const char *cddb_track_get_ext_data(cddb_track_t *track);

/**
 * Set the extended data for the track.  If the track already had
 * extended data, then the memory for that string will be freed.  The
 * new extended data will be copied into a new chunk of memory.  If
 * the given extended data is NULL, then the existing data will be
 * deleted.
 *
 * @param track    The CDDB track structure.
 * @param ext_data The new extended data.
 */
void cddb_track_set_ext_data(cddb_track_t *track, const char *ext_data);

/**
 * Append to the extended track data.  If the track does not have an
 * extended data section yet, then a new one will be created from the
 * given string, otherwise that string will be appended to the
 * existing data.
 *
 * @param track    The CDDB track structure.
 * @param ext_data Part of the extended track data.
 */
void cddb_track_append_ext_data(cddb_track_t *track, const char *ext_data);


/* --- miscellaneous */


/**
 * Copy all data from one track to another.  Any fields that are
 * unavailable in the source track structure will not result in a
 * reset of the same field in the destination track structure; e.g. if
 * there is no title in the source track, but there is one in the
 * destination track, then the destination's title will remain
 * unchanged.
 *
 * @param dst The destination CDDB track structure.
 * @param src The source CDDB track structure.
 */
void cddb_track_copy(cddb_track_t *dst, cddb_track_t *src);

/**
 * Prints information about the track on stdout.  This is just a
 * debugging routine to display the structure's content.  It is used
 * by cddb_disc_print to print the contents of a complete disc.
 *
 * @param track The CDDB track structure.
 */
void cddb_track_print(cddb_track_t *track);


#ifdef __cplusplus
    }
#endif

#endif /* CDDB_TRACK_H */
