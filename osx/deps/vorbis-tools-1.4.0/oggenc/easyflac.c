/* EasyFLAC - A thin decoding wrapper around libFLAC and libOggFLAC to
 * make your code less ugly.  See easyflac.h for explanation.
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
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <FLAC/export.h>
#if !defined(FLAC_API_VERSION_CURRENT) || (FLAC_API_VERSION_CURRENT < 8)

#include <stdlib.h>
#include "easyflac.h"


FLAC__bool EasyFLAC__is_oggflac(EasyFLAC__StreamDecoder *decoder)
{
    return decoder->is_oggflac;
}


EasyFLAC__StreamDecoder *EasyFLAC__stream_decoder_new(FLAC__bool is_oggflac)
{
    EasyFLAC__StreamDecoder *decoder = malloc(sizeof(EasyFLAC__StreamDecoder));

    if (decoder != NULL)
    {
        decoder->is_oggflac = is_oggflac;

        if (decoder->is_oggflac)
            decoder->oggflac = OggFLAC__stream_decoder_new();
        else
            decoder->flac = FLAC__stream_decoder_new();

        if (  (decoder->is_oggflac && decoder->oggflac == NULL)
            ||(!decoder->is_oggflac && decoder->flac == NULL)  )
        {
            free(decoder);
            decoder = NULL;
        }
    }

    return decoder;
}


void EasyFLAC__stream_decoder_delete(EasyFLAC__StreamDecoder *decoder)
{
    if (decoder->is_oggflac)
        OggFLAC__stream_decoder_delete(decoder->oggflac);
    else
        FLAC__stream_decoder_delete(decoder->flac);

    free(decoder);
}


/* Wrappers around the callbacks for OggFLAC */

FLAC__StreamDecoderReadStatus oggflac_read_callback(const OggFLAC__StreamDecoder *decoder, FLAC__byte buffer[], unsigned *bytes, void *client_data)
{
    EasyFLAC__StreamDecoder *e_decoder = (EasyFLAC__StreamDecoder *) client_data;

    return (*e_decoder->callbacks.read)(e_decoder, buffer, bytes, e_decoder->callbacks.client_data);
}


FLAC__StreamDecoderWriteStatus oggflac_write_callback(const OggFLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data)
{
    EasyFLAC__StreamDecoder *e_decoder = (EasyFLAC__StreamDecoder *) client_data;

    return (*(e_decoder->callbacks.write))(e_decoder, frame, buffer, e_decoder->callbacks.client_data);
}


void oggflac_metadata_callback(const OggFLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data)
{
    EasyFLAC__StreamDecoder *e_decoder = (EasyFLAC__StreamDecoder *) client_data;

    (*e_decoder->callbacks.metadata)(e_decoder, metadata, e_decoder->callbacks.client_data);
}


void oggflac_error_callback(const OggFLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data)
{
    EasyFLAC__StreamDecoder *e_decoder = (EasyFLAC__StreamDecoder *) client_data;

    (*e_decoder->callbacks.error)(e_decoder, status, e_decoder->callbacks.client_data);
}


/* Wrappers around the callbacks for FLAC */

FLAC__StreamDecoderReadStatus flac_read_callback(const FLAC__StreamDecoder *decoder, FLAC__byte buffer[], unsigned *bytes, void *client_data)
{
    EasyFLAC__StreamDecoder *e_decoder = (EasyFLAC__StreamDecoder *) client_data;

    return (*e_decoder->callbacks.read)(e_decoder, buffer, bytes, e_decoder->callbacks.client_data);
}


FLAC__StreamDecoderWriteStatus flac_write_callback(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data)
{
    EasyFLAC__StreamDecoder *e_decoder = (EasyFLAC__StreamDecoder *) client_data;

    return (*e_decoder->callbacks.write)(e_decoder, frame, buffer, e_decoder->callbacks.client_data);
}


void flac_metadata_callback(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data)
{
    EasyFLAC__StreamDecoder *e_decoder = (EasyFLAC__StreamDecoder *) client_data;

    (*e_decoder->callbacks.metadata)(e_decoder, metadata, e_decoder->callbacks.client_data);
}


void flac_error_callback(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data)
{
    EasyFLAC__StreamDecoder *e_decoder = (EasyFLAC__StreamDecoder *) client_data;

    (*e_decoder->callbacks.error)(e_decoder, status, e_decoder->callbacks.client_data);
}


FLAC__bool EasyFLAC__set_read_callback(EasyFLAC__StreamDecoder *decoder, EasyFLAC__StreamDecoderReadCallback value)
{
    decoder->callbacks.read = value;

    if (decoder->is_oggflac)
        return OggFLAC__stream_decoder_set_read_callback(decoder->oggflac, &oggflac_read_callback);
    else
        return FLAC__stream_decoder_set_read_callback(decoder->flac, &flac_read_callback);
}


FLAC__bool EasyFLAC__set_write_callback(EasyFLAC__StreamDecoder *decoder, EasyFLAC__StreamDecoderWriteCallback value)
{
    decoder->callbacks.write = value;

    if (decoder->is_oggflac)
        return OggFLAC__stream_decoder_set_write_callback(decoder->oggflac, &oggflac_write_callback);
    else
        return FLAC__stream_decoder_set_write_callback(decoder->flac, &flac_write_callback);
}


FLAC__bool EasyFLAC__set_metadata_callback(EasyFLAC__StreamDecoder *decoder, EasyFLAC__StreamDecoderMetadataCallback value)
{
    decoder->callbacks.metadata = value;

    if (decoder->is_oggflac)
        return OggFLAC__stream_decoder_set_metadata_callback(decoder->oggflac, &oggflac_metadata_callback);
    else
        return FLAC__stream_decoder_set_metadata_callback(decoder->flac, &flac_metadata_callback);
}


FLAC__bool EasyFLAC__set_error_callback(EasyFLAC__StreamDecoder *decoder, EasyFLAC__StreamDecoderErrorCallback value)
{
    decoder->callbacks.error = value;

    if (decoder->is_oggflac)
        return OggFLAC__stream_decoder_set_error_callback(decoder->oggflac, &oggflac_error_callback);
    else
        return FLAC__stream_decoder_set_error_callback(decoder->flac, &flac_error_callback);
}


FLAC__bool EasyFLAC__set_client_data(EasyFLAC__StreamDecoder *decoder, void *value)
{
    decoder->callbacks.client_data = value;

    if (decoder->is_oggflac)
        return OggFLAC__stream_decoder_set_client_data(decoder->oggflac, decoder);
    else
        return FLAC__stream_decoder_set_client_data(decoder->flac, decoder);
}


FLAC__bool EasyFLAC__set_metadata_respond(EasyFLAC__StreamDecoder *decoder, FLAC__MetadataType type)
{
    if (decoder->is_oggflac)
        return OggFLAC__stream_decoder_set_metadata_respond(decoder->oggflac, type);
    else
        return FLAC__stream_decoder_set_metadata_respond(decoder->flac, type);
}


FLAC__bool EasyFLAC__set_metadata_respond_application(EasyFLAC__StreamDecoder *decoder, const FLAC__byte id[4])
{
    if (decoder->is_oggflac)
        return OggFLAC__stream_decoder_set_metadata_respond_application(decoder->oggflac, id);
    else
        return FLAC__stream_decoder_set_metadata_respond_application(decoder->flac, id);
}


FLAC__bool EasyFLAC__set_metadata_respond_all(EasyFLAC__StreamDecoder *decoder)
{
    if (decoder->is_oggflac)
        return OggFLAC__stream_decoder_set_metadata_respond_all(decoder->oggflac);
    else
        return FLAC__stream_decoder_set_metadata_respond_all(decoder->flac);
}


FLAC__bool EasyFLAC__set_metadata_ignore(EasyFLAC__StreamDecoder *decoder, FLAC__MetadataType type)
{
    if (decoder->is_oggflac)
        return OggFLAC__stream_decoder_set_metadata_ignore(decoder->oggflac, type);
    else
        return FLAC__stream_decoder_set_metadata_ignore(decoder->flac, type);
}


FLAC__bool EasyFLAC__set_metadata_ignore_application(EasyFLAC__StreamDecoder *decoder, const FLAC__byte id[4])
{
    if (decoder->is_oggflac)
        return OggFLAC__stream_decoder_set_metadata_ignore_application(decoder->oggflac, id);
    else
        return FLAC__stream_decoder_set_metadata_ignore_application(decoder->flac, id);
}

FLAC__bool EasyFLAC__set_metadata_ignore_all(EasyFLAC__StreamDecoder *decoder)
{
    if (decoder->is_oggflac)
        return OggFLAC__stream_decoder_set_metadata_ignore_all(decoder->oggflac);
    else
        return FLAC__stream_decoder_set_metadata_ignore_all(decoder->flac);
}


FLAC__StreamDecoderState EasyFLAC__get_state(const EasyFLAC__StreamDecoder *decoder)
{
    if (decoder->is_oggflac)
        return OggFLAC__stream_decoder_get_FLAC_stream_decoder_state(decoder->oggflac);
    else
        return FLAC__stream_decoder_get_state(decoder->flac);
}


unsigned EasyFLAC__get_channels(const EasyFLAC__StreamDecoder *decoder)
{
    if (decoder->is_oggflac)
        return OggFLAC__stream_decoder_get_channels(decoder->oggflac);
    else
        return FLAC__stream_decoder_get_channels(decoder->flac);
}


FLAC__ChannelAssignment EasyFLAC__get_channel_assignment(const EasyFLAC__StreamDecoder *decoder)
{
    if (decoder->is_oggflac)
        return OggFLAC__stream_decoder_get_channel_assignment(decoder->oggflac);
    else
        return FLAC__stream_decoder_get_channel_assignment(decoder->flac);
}


unsigned EasyFLAC__get_bits_per_sample(const EasyFLAC__StreamDecoder *decoder)
{
    if (decoder->is_oggflac)
        return OggFLAC__stream_decoder_get_bits_per_sample(decoder->oggflac);
    else
        return FLAC__stream_decoder_get_bits_per_sample(decoder->flac);
}


unsigned EasyFLAC__get_sample_rate(const EasyFLAC__StreamDecoder *decoder)
{
    if (decoder->is_oggflac)
        return OggFLAC__stream_decoder_get_sample_rate(decoder->oggflac);
    else
        return FLAC__stream_decoder_get_sample_rate(decoder->flac);
}


unsigned EasyFLAC__get_blocksize(const EasyFLAC__StreamDecoder *decoder)
{
    if (decoder->is_oggflac)
        return OggFLAC__stream_decoder_get_blocksize(decoder->oggflac);
    else
        return FLAC__stream_decoder_get_blocksize(decoder->flac);
}


FLAC__StreamDecoderState EasyFLAC__init(EasyFLAC__StreamDecoder *decoder)
{
    if (decoder->is_oggflac)
    {
        OggFLAC__stream_decoder_init(decoder->oggflac);
        return OggFLAC__stream_decoder_get_FLAC_stream_decoder_state(decoder->oggflac);
    }
    else
        return FLAC__stream_decoder_init(decoder->flac);
}


void EasyFLAC__finish(EasyFLAC__StreamDecoder *decoder)
{
    if (decoder->is_oggflac)
        OggFLAC__stream_decoder_finish(decoder->oggflac);
    else
        FLAC__stream_decoder_finish(decoder->flac);
}


FLAC__bool EasyFLAC__flush(EasyFLAC__StreamDecoder *decoder)
{
    if (decoder->is_oggflac)
        return OggFLAC__stream_decoder_flush(decoder->oggflac);
    else
        return FLAC__stream_decoder_flush(decoder->flac);
}


FLAC__bool EasyFLAC__reset(EasyFLAC__StreamDecoder *decoder)
{
    if (decoder->is_oggflac)
        return OggFLAC__stream_decoder_reset(decoder->oggflac);
    else
        return FLAC__stream_decoder_reset(decoder->flac);
}


FLAC__bool EasyFLAC__process_single(EasyFLAC__StreamDecoder *decoder)
{
    if (decoder->is_oggflac)
        return OggFLAC__stream_decoder_process_single(decoder->oggflac);
    else
        return FLAC__stream_decoder_process_single(decoder->flac);
}


FLAC__bool EasyFLAC__process_until_end_of_metadata(EasyFLAC__StreamDecoder *decoder)
{
    if (decoder->is_oggflac)
        return OggFLAC__stream_decoder_process_until_end_of_metadata(decoder->oggflac);
    else
        return FLAC__stream_decoder_process_until_end_of_metadata(decoder->flac);
}


FLAC__bool EasyFLAC__process_until_end_of_stream(EasyFLAC__StreamDecoder *decoder)
{
    if (decoder->is_oggflac)
        return OggFLAC__stream_decoder_process_until_end_of_stream(decoder->oggflac);
    else
        return FLAC__stream_decoder_process_until_end_of_stream(decoder->flac);
}

#endif
