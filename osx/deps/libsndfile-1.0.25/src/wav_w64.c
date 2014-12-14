/*
** Copyright (C) 1999-2011 Erik de Castro Lopo <erikd@mega-nerd.com>
** Copyright (C) 2004-2005 David Viens <davidv@plogue.com>
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 2.1 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU Lesser General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include	"sfconfig.h"

#include	<stdio.h>
#include	<string.h>
#include	<ctype.h>
#include	<time.h>

#include	"sndfile.h"
#include	"sfendian.h"
#include	"common.h"
#include	"wav_w64.h"

/*  Known WAVEFORMATEXTENSIBLE GUIDS.  */
static const EXT_SUBFORMAT MSGUID_SUBTYPE_PCM =
{	0x00000001, 0x0000, 0x0010, {	0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 }
} ;

static const EXT_SUBFORMAT MSGUID_SUBTYPE_MS_ADPCM =
{	0x00000002, 0x0000, 0x0010, {	0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 }
} ;

static const EXT_SUBFORMAT MSGUID_SUBTYPE_IEEE_FLOAT =
{	0x00000003, 0x0000, 0x0010, {	0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 }
} ;

static const EXT_SUBFORMAT MSGUID_SUBTYPE_ALAW =
{	0x00000006, 0x0000, 0x0010, {	0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 }
} ;

static const EXT_SUBFORMAT MSGUID_SUBTYPE_MULAW =
{	0x00000007, 0x0000, 0x0010, {	0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 }
} ;

/*
** the next two are from
** http://dream.cs.bath.ac.uk/researchdev/wave-ex/bformat.html
*/

static const EXT_SUBFORMAT MSGUID_SUBTYPE_AMBISONIC_B_FORMAT_PCM =
{	0x00000001, 0x0721, 0x11d3, {	0x86, 0x44, 0xc8, 0xc1, 0xca, 0x00, 0x00, 0x00 }
} ;

static const EXT_SUBFORMAT MSGUID_SUBTYPE_AMBISONIC_B_FORMAT_IEEE_FLOAT =
{	0x00000003, 0x0721, 0x11d3, {	0x86, 0x44, 0xc8, 0xc1, 0xca, 0x00, 0x00, 0x00 }
} ;


#if 0
/* maybe interesting one day to read the following through sf_read_raw */
/* http://www.bath.ac.uk/~masrwd/pvocex/pvocex.html */
static const EXT_SUBFORMAT MSGUID_SUBTYPE_PVOCEX =
{	0x8312b9c2, 0x2e6e, 0x11d4, {	0xa8, 0x24, 0xde, 0x5b, 0x96, 0xc3, 0xab, 0x21 }
} ;
#endif

/* This stores which bit in dwChannelMask maps to which channel */
static const struct chanmap_s
{	int id ;
	const char * name ;
} channel_mask_bits [] =
{	/* WAVEFORMATEXTENSIBLE doesn't distuingish FRONT_LEFT from LEFT */
	{	SF_CHANNEL_MAP_LEFT, "L" },
	{	SF_CHANNEL_MAP_RIGHT, "R" },
	{	SF_CHANNEL_MAP_CENTER, "C" },
	{	SF_CHANNEL_MAP_LFE, "LFE" },
	{	SF_CHANNEL_MAP_REAR_LEFT, "Ls" },
	{	SF_CHANNEL_MAP_REAR_RIGHT, "Rs" },
	{	SF_CHANNEL_MAP_FRONT_LEFT_OF_CENTER, "Lc" },
	{	SF_CHANNEL_MAP_FRONT_RIGHT_OF_CENTER, "Rc" },
	{	SF_CHANNEL_MAP_REAR_CENTER, "Cs" },
	{	SF_CHANNEL_MAP_SIDE_LEFT, "Sl" },
	{	SF_CHANNEL_MAP_SIDE_RIGHT, "Sr" },
	{	SF_CHANNEL_MAP_TOP_CENTER, "Tc" },
	{	SF_CHANNEL_MAP_TOP_FRONT_LEFT, "Tfl" },
	{	SF_CHANNEL_MAP_TOP_FRONT_CENTER, "Tfc" },
	{	SF_CHANNEL_MAP_TOP_FRONT_RIGHT, "Tfr" },
	{	SF_CHANNEL_MAP_TOP_REAR_LEFT, "Trl" },
	{	SF_CHANNEL_MAP_TOP_REAR_CENTER, "Trc" },
	{	SF_CHANNEL_MAP_TOP_REAR_RIGHT, "Trr" },
} ;

/*------------------------------------------------------------------------------
 * Private static functions.
 */

static int
wavex_guid_equal (const EXT_SUBFORMAT * first, const EXT_SUBFORMAT * second)
{	return !memcmp (first, second, sizeof (EXT_SUBFORMAT)) ;
} /* wavex_guid_equal */



int
wav_w64_read_fmt_chunk (SF_PRIVATE *psf, int fmtsize)
{	WAV_PRIVATE * wpriv ;
	WAV_FMT *wav_fmt ;
	int	bytesread, k, bytespersec = 0 ;

	if ((wpriv = psf->container_data) == NULL)
		return SFE_INTERNAL ;
	wav_fmt = &wpriv->wav_fmt ;

	memset (wav_fmt, 0, sizeof (WAV_FMT)) ;

	if (fmtsize < 16)
		return SFE_WAV_FMT_SHORT ;

	/* assume psf->rwf_endian is already properly set */

	/* Read the minimal WAV file header here. */
	bytesread = psf_binheader_readf (psf, "224422",
					&(wav_fmt->format), &(wav_fmt->min.channels),
					&(wav_fmt->min.samplerate), &(wav_fmt->min.bytespersec),
					&(wav_fmt->min.blockalign), &(wav_fmt->min.bitwidth)) ;

	psf_log_printf (psf, "  Format        : 0x%X => %s\n", wav_fmt->format, wav_w64_format_str (wav_fmt->format)) ;
	psf_log_printf (psf, "  Channels      : %d\n", wav_fmt->min.channels) ;
	psf_log_printf (psf, "  Sample Rate   : %d\n", wav_fmt->min.samplerate) ;

	if (wav_fmt->format == WAVE_FORMAT_PCM && wav_fmt->min.blockalign == 0
		&& wav_fmt->min.bitwidth > 0 && wav_fmt->min.channels > 0)
	{	wav_fmt->min.blockalign = wav_fmt->min.bitwidth / 8 + (wav_fmt->min.bitwidth % 8 > 0 ? 1 : 0) ;
		wav_fmt->min.blockalign *= wav_fmt->min.channels ;
		psf_log_printf (psf, "  Block Align   : 0 (should be %d)\n", wav_fmt->min.blockalign) ;
		}
	else
		psf_log_printf (psf, "  Block Align   : %d\n", wav_fmt->min.blockalign) ;

	if (wav_fmt->format == WAVE_FORMAT_PCM && wav_fmt->min.bitwidth == 24 &&
			wav_fmt->min.blockalign == 4 * wav_fmt->min.channels)
	{	psf_log_printf (psf, "  Bit Width     : 24\n") ;

		psf_log_printf (psf, "\n"
			"  Ambiguous information in 'fmt ' chunk. Possibile file types:\n"
			"    0) Invalid IEEE float file generated by Syntrillium's Cooledit!\n"
			"    1) File generated by ALSA's arecord containing 24 bit samples in 32 bit containers.\n"
			"    2) 24 bit file with incorrect Block Align value.\n"
			"\n") ;

		wpriv->fmt_is_broken = 1 ;
		}
	else if (wav_fmt->min.bitwidth == 0)
	{	switch (wav_fmt->format)
		{	case WAVE_FORMAT_GSM610 :
			case WAVE_FORMAT_IPP_ITU_G_723_1 :
					psf_log_printf (psf, "  Bit Width     : %d\n", wav_fmt->min.bitwidth) ;
					break ;
			default :
					psf_log_printf (psf, "  Bit Width     : %d (should not be 0)\n", wav_fmt->min.bitwidth) ;
			}
		}
	else
	{	switch (wav_fmt->format)
		{	case WAVE_FORMAT_GSM610 :
			case WAVE_FORMAT_IPP_ITU_G_723_1 :
		psf_log_printf (psf, "  Bit Width     : %d (should be 0)\n", wav_fmt->min.bitwidth) ;
					break ;
			default :
					psf_log_printf (psf, "  Bit Width     : %d\n", wav_fmt->min.bitwidth) ;
			}
		} ;

	psf->sf.samplerate	= wav_fmt->min.samplerate ;
	psf->sf.frames 		= 0 ;					/* Correct this when reading data chunk. */
	psf->sf.channels	= wav_fmt->min.channels ;

	switch (wav_fmt->format)
	{	case WAVE_FORMAT_PCM :
		case WAVE_FORMAT_IEEE_FLOAT :
				bytespersec = wav_fmt->min.samplerate * wav_fmt->min.blockalign ;
				if (wav_fmt->min.bytespersec != (unsigned) bytespersec)
					psf_log_printf (psf, "  Bytes/sec     : %d (should be %d)\n", wav_fmt->min.bytespersec, bytespersec) ;
				else
					psf_log_printf (psf, "  Bytes/sec     : %d\n", wav_fmt->min.bytespersec) ;

				psf->bytewidth = BITWIDTH2BYTES (wav_fmt->min.bitwidth) ;
				break ;

		case WAVE_FORMAT_ALAW :
		case WAVE_FORMAT_MULAW :
				if (wav_fmt->min.bytespersec / wav_fmt->min.blockalign != wav_fmt->min.samplerate)
					psf_log_printf (psf, "  Bytes/sec     : %d (should be %d)\n", wav_fmt->min.bytespersec, wav_fmt->min.samplerate * wav_fmt->min.blockalign) ;
				else
					psf_log_printf (psf, "  Bytes/sec     : %d\n", wav_fmt->min.bytespersec) ;

				psf->bytewidth = 1 ;
				if (fmtsize >= 18)
				{	bytesread += psf_binheader_readf (psf, "2", &(wav_fmt->size20.extrabytes)) ;
					psf_log_printf (psf, "  Extra Bytes   : %d\n", wav_fmt->size20.extrabytes) ;
					} ;
				break ;

		case WAVE_FORMAT_IMA_ADPCM :
				if (wav_fmt->min.bitwidth != 4)
					return SFE_WAV_ADPCM_NOT4BIT ;
				if (wav_fmt->min.channels < 1 || wav_fmt->min.channels > 2)
					return SFE_WAV_ADPCM_CHANNELS ;

				bytesread +=
				psf_binheader_readf (psf, "22", &(wav_fmt->ima.extrabytes), &(wav_fmt->ima.samplesperblock)) ;

				bytespersec = (wav_fmt->ima.samplerate * wav_fmt->ima.blockalign) / wav_fmt->ima.samplesperblock ;
				if (wav_fmt->ima.bytespersec != (unsigned) bytespersec)
					psf_log_printf (psf, "  Bytes/sec     : %d (should be %d)\n", wav_fmt->ima.bytespersec, bytespersec) ;
				else
					psf_log_printf (psf, "  Bytes/sec     : %d\n", wav_fmt->ima.bytespersec) ;

				psf->bytewidth = 2 ;
				psf_log_printf (psf, "  Extra Bytes   : %d\n", wav_fmt->ima.extrabytes) ;
				psf_log_printf (psf, "  Samples/Block : %d\n", wav_fmt->ima.samplesperblock) ;
				break ;

		case WAVE_FORMAT_MS_ADPCM :
				if (wav_fmt->msadpcm.bitwidth != 4)
					return SFE_WAV_ADPCM_NOT4BIT ;
				if (wav_fmt->msadpcm.channels < 1 || wav_fmt->msadpcm.channels > 2)
					return SFE_WAV_ADPCM_CHANNELS ;

				bytesread +=
				psf_binheader_readf (psf, "222", &(wav_fmt->msadpcm.extrabytes),
						&(wav_fmt->msadpcm.samplesperblock), &(wav_fmt->msadpcm.numcoeffs)) ;

				bytespersec = (wav_fmt->min.samplerate * wav_fmt->min.blockalign) / wav_fmt->msadpcm.samplesperblock ;
				if (wav_fmt->min.bytespersec == (unsigned) bytespersec)
					psf_log_printf (psf, "  Bytes/sec     : %d\n", wav_fmt->min.bytespersec) ;
				else if (wav_fmt->min.bytespersec == (wav_fmt->min.samplerate / wav_fmt->msadpcm.samplesperblock) * wav_fmt->min.blockalign)
					psf_log_printf (psf, "  Bytes/sec     : %d (should be %d (MS BUG!))\n", wav_fmt->min.bytespersec, bytespersec) ;
				else
					psf_log_printf (psf, "  Bytes/sec     : %d (should be %d)\n", wav_fmt->min.bytespersec, bytespersec) ;

				psf->bytewidth = 2 ;
				psf_log_printf (psf, "  Extra Bytes   : %d\n", wav_fmt->msadpcm.extrabytes) ;
				psf_log_printf (psf, "  Samples/Block : %d\n", wav_fmt->msadpcm.samplesperblock) ;
				if (wav_fmt->msadpcm.numcoeffs > ARRAY_LEN (wav_fmt->msadpcm.coeffs))
				{	psf_log_printf (psf, "  No. of Coeffs : %d (should be <= %d)\n", wav_fmt->msadpcm.numcoeffs, ARRAY_LEN (wav_fmt->msadpcm.coeffs)) ;
					wav_fmt->msadpcm.numcoeffs = ARRAY_LEN (wav_fmt->msadpcm.coeffs) ;
					}
				else
					psf_log_printf (psf, "  No. of Coeffs : %d\n", wav_fmt->msadpcm.numcoeffs) ;

				psf_log_printf (psf, "    Index   Coeffs1   Coeffs2\n") ;
				for (k = 0 ; k < wav_fmt->msadpcm.numcoeffs ; k++)
				{	bytesread +=
					psf_binheader_readf (psf, "22", &(wav_fmt->msadpcm.coeffs [k].coeff1), &(wav_fmt->msadpcm.coeffs [k].coeff2)) ;
					snprintf (psf->u.cbuf, sizeof (psf->u.cbuf), "     %2d     %7d   %7d\n", k, wav_fmt->msadpcm.coeffs [k].coeff1, wav_fmt->msadpcm.coeffs [k].coeff2) ;
					psf_log_printf (psf, psf->u.cbuf) ;
					} ;
				break ;

		case WAVE_FORMAT_GSM610 :
				if (wav_fmt->gsm610.channels != 1 || wav_fmt->gsm610.blockalign != 65)
					return SFE_WAV_GSM610_FORMAT ;

				bytesread +=
				psf_binheader_readf (psf, "22", &(wav_fmt->gsm610.extrabytes), &(wav_fmt->gsm610.samplesperblock)) ;

				if (wav_fmt->gsm610.samplesperblock != 320)
					return SFE_WAV_GSM610_FORMAT ;

				bytespersec = (wav_fmt->gsm610.samplerate * wav_fmt->gsm610.blockalign) / wav_fmt->gsm610.samplesperblock ;
				if (wav_fmt->gsm610.bytespersec != (unsigned) bytespersec)
					psf_log_printf (psf, "  Bytes/sec     : %d (should be %d)\n", wav_fmt->gsm610.bytespersec, bytespersec) ;
				else
					psf_log_printf (psf, "  Bytes/sec     : %d\n", wav_fmt->gsm610.bytespersec) ;

				psf->bytewidth = 2 ;
				psf_log_printf (psf, "  Extra Bytes   : %d\n", wav_fmt->gsm610.extrabytes) ;
				psf_log_printf (psf, "  Samples/Block : %d\n", wav_fmt->gsm610.samplesperblock) ;
				break ;

		case WAVE_FORMAT_EXTENSIBLE :
				if (wav_fmt->ext.bytespersec / wav_fmt->ext.blockalign != wav_fmt->ext.samplerate)
					psf_log_printf (psf, "  Bytes/sec     : %d (should be %d)\n", wav_fmt->ext.bytespersec, wav_fmt->ext.samplerate * wav_fmt->ext.blockalign) ;
				else
					psf_log_printf (psf, "  Bytes/sec     : %d\n", wav_fmt->ext.bytespersec) ;

				bytesread +=
				psf_binheader_readf (psf, "224", &(wav_fmt->ext.extrabytes), &(wav_fmt->ext.validbits),
						&(wav_fmt->ext.channelmask)) ;

				psf_log_printf (psf, "  Valid Bits    : %d\n", wav_fmt->ext.validbits) ;

				if (wav_fmt->ext.channelmask == 0)
					psf_log_printf (psf, "  Channel Mask  : 0x0 (should not be zero)\n") ;
				else
				{	unsigned bit ;

					wpriv->wavex_channelmask = wav_fmt->ext.channelmask ;

					/* It's probably wise to ignore the channel mask if it is all zero */
					free (psf->channel_map) ;

					if ((psf->channel_map = calloc (psf->sf.channels, sizeof (psf->channel_map [0]))) == NULL)
						return SFE_MALLOC_FAILED ;

					/* Terminate the buffer we're going to append_snprintf into. */
					psf->u.cbuf [0] = 0 ;

					for (bit = k = 0 ; bit < ARRAY_LEN (channel_mask_bits) ; bit++)
					{
						if (wav_fmt->ext.channelmask & (1 << bit))
						{	if (k > psf->sf.channels)
							{	psf_log_printf (psf, "*** More channel map bits than there are channels.\n") ;
								break ;
								} ;

							psf->channel_map [k++] = channel_mask_bits [bit].id ;
							append_snprintf (psf->u.cbuf, sizeof (psf->u.cbuf), "%s, ", channel_mask_bits [bit].name) ;
							} ;
						} ;

					/* Remove trailing ", ". */
					bit = strlen (psf->u.cbuf) ;
					psf->u.cbuf [--bit] = 0 ;
					psf->u.cbuf [--bit] = 0 ;

					if (k != psf->sf.channels)
					{	psf_log_printf (psf, "  Channel Mask  : 0x%X\n", wav_fmt->ext.channelmask) ;
						psf_log_printf (psf, "*** Less channel map bits than there are channels.\n") ;
						}
					else
						psf_log_printf (psf, "  Channel Mask  : 0x%X (%s)\n", wav_fmt->ext.channelmask, psf->u.cbuf) ;
					} ;

				bytesread += psf_binheader_readf (psf, "422", &(wav_fmt->ext.esf.esf_field1), &(wav_fmt->ext.esf.esf_field2), &(wav_fmt->ext.esf.esf_field3)) ;

				/* compare the esf_fields with each known GUID? and print? */
				psf_log_printf (psf, "  Subformat\n") ;
				psf_log_printf (psf, "    esf_field1 : 0x%X\n", wav_fmt->ext.esf.esf_field1) ;
				psf_log_printf (psf, "    esf_field2 : 0x%X\n", wav_fmt->ext.esf.esf_field2) ;
				psf_log_printf (psf, "    esf_field3 : 0x%X\n", wav_fmt->ext.esf.esf_field3) ;
				psf_log_printf (psf, "    esf_field4 : ") ;
				for (k = 0 ; k < 8 ; k++)
				{	bytesread += psf_binheader_readf (psf, "1", &(wav_fmt->ext.esf.esf_field4 [k])) ;
					psf_log_printf (psf, "0x%X ", wav_fmt->ext.esf.esf_field4 [k] & 0xFF) ;
					} ;
				psf_log_printf (psf, "\n") ;
				psf->bytewidth = BITWIDTH2BYTES (wav_fmt->ext.bitwidth) ;

				/* Compare GUIDs for known ones. */
				if (wavex_guid_equal (&wav_fmt->ext.esf, &MSGUID_SUBTYPE_PCM))
				{	psf->sf.format = SF_FORMAT_WAVEX | u_bitwidth_to_subformat (psf->bytewidth * 8) ;
					psf_log_printf (psf, "    format : pcm\n") ;
					}
				else if (wavex_guid_equal (&wav_fmt->ext.esf, &MSGUID_SUBTYPE_MS_ADPCM))
				{	psf->sf.format = (SF_FORMAT_WAVEX | SF_FORMAT_MS_ADPCM) ;
					psf_log_printf (psf, "    format : ms adpcm\n") ;
					}
				else if (wavex_guid_equal (&wav_fmt->ext.esf, &MSGUID_SUBTYPE_IEEE_FLOAT))
				{	psf->sf.format = SF_FORMAT_WAVEX | ((psf->bytewidth == 8) ? SF_FORMAT_DOUBLE : SF_FORMAT_FLOAT) ;
					psf_log_printf (psf, "    format : IEEE float\n") ;
					}
				else if (wavex_guid_equal (&wav_fmt->ext.esf, &MSGUID_SUBTYPE_ALAW))
				{	psf->sf.format = (SF_FORMAT_WAVEX | SF_FORMAT_ALAW) ;
					psf_log_printf (psf, "    format : A-law\n") ;
					}
				else if (wavex_guid_equal (&wav_fmt->ext.esf, &MSGUID_SUBTYPE_MULAW))
				{	psf->sf.format = (SF_FORMAT_WAVEX | SF_FORMAT_ULAW) ;
					psf_log_printf (psf, "    format : u-law\n") ;
					}
				else if (wavex_guid_equal (&wav_fmt->ext.esf, &MSGUID_SUBTYPE_AMBISONIC_B_FORMAT_PCM))
				{	psf->sf.format = SF_FORMAT_WAVEX | u_bitwidth_to_subformat (psf->bytewidth * 8) ;
					psf_log_printf (psf, "    format : pcm (Ambisonic B)\n") ;
					wpriv->wavex_ambisonic = SF_AMBISONIC_B_FORMAT ;
					}
				else if (wavex_guid_equal (&wav_fmt->ext.esf, &MSGUID_SUBTYPE_AMBISONIC_B_FORMAT_IEEE_FLOAT))
				{	psf->sf.format = SF_FORMAT_WAVEX | ((psf->bytewidth == 8) ? SF_FORMAT_DOUBLE : SF_FORMAT_FLOAT) ;
					psf_log_printf (psf, "    format : IEEE float (Ambisonic B)\n") ;
					wpriv->wavex_ambisonic = SF_AMBISONIC_B_FORMAT ;
					}
				else
					return SFE_UNIMPLEMENTED ;

				break ;

		case WAVE_FORMAT_G721_ADPCM :
				psf_log_printf (psf, "  Bytes/sec     : %d\n", wav_fmt->g72x.bytespersec) ;
				if (fmtsize >= 20)
				{	bytesread += psf_binheader_readf (psf, "22", &(wav_fmt->g72x.extrabytes), &(wav_fmt->g72x.auxblocksize)) ;
					if (wav_fmt->g72x.extrabytes == 0)
						psf_log_printf (psf, "  Extra Bytes   : %d (should be 2)\n", wav_fmt->g72x.extrabytes) ;
					else
						psf_log_printf (psf, "  Extra Bytes   : %d\n", wav_fmt->g72x.extrabytes) ;
						psf_log_printf (psf, "  Aux Blk Size  : %d\n", wav_fmt->g72x.auxblocksize) ;
					}
				else if (fmtsize == 18)
				{	bytesread += psf_binheader_readf (psf, "2", &(wav_fmt->g72x.extrabytes)) ;
					psf_log_printf (psf, "  Extra Bytes   : %d%s\n", wav_fmt->g72x.extrabytes, wav_fmt->g72x.extrabytes != 0 ? " (should be 0)" : "") ;
					}
				else
					psf_log_printf (psf, "*** 'fmt ' chunk should be bigger than this!\n") ;
				break ;

		default :
				psf_log_printf (psf, "*** No 'fmt ' chunk dumper for this format!\n") ;
				return SFE_WAV_BAD_FMT ;
		} ;

	if (bytesread > fmtsize)
	{	psf_log_printf (psf, "*** wav_w64_read_fmt_chunk (bytesread > fmtsize)\n") ;
		return SFE_WAV_BAD_FMT ;
		}
	else
		psf_binheader_readf (psf, "j", fmtsize - bytesread) ;

	psf->blockwidth = wav_fmt->min.channels * psf->bytewidth ;

	return 0 ;
} /* wav_w64_read_fmt_chunk */

void
wavex_write_guid (SF_PRIVATE *psf, const EXT_SUBFORMAT * subformat)
{
	psf_binheader_writef (psf, "422b", subformat->esf_field1,
					subformat->esf_field2, subformat->esf_field3,
					subformat->esf_field4, make_size_t (8)) ;
} /* wavex_write_guid */


int
wavex_gen_channel_mask (const int *chan_map, int channels)
{	int chan, mask = 0, bit = -1, last_bit = -1 ;

	if (chan_map == NULL)
		return 0 ;

	for (chan = 0 ; chan < channels ; chan ++)
	{	int k ;

		for (k = bit + 1 ; k < ARRAY_LEN (channel_mask_bits) ; k++)
			if (chan_map [chan] == channel_mask_bits [k].id)
			{	bit = k ;
				break ;
				} ;

		/* Check for bad sequence. */
		if (bit <= last_bit)
			return 0 ;

		mask += 1 << bit ;
		last_bit = bit ;
		} ;

	return mask ;
} /* wavex_gen_channel_mask */

void
wav_w64_analyze (SF_PRIVATE *psf)
{	AUDIO_DETECT ad ;
	int format = 0 ;

	if (psf->is_pipe)
	{	psf_log_printf (psf, "*** Error : Reading from a pipe. Can't analyze data section to figure out real data format.\n\n") ;
		return ;
		} ;

	psf_log_printf (psf, "---------------------------------------------------\n"
						"Format is known to be broken. Using detection code.\n") ;

	/* Code goes here. */
	ad.endianness = SF_ENDIAN_LITTLE ;
	ad.channels = psf->sf.channels ;

	psf_fseek (psf, 3 * 4 * 50, SEEK_SET) ;

	while (psf_fread (psf->u.ucbuf, 1, 4096, psf) == 4096)
	{	format = audio_detect (psf, &ad, psf->u.ucbuf, 4096) ;
		if (format != 0)
			break ;
		} ;

	/* Seek to start of DATA section. */
	psf_fseek (psf, psf->dataoffset, SEEK_SET) ;

	if (format == 0)
	{	psf_log_printf (psf, "wav_w64_analyze : detection failed.\n") ;
		return ;
		} ;

	switch (format)
	{	case SF_FORMAT_PCM_32 :
		case SF_FORMAT_FLOAT :
			psf_log_printf (psf, "wav_w64_analyze : found format : 0x%X\n", format) ;
			psf->sf.format = (psf->sf.format & ~SF_FORMAT_SUBMASK) + format ;
			psf->bytewidth = 4 ;
			psf->blockwidth = psf->sf.channels * psf->bytewidth ;
			break ;

		case SF_FORMAT_PCM_24 :
			psf_log_printf (psf, "wav_w64_analyze : found format : 0x%X\n", format) ;
			psf->sf.format = (psf->sf.format & ~SF_FORMAT_SUBMASK) + format ;
			psf->bytewidth = 3 ;
			psf->blockwidth = psf->sf.channels * psf->bytewidth ;
			break ;

		default :
			psf_log_printf (psf, "wav_w64_analyze : unhandled format : 0x%X\n", format) ;
			break ;
		} ;

	return ;
} /* wav_w64_analyze */

/*==============================================================================
*/

typedef struct
{	int			ID ;
	const char	*name ;
} WAV_FORMAT_DESC ;

#define STR(x)			#x
#define FORMAT_TYPE(x)	{ x, STR (x) }

static WAV_FORMAT_DESC wave_descs [] =
{	FORMAT_TYPE	(WAVE_FORMAT_PCM),
	FORMAT_TYPE (WAVE_FORMAT_MS_ADPCM),
	FORMAT_TYPE (WAVE_FORMAT_IEEE_FLOAT),
	FORMAT_TYPE (WAVE_FORMAT_VSELP),
	FORMAT_TYPE (WAVE_FORMAT_IBM_CVSD),
	FORMAT_TYPE (WAVE_FORMAT_ALAW),
	FORMAT_TYPE (WAVE_FORMAT_MULAW),
	FORMAT_TYPE (WAVE_FORMAT_OKI_ADPCM),
	FORMAT_TYPE (WAVE_FORMAT_IMA_ADPCM),
	FORMAT_TYPE (WAVE_FORMAT_MEDIASPACE_ADPCM),
	FORMAT_TYPE (WAVE_FORMAT_SIERRA_ADPCM),
	FORMAT_TYPE (WAVE_FORMAT_G723_ADPCM),
	FORMAT_TYPE (WAVE_FORMAT_DIGISTD),
	FORMAT_TYPE (WAVE_FORMAT_DIGIFIX),
	FORMAT_TYPE (WAVE_FORMAT_DIALOGIC_OKI_ADPCM),
	FORMAT_TYPE (WAVE_FORMAT_MEDIAVISION_ADPCM),
	FORMAT_TYPE (WAVE_FORMAT_CU_CODEC),
	FORMAT_TYPE (WAVE_FORMAT_YAMAHA_ADPCM),
	FORMAT_TYPE (WAVE_FORMAT_SONARC),
	FORMAT_TYPE (WAVE_FORMAT_DSPGROUP_TRUESPEECH),
	FORMAT_TYPE (WAVE_FORMAT_ECHOSC1),
	FORMAT_TYPE (WAVE_FORMAT_AUDIOFILE_AF36),
	FORMAT_TYPE (WAVE_FORMAT_APTX),
	FORMAT_TYPE (WAVE_FORMAT_AUDIOFILE_AF10),
	FORMAT_TYPE (WAVE_FORMAT_PROSODY_1612),
	FORMAT_TYPE (WAVE_FORMAT_LRC),
	FORMAT_TYPE (WAVE_FORMAT_DOLBY_AC2),
	FORMAT_TYPE (WAVE_FORMAT_GSM610),
	FORMAT_TYPE (WAVE_FORMAT_MSNAUDIO),
	FORMAT_TYPE (WAVE_FORMAT_ANTEX_ADPCME),
	FORMAT_TYPE (WAVE_FORMAT_CONTROL_RES_VQLPC),
	FORMAT_TYPE (WAVE_FORMAT_DIGIREAL),
	FORMAT_TYPE (WAVE_FORMAT_DIGIADPCM),
	FORMAT_TYPE (WAVE_FORMAT_CONTROL_RES_CR10),
	FORMAT_TYPE (WAVE_FORMAT_NMS_VBXADPCM),
	FORMAT_TYPE (WAVE_FORMAT_ROLAND_RDAC),
	FORMAT_TYPE (WAVE_FORMAT_ECHOSC3),
	FORMAT_TYPE (WAVE_FORMAT_ROCKWELL_ADPCM),
	FORMAT_TYPE (WAVE_FORMAT_ROCKWELL_DIGITALK),
	FORMAT_TYPE (WAVE_FORMAT_XEBEC),
	FORMAT_TYPE (WAVE_FORMAT_G721_ADPCM),
	FORMAT_TYPE (WAVE_FORMAT_G728_CELP),
	FORMAT_TYPE (WAVE_FORMAT_MSG723),
	FORMAT_TYPE (WAVE_FORMAT_MPEG),
	FORMAT_TYPE (WAVE_FORMAT_RT24),
	FORMAT_TYPE (WAVE_FORMAT_PAC),
	FORMAT_TYPE (WAVE_FORMAT_MPEGLAYER3),
	FORMAT_TYPE (WAVE_FORMAT_LUCENT_G723),
	FORMAT_TYPE (WAVE_FORMAT_CIRRUS),
	FORMAT_TYPE (WAVE_FORMAT_ESPCM),
	FORMAT_TYPE (WAVE_FORMAT_VOXWARE),
	FORMAT_TYPE (WAVE_FORMAT_CANOPUS_ATRAC),
	FORMAT_TYPE (WAVE_FORMAT_G726_ADPCM),
	FORMAT_TYPE (WAVE_FORMAT_G722_ADPCM),
	FORMAT_TYPE (WAVE_FORMAT_DSAT),
	FORMAT_TYPE (WAVE_FORMAT_DSAT_DISPLAY),
	FORMAT_TYPE (WAVE_FORMAT_VOXWARE_BYTE_ALIGNED),
	FORMAT_TYPE (WAVE_FORMAT_VOXWARE_AC8),
	FORMAT_TYPE (WAVE_FORMAT_VOXWARE_AC10),
	FORMAT_TYPE (WAVE_FORMAT_VOXWARE_AC16),
	FORMAT_TYPE (WAVE_FORMAT_VOXWARE_AC20),
	FORMAT_TYPE (WAVE_FORMAT_VOXWARE_RT24),
	FORMAT_TYPE (WAVE_FORMAT_VOXWARE_RT29),
	FORMAT_TYPE (WAVE_FORMAT_VOXWARE_RT29HW),
	FORMAT_TYPE (WAVE_FORMAT_VOXWARE_VR12),
	FORMAT_TYPE (WAVE_FORMAT_VOXWARE_VR18),
	FORMAT_TYPE (WAVE_FORMAT_VOXWARE_TQ40),
	FORMAT_TYPE (WAVE_FORMAT_SOFTSOUND),
	FORMAT_TYPE (WAVE_FORMAT_VOXARE_TQ60),
	FORMAT_TYPE (WAVE_FORMAT_MSRT24),
	FORMAT_TYPE (WAVE_FORMAT_G729A),
	FORMAT_TYPE (WAVE_FORMAT_MVI_MV12),
	FORMAT_TYPE (WAVE_FORMAT_DF_G726),
	FORMAT_TYPE (WAVE_FORMAT_DF_GSM610),
	FORMAT_TYPE (WAVE_FORMAT_ONLIVE),
	FORMAT_TYPE (WAVE_FORMAT_SBC24),
	FORMAT_TYPE (WAVE_FORMAT_DOLBY_AC3_SPDIF),
	FORMAT_TYPE (WAVE_FORMAT_ZYXEL_ADPCM),
	FORMAT_TYPE (WAVE_FORMAT_PHILIPS_LPCBB),
	FORMAT_TYPE (WAVE_FORMAT_PACKED),
	FORMAT_TYPE (WAVE_FORMAT_RHETOREX_ADPCM),
	FORMAT_TYPE (IBM_FORMAT_MULAW),
	FORMAT_TYPE (IBM_FORMAT_ALAW),
	FORMAT_TYPE (IBM_FORMAT_ADPCM),
	FORMAT_TYPE (WAVE_FORMAT_VIVO_G723),
	FORMAT_TYPE (WAVE_FORMAT_VIVO_SIREN),
	FORMAT_TYPE (WAVE_FORMAT_DIGITAL_G723),
	FORMAT_TYPE (WAVE_FORMAT_CREATIVE_ADPCM),
	FORMAT_TYPE (WAVE_FORMAT_CREATIVE_FASTSPEECH8),
	FORMAT_TYPE (WAVE_FORMAT_CREATIVE_FASTSPEECH10),
	FORMAT_TYPE (WAVE_FORMAT_QUARTERDECK),
	FORMAT_TYPE (WAVE_FORMAT_FM_TOWNS_SND),
	FORMAT_TYPE (WAVE_FORMAT_BZV_DIGITAL),
	FORMAT_TYPE (WAVE_FORMAT_VME_VMPCM),
	FORMAT_TYPE (WAVE_FORMAT_OLIGSM),
	FORMAT_TYPE (WAVE_FORMAT_OLIADPCM),
	FORMAT_TYPE (WAVE_FORMAT_OLICELP),
	FORMAT_TYPE (WAVE_FORMAT_OLISBC),
	FORMAT_TYPE (WAVE_FORMAT_OLIOPR),
	FORMAT_TYPE (WAVE_FORMAT_LH_CODEC),
	FORMAT_TYPE (WAVE_FORMAT_NORRIS),
	FORMAT_TYPE (WAVE_FORMAT_SOUNDSPACE_MUSICOMPRESS),
	FORMAT_TYPE (WAVE_FORMAT_DVM),
	FORMAT_TYPE (WAVE_FORMAT_INTERWAV_VSC112),
	FORMAT_TYPE (WAVE_FORMAT_IPP_ITU_G_723_1),
	FORMAT_TYPE (WAVE_FORMAT_EXTENSIBLE),
} ;

char const*
wav_w64_format_str (int k)
{	int lower, upper, mid ;

	lower = -1 ;
	upper = sizeof (wave_descs) / sizeof (WAV_FORMAT_DESC) ;

	/* binary search */
	if ((wave_descs [0].ID <= k) && (k <= wave_descs [upper - 1].ID))
	{
		while (lower + 1 < upper)
		{	mid = (upper + lower) / 2 ;

			if (k == wave_descs [mid].ID)
				return wave_descs [mid].name ;
			if (k < wave_descs [mid].ID)
				upper = mid ;
			else
				lower = mid ;
			} ;
		} ;

	return "Unknown format" ;
} /* wav_w64_format_str */

int
wav_w64_srate2blocksize (int srate_chan_product)
{	if (srate_chan_product < 12000)
		return 256 ;
	if (srate_chan_product < 23000)
		return 512 ;
	if (srate_chan_product < 44000)
		return 1024 ;
	return 2048 ;
} /* srate2blocksize */
