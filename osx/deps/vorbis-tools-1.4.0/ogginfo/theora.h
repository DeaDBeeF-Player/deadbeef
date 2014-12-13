/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggTheora SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE Theora SOURCE CODE IS COPYRIGHT (C) 2002-2003                *
 * by the Xiph.Org Foundation http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

  function:
  last mod: $Id: theora.h 16826 2010-01-27 04:16:24Z xiphmont $

 ********************************************************************/

#include <ogg/ogg.h>

/**
 * A Colorspace.
 */
typedef enum {
  OC_CS_UNSPECIFIED,	/**< the colorspace is unknown or unspecified */
  OC_CS_ITU_REC_470M,	/**< best option for 'NTSC' content */
  OC_CS_ITU_REC_470BG,	/**< best option for 'PAL' content */
  OC_CS_NSPACES		/* mark the end of the defined colorspaces */
} theora_colorspace;

/**
 * A Chroma subsampling
 *
 * These enumerate the available chroma subsampling options supported
 * by the theora format. See Section 4.4 of the specification for
 * exact definitions.
 */
typedef enum {
  OC_PF_420,	/**< Chroma subsampling by 2 in each direction (4:2:0) */
  OC_PF_RSVD,	/**< reserved value */
  OC_PF_422,	/**< Horizonatal chroma subsampling by 2 (4:2:2) */
  OC_PF_444,	/**< No chroma subsampling at all (4:4:4) */
} theora_pixelformat;

/**
 * Theora bitstream info.
 * Contains the basic playback parameters for a stream,
 * corresponds to the initial 'info' header packet.
 * 
 * Encoded theora frames must be a multiple of 16 is size;
 * this is what the width and height members represent. To
 * handle other sizes, a crop rectangle is specified in 
 * frame_height and frame_width, offset_x and offset_y. The
 * offset and size should still be a power of 2 to avoid
 * chroma sampling shifts.
 *
 * Frame rate, in frames per second is stored as a rational
 * fraction. So is the aspect ratio. Note that this refers
 * to the aspect ratio of the frame pixels, not of the
 * overall frame itself.
 * 
 * see the example code for use of the other parameters and
 * good default settings for the encoder parameters.
 */
typedef struct {
  ogg_uint32_t  width;
  ogg_uint32_t  height;
  ogg_uint32_t  frame_width;
  ogg_uint32_t  frame_height;
  ogg_uint32_t  offset_x;
  ogg_uint32_t  offset_y;
  ogg_uint32_t  fps_numerator;
  ogg_uint32_t  fps_denominator;
  ogg_uint32_t  aspect_numerator;
  ogg_uint32_t  aspect_denominator;
  theora_colorspace colorspace;
  int           target_bitrate;
  int           quality;  /**< nominal quality setting, 0-63 */
  int           quick_p;  /**< quick encode/decode */

  /* decode only */
  unsigned char version_major;
  unsigned char version_minor;
  unsigned char version_subminor;

  int granule_shift;

  void *codec_setup;

  /* encode only */
  int           dropframes_p;
  int           keyframe_auto_p;
  ogg_uint32_t  keyframe_frequency;
  ogg_uint32_t  keyframe_data_target_bitrate;
  ogg_int32_t   keyframe_auto_threshold;
  ogg_uint32_t  keyframe_mindistance;
  ogg_int32_t   noise_sensitivity;
  ogg_int32_t   sharpness;

  theora_pixelformat pixelformat;

} theora_info;

/** 
 * Comment header metadata.
 *
 * This structure holds the in-stream metadata corresponding to
 * the 'comment' header packet.
 *
 * Meta data is stored as a series of (tag, value) pairs, in
 * length-encoded string vectors. The first occurence of the 
 * '=' character delimits the tag and value. A particular tag
 * may occur more than once. The character set encoding for
 * the strings is always utf-8, but the tag names are limited
 * to case-insensitive ascii. See the spec for details.
 *
 * In filling in this structure, theora_decode_header() will
 * null-terminate the user_comment strings for safety. However,
 * the bitstream format itself treats them as 8-bit clean,
 * and so the length array should be treated as authoritative
 * for their length.
 */
typedef struct theora_comment{
  char **user_comments;		/**< an array of comment string vectors */
  int   *comment_lengths;	/**< an array of corresponding string vector lengths in bytes */
  int    comments;		/**< the total number of comment string vectors */
  char  *vendor;		/**< the vendor string identifying the encoder, null terminated */

} theora_comment;

#define OC_FAULT       -1	/**< general failure */
#define OC_EINVAL      -10	/**< library encountered invalid internal data */
#define OC_DISABLED    -11	/**< requested action is disabled */
#define OC_BADHEADER   -20	/**< header packet was corrupt/invalid */
#define OC_NOTFORMAT   -21	/**< packet is not a theora packet */
#define OC_VERSION     -22	/**< bitstream version is not handled */
#define OC_IMPL        -23	/**< feature or action not implemented */
#define OC_BADPACKET   -24	/**< packet is corrupt */
#define OC_NEWPACKET   -25	/**< packet is an (ignorable) unhandled extension */

/**
 * Decode an Ogg packet, with the expectation that the packet contains
 * an initial header, comment data or codebook tables.
 *
 * \param ci A theora_info structure to fill. This must have been previously
 *           initialized with theora_info_init(). If \a op contains an initial
 *           header, theora_decode_header() will fill \a ci with the
 *           parsed header values. If \a op contains codebook tables,
 *           theora_decode_header() will parse these and attach an internal
 *           representation to \a ci->codec_setup.
 * \param cc A theora_comment structure to fill. If \a op contains comment
 *           data, theora_decode_header() will fill \a cc with the parsed
 *           comments.
 * \param op An ogg_packet structure which you expect contains an initial
 *           header, comment data or codebook tables.
 *
 * \retval OC_BADHEADER \a op is NULL; OR the first byte of \a op->packet
 *                      has the signature of an initial packet, but op is
 *                      not a b_o_s packet; OR this packet has the signature
 *                      of an initial header packet, but an initial header
 *                      packet has already been seen; OR this packet has the
 *                      signature of a comment packet, but the initial header
 *                      has not yet been seen; OR this packet has the signature
 *                      of a comment packet, but contains invalid data; OR
 *                      this packet has the signature of codebook tables,
 *                      but the initial header or comments have not yet
 *                      been seen; OR this packet has the signature of codebook
 *                      tables, but contains invalid data;
 *                      OR the stream being decoded has a compatible version
 *                      but this packet does not have the signature of a
 *                      theora initial header, comments, or codebook packet
 * \retval OC_VERSION   The packet data of \a op is an initial header with
 *                      a version which is incompatible with this version of
 *                      libtheora.
 * \retval OC_NEWPACKET the stream being decoded has an incompatible (future)
 *                      version and contains an unknown signature.
 * \retval 0            Success
 *
 * \note The normal usage is that theora_decode_header() be called on the
 *       first three packets of a theora logical bitstream in succession.
 */
extern int theora_decode_header(theora_info *ci, theora_comment *cc,
                                ogg_packet *op);

void theora_info_clear(theora_info *c);
void theora_comment_clear(theora_comment *tc);
