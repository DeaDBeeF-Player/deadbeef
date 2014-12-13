/* EasyFLAC - A thin decoding wrapper around libFLAC and libOggFLAC to
 * make your code less ugly.
 *
 * Copyright 2003 - Stan Seibert <volsung@xiph.org>
 * This code is licensed under a BSD style license:
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * - Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * - Neither the name of the Xiph.org Foundation nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ************************************************************************
 *
 * The motivation for this wrapper is to avoid issues where you need to
 * decode both FLAC and Ogg FLAC but don't want to enclose all of your code
 * in enormous if blocks where the body of the two branches is essentially 
 * the same.  For example, you don't want to do something like this:
 *
 *  if (is_ogg_flac)
 *  {
 *      OggFLAC__blah_blah();
 *      OggFLAC__more_stuff();
 *  }
 *  else
 *  {
 *      FLAC__blah_blah();
 *      FLAC__more_stuff();
 *  }
 * 
 * when you really just want this:
 *
 * EasyFLAC__blah_blah();
 * EasyFLAC__more_stuff();
 *
 * This is even more cumbersome when you have to deal with constants.
 *
 * EasyFLAC uses essentially the same API as
 * FLAC__stream_decoder with two additions:
 * 
 * - EasyFLAC__is_oggflac() for those rare occassions when you might
 * need to distiguish the difference cases.
 *
 * - EasyFLAC__stream_decoder_new() takes a parameter to select when
 * you are reading FLAC or Ogg FLAC.
 *
 * The constants are all FLAC__stream_decoder_*.
 *
 * WARNING: Always call EasyFLAC__set_client_data() even if all you
 * want to do is set the client data to NULL.
 */

#ifndef __EASYFLAC_H
#define __EASYFLAC_H

#include <FLAC/stream_decoder.h>
#include <OggFLAC/stream_decoder.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef struct EasyFLAC__StreamDecoder  EasyFLAC__StreamDecoder;


typedef FLAC__StreamDecoderReadStatus (*EasyFLAC__StreamDecoderReadCallback)(const EasyFLAC__StreamDecoder *decoder, FLAC__byte buffer[], unsigned *bytes, void *client_data);
typedef FLAC__StreamDecoderWriteStatus (*EasyFLAC__StreamDecoderWriteCallback)(const EasyFLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data);
typedef void (*EasyFLAC__StreamDecoderMetadataCallback)(const EasyFLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data);
typedef void (*EasyFLAC__StreamDecoderErrorCallback)(const EasyFLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data);

struct EasyFLAC__StreamDecoder {
	FLAC__bool is_oggflac;
	FLAC__StreamDecoder *flac;
	OggFLAC__StreamDecoder *oggflac;
	struct {
		EasyFLAC__StreamDecoderReadCallback     read;
		EasyFLAC__StreamDecoderWriteCallback    write;
		EasyFLAC__StreamDecoderMetadataCallback metadata;
		EasyFLAC__StreamDecoderErrorCallback    error;
		void *client_data;
	} callbacks;
};



FLAC__bool EasyFLAC__is_oggflac(EasyFLAC__StreamDecoder *decoder);

EasyFLAC__StreamDecoder *EasyFLAC__stream_decoder_new(FLAC__bool is_oggflac);
void EasyFLAC__stream_decoder_delete(EasyFLAC__StreamDecoder *decoder);
FLAC__bool EasyFLAC__set_read_callback(EasyFLAC__StreamDecoder *decoder, EasyFLAC__StreamDecoderReadCallback value);
FLAC__bool EasyFLAC__set_write_callback(EasyFLAC__StreamDecoder *decoder, EasyFLAC__StreamDecoderWriteCallback value);
FLAC__bool EasyFLAC__set_metadata_callback(EasyFLAC__StreamDecoder *decoder, EasyFLAC__StreamDecoderMetadataCallback value);
FLAC__bool EasyFLAC__set_error_callback(EasyFLAC__StreamDecoder *decoder, EasyFLAC__StreamDecoderErrorCallback value);
FLAC__bool EasyFLAC__set_client_data(EasyFLAC__StreamDecoder *decoder, void *value);
FLAC__bool EasyFLAC__set_metadata_respond(EasyFLAC__StreamDecoder *decoder, FLAC__MetadataType type);
FLAC__bool EasyFLAC__set_metadata_respond_application(EasyFLAC__StreamDecoder *decoder, const FLAC__byte id[4]);
FLAC__bool EasyFLAC__set_metadata_respond_all(EasyFLAC__StreamDecoder *decoder);
FLAC__bool EasyFLAC__set_metadata_ignore(EasyFLAC__StreamDecoder *decoder, FLAC__MetadataType type);
FLAC__bool EasyFLAC__set_metadata_ignore_application(EasyFLAC__StreamDecoder *decoder, const FLAC__byte id[4]);
FLAC__bool EasyFLAC__set_metadata_ignore_all(EasyFLAC__StreamDecoder *decoder);
FLAC__StreamDecoderState EasyFLAC__get_state(const EasyFLAC__StreamDecoder *decoder);
unsigned EasyFLAC__get_channels(const EasyFLAC__StreamDecoder *decoder);
FLAC__ChannelAssignment EasyFLAC__get_channel_assignment(const EasyFLAC__StreamDecoder *decoder);
unsigned EasyFLAC__get_bits_per_sample(const EasyFLAC__StreamDecoder *decoder);
unsigned EasyFLAC__get_sample_rate(const EasyFLAC__StreamDecoder *decoder);
unsigned EasyFLAC__get_blocksize(const EasyFLAC__StreamDecoder *decoder);
FLAC__StreamDecoderState EasyFLAC__init(EasyFLAC__StreamDecoder *decoder);
void EasyFLAC__finish(EasyFLAC__StreamDecoder *decoder);
FLAC__bool EasyFLAC__flush(EasyFLAC__StreamDecoder *decoder);
FLAC__bool EasyFLAC__reset(EasyFLAC__StreamDecoder *decoder);
FLAC__bool EasyFLAC__process_single(EasyFLAC__StreamDecoder *decoder);
FLAC__bool EasyFLAC__process_until_end_of_metadata(EasyFLAC__StreamDecoder *decoder);
FLAC__bool EasyFLAC__process_until_end_of_stream(EasyFLAC__StreamDecoder *decoder);

#ifdef __cplusplus
}
#endif

#endif
