#ifndef __VCUT_H
#define __VCUT_H

#include <stdio.h>
#include <ogg/ogg.h>
#include <vorbis/codec.h>

typedef struct {
	int length;
	unsigned char *packet;
} vcut_packet;

/* this structure stores data associated with a single input stream; it will be
   cleared between streams if the input file has multiple chained streams */
typedef struct {
	ogg_stream_state stream_in;
	vorbis_dsp_state vd;
	vorbis_block vb;
	vorbis_info vi;
	vorbis_comment vc;
	int prevW;

	/* granulepos is -1 before any packets are seen, and 0 after the first
	   packet; otherwise it's the GP of the last sample seen */
	ogg_int64_t granulepos;

	/* the granulepos of the first sample (>= 0, since samples with a negative
	   GP are discarded); always 0 for files produced by oggenc or vcut, but
	   may be > 0 for data recorded from a stream (for example) */
	ogg_int64_t initial_granpos;

	/* the number of samples already cut from this stream (all
	   granule positions  */
	ogg_int64_t samples_cut;

	unsigned int serial;
	vcut_packet headers[3];
	vcut_packet last_packet;
} vcut_vorbis_stream;

typedef struct vcut_segment {
	double cuttime;        /* number of seconds at which to cut;
	                          -1 if cutting by sample number */
	ogg_int64_t cutpoint;  /* sample number at which to cut */
	char *filename;        /* name of the file to contain data after the CP */
	struct vcut_segment *next;  /* data for next cut, or NULL */
} vcut_segment;

typedef struct {
	/* pointer to a linked list of segments/cutpoints */
	vcut_segment *next_segment;

	/* the input file may have multiple chained streams; these variables store
	   the number of samples and seconds that passed before the beginning of
	   the current stream */
	ogg_int64_t prevstream_samples;  /* # of samples in prev. streams */
	double prevstream_time;          /* # of seconds past before this stream */

	FILE *in;
	ogg_sync_state sync_in;
	int input_corrupt;            /* 1 if we've complained about corruption */
	int vorbis_init;              /* 1 if vorbis_stream initialized */
	vcut_vorbis_stream vorbis_stream;

	FILE *out;
	char *output_filename;
	int drop_output;              /* 1 if we don't want any output */
	int output_stream_open;       /* 1 if stream_out initialized */
	ogg_stream_state stream_out;
	unsigned int serial_out;      /* serial # for the next output stream */
} vcut_state;

int vcut_process(vcut_state *state);
void vcut_clear(vcut_state *state);
void vcut_vorbis_clear(vcut_vorbis_stream *state);
int write_packet(vcut_state *s, ogg_packet *packet);

#endif /* __VCUT_H */
