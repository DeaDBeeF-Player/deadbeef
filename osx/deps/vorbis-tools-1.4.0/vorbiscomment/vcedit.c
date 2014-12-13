/* This program is licensed under the GNU Library General Public License, version 2,
 * a copy of which is included with this program (LICENCE.LGPL).
 *
 * (c) 2000-2001 Michael Smith <msmith@xiph.org>
 *
 *
 * Comment editing backend, suitable for use by nice frontend interfaces.
 *
 * last modified: $Id: vcedit.c 16826 2010-01-27 04:16:24Z xiphmont $
 */

/* Handle muxed streams and the Vorbis renormalization without having
 * to understand remuxing:
 * Linked list of buffers (buffer_chain).  Start a link and whenever
 * you encounter an unknown page from the current stream (ie we found
 * its bos in the bos section) push it onto the current buffer.  Whenever
 * you encounter the stream being renormalized create a new link in the
 * chain.
 * On writing, write the contents of the first link before every Vorbis
 * page written, and move to the next link.  Assuming the Vorbis pages
 * in match vorbis pages out, the order of pages from different logical
 * streams will be unchanged.
 * Special case: header. After writing the vorbis headers, and before
 * starting renormalization, flush accumulated links (takes care of
 * situations where number of secondary vorbis header pages changes due
 * to remuxing.  Similarly flush links at the end of renormalization
 * and before the start of the next chain is written.
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ogg/ogg.h>
#include <vorbis/codec.h>

#include "vcedit.h"
#include "vceditaux.h"
#include "i18n.h"


#define CHUNKSIZE 4096
#define BUFFERCHUNK CHUNKSIZE

/* Helper function, shouldn't need to call directly */
static int page_buffer_push(vcedit_buffer_chain *bufferlink, ogg_page *og) {
	int result=0;
	char *tmp;
	vcedit_page_buffer *buffer;
	
	buffer = &bufferlink->buffer;
	tmp = realloc(buffer->data,
		      buffer->data_len + og->header_len + og->body_len);
	if(tmp) {
		buffer->data = tmp;
		memcpy(buffer->data + buffer->data_len, og->header,
		       og->header_len);
		buffer->data_len += og->header_len;
		memcpy(buffer->data + buffer->data_len, og->body,
	       og->body_len);
		result = 1;
		buffer->data_len += og->body_len;
	} else {
		result = -1;
	}
	
	return result;
}

/* Write and free the first link using callbacks */
static int buffer_chain_writelink(vcedit_state *state, void *out) {
	int result = 0;
	vcedit_buffer_chain *tmpchain;
	vcedit_page_buffer *tmpbuffer;

	tmpchain = state->sidebuf;
	tmpbuffer = &tmpchain->buffer;
	if(tmpbuffer->data_len)
	{
		if(state->write(tmpbuffer->data,1,tmpbuffer->data_len, out) !=
		   (size_t) tmpbuffer->data_len)
			result = -1;
		else 
			result = 1;
	}

	free(tmpbuffer->data);
	state->sidebuf = tmpchain->next;
	free(tmpchain);
	return result;
}


static int buffer_chain_newlink(vcedit_state *state) {
	int result = 1;
	vcedit_buffer_chain *bufferlink;
	
	if(!state->sidebuf) {
		state->sidebuf = malloc (sizeof *state->sidebuf);
		if(state->sidebuf) {
			bufferlink = state->sidebuf;
		} else {
			result = -1;
		}
	} else {
		bufferlink=state->sidebuf;
		while(bufferlink->next) {
			bufferlink = bufferlink->next;
		}
		bufferlink->next =  malloc (sizeof *bufferlink->next);
		if(bufferlink->next) {
			bufferlink = bufferlink->next;
		} else {
			result = -1;
		}
	}

	if(result > 0 ) {
		bufferlink->next = 0;
		bufferlink->buffer.data = 0;
		bufferlink->buffer.data_len = 0;
	}
	else 
		state->lasterror =
			_("Couldn't get enough memory for input buffering.");

  return result;
}


/* Push page onto the end of the buffer chain */
static int buffer_chain_push(vcedit_state *state, ogg_page *og) {
	/* If there is no sidebuffer yet we need to create one, otherwise
	 * traverse to the last buffer and push the new page onto it. */
	int result=1;
	vcedit_buffer_chain *bufferlink;
	if(!state->sidebuf) {
		result = buffer_chain_newlink(state);
	}
	
	if(result > 0) {
		bufferlink = state->sidebuf;
		while(bufferlink->next) {
			bufferlink = bufferlink->next;
	}
		result = page_buffer_push(bufferlink, og);
	}

	if(result < 0)
		state->lasterror =
			_("Couldn't get enough memory for input buffering.");

	return result;
}



static int vcedit_supported_stream(vcedit_state *state, ogg_page *og) {
	ogg_stream_state os;
	vorbis_info vi;
	vorbis_comment vc;
	ogg_packet header;
	int result = 0;
	
	ogg_stream_init(&os, ogg_page_serialno(og));
	vorbis_info_init(&vi);
	vorbis_comment_init(&vc);
	
	if( !ogg_page_bos(og) )
                result = -1;

	if(result >= 0 && ogg_stream_pagein(&os, og) < 0)
	{
		state->lasterror =
			_("Error reading first page of Ogg bitstream.");
		result = -1;
	}

	if(result >= 0 && ogg_stream_packetout(&os, &header) != 1)
	{
		state->lasterror = _("Error reading initial header packet.");
		result = -1;
	}

	if(result >= 0 && vorbis_synthesis_headerin(&vi, &vc, &header) >= 0)
	{
                result = 1;
	} else {
		/* Not vorbis, may eventually become a chain of checks (Speex,
		 * Theora), but for the moment return 0, bos scan will push
                 * the current page onto the buffer.
		 */
	}

	ogg_stream_clear(&os);
	vorbis_info_clear(&vi);
	vorbis_comment_clear(&vc);
        return result;
}


static int vcedit_contains_serial (vcedit_state *state, int serialno) {
	int result = 0;
	size_t count;
	for( count=0; count < state->serials.streams_len; count++ ) {
		if ( *(state->serials.streams + count ) == serialno )
			result = 1;
	}
	
	return result;
}


static int vcedit_add_serial (vcedit_state *state, long serial) {
        int result = 0;
        long *tmp;
	
	
	if( vcedit_contains_serial(state, serial) )
        {
		result = 1;
	} else {
		tmp   = realloc(state->serials.streams,
				(state->serials.streams_len + 1) * sizeof *tmp);
		if(tmp) {
			state->serials.streams = tmp;
			*(state->serials.streams +
			  state->serials.streams_len) = serial;
			state->serials.streams_len += 1;
			result = 1;
		} else {
			state->lasterror =
				_("Couldn't get enough memory to register new stream serial number.");
		        result = -1;
		}
	} 
	return result;
}


/* For the benefit of the secondary header read only.  Quietly creates
 * newlinks and pushes pages onto the buffer in the right way */
static int vcedit_target_pageout (vcedit_state *state, ogg_page *og) {
	int result = 0;
	int pageout_result;
	pageout_result = ogg_sync_pageout(state->oy, og);
	if(pageout_result > 0)
	{
		if(state->serial == ogg_page_serialno(og))
			result = buffer_chain_newlink(state);
		else
			result = buffer_chain_push(state, og);
	} else if (pageout_result < 0) {
		/* Vorbis comment traditionally ignores the not-synced
		 * error from pageout, so give it a different code. */
		result = -2;
	}
	return result;
}


/* (I'm paranoid about memset(x,0,len) not giving null pointers */
vcedit_state *vcedit_new_state(void) {
	vcedit_state *state = malloc(sizeof(vcedit_state));
	if(state) {
		memset(state, 0, sizeof(vcedit_state));
		state->sidebuf = 0;
		state->serials.streams = 0;
		state->serials.streams_len = 0;
	}
	return state;
}

char *vcedit_error(vcedit_state *state) {
	return state->lasterror;
}

vorbis_comment *vcedit_comments(vcedit_state *state) {
	return state->vc;
}

static void vcedit_clear_internals(vcedit_state *state) {
    char *tmp;
	if(state->vc) {
		vorbis_comment_clear(state->vc);
		free(state->vc);
	}
	if(state->os) {
		ogg_stream_clear(state->os);
		free(state->os);
	}
	if(state->oy) {
		ogg_sync_clear(state->oy);
		free(state->oy);
	}
	if(state->serials.streams_len) {
		free(state->serials.streams);
		state->serials.streams_len = 0;
		state->serials.streams = 0;
	}
	while(state->sidebuf) {
		vcedit_buffer_chain *tmpbuffer;
		tmpbuffer = state->sidebuf;
		state->sidebuf = tmpbuffer->next;
		free(tmpbuffer->buffer.data);
		free(tmpbuffer);
	}
	if(state->vendor)
		free(state->vendor);
    if(state->mainbuf)
        free(state->mainbuf);
    if(state->bookbuf)
        free(state->bookbuf);
    if(state->vi) {
       	vorbis_info_clear(state->vi);
        free(state->vi);
    }

    tmp = state->lasterror;
    memset(state, 0, sizeof(*state));
    state->lasterror = tmp;
}

void vcedit_clear(vcedit_state *state)
{
	if(state)
	{
		vcedit_clear_internals(state);
		free(state);
	}
}

/* Next two functions pulled straight from libvorbis, apart from one change
 * - we don't want to overwrite the vendor string.
 */
static void _v_writestring(oggpack_buffer *o,char *s, int len)
{
	while(len--)
	{
		oggpack_write(o,*s++,8);
	}
}

static int _commentheader_out(vorbis_comment *vc, char *vendor, ogg_packet *op)
{
	oggpack_buffer opb;

	oggpack_writeinit(&opb);

	/* preamble */  
	oggpack_write(&opb,0x03,8);
	_v_writestring(&opb,"vorbis", 6);

	/* vendor */
	oggpack_write(&opb,strlen(vendor),32);
	_v_writestring(&opb,vendor, strlen(vendor));

	/* comments */
	oggpack_write(&opb,vc->comments,32);
	if(vc->comments){
		int i;
		for(i=0;i<vc->comments;i++){
			if(vc->user_comments[i]){
				oggpack_write(&opb,vc->comment_lengths[i],32);
				_v_writestring(&opb,vc->user_comments[i], 
                        vc->comment_lengths[i]);
			}else{
				oggpack_write(&opb,0,32);
			}
		}
	}
	oggpack_write(&opb,1,1);

	op->packet = malloc(oggpack_bytes(&opb));
	memcpy(op->packet, opb.buffer, oggpack_bytes(&opb));

	op->bytes=oggpack_bytes(&opb);
	op->b_o_s=0;
	op->e_o_s=0;
	op->granulepos=0;

	oggpack_writeclear(&opb);
	return 0;
}

static int _blocksize(vcedit_state *s, ogg_packet *p)
{
	int this = vorbis_packet_blocksize(s->vi, p);
	int ret = (this + s->prevW)/4;

	if(!s->prevW)
	{
		s->prevW = this;
		return 0;
	}

	s->prevW = this;
	return ret;
}

static int _fetch_next_packet(vcedit_state *s, ogg_packet *p, ogg_page *page)
{
	int result;
	char *buffer;
	int bytes;
	int serialno;

	result = ogg_stream_packetout(s->os, p);

	if(result > 0)
		return 1;
	else {
		while(1) {
			if(s->eosin)
				return 0;

			while(ogg_sync_pageout(s->oy, page) <= 0)
			{
				buffer = ogg_sync_buffer(s->oy, CHUNKSIZE);
				bytes = s->read(buffer,1, CHUNKSIZE, s->in);
				ogg_sync_wrote(s->oy, bytes);
				if(bytes == 0)
				return 0;
			}

			serialno = ogg_page_serialno(page);
			if(ogg_page_serialno(page) != s->serial)
			{
				if(vcedit_contains_serial(s, serialno)) {
					result = buffer_chain_push(s, page);
					if(result < 0)
						return result;
				}
				else
				{
					s->eosin = 1;
					s->extrapage = 1;
					return 0;
				}
			} 
			else
			{
			  ogg_stream_pagein(s->os, page);
			  result = buffer_chain_newlink(s);
			  if (result < 0)
				  return result;

			  if(ogg_page_eos(page))
				s->eosin = 1;
			}
			result = ogg_stream_packetout(s->os, p);
			if(result > 0)
				return 1;
		}
		/* Here == trouble */
		return 0;
	}
}

int vcedit_open(vcedit_state *state, FILE *in)
{
	return vcedit_open_callbacks(state, (void *)in, 
			(vcedit_read_func)fread, (vcedit_write_func)fwrite);
}

int vcedit_open_callbacks(vcedit_state *state, void *in,
		vcedit_read_func read_func, vcedit_write_func write_func)
{

	char *buffer;
	int bytes,i;
	int chunks = 0;
	int read_bos, test_supported, page_pending;
	int have_vorbis;
	ogg_packet *header;
	ogg_packet	header_main;
	ogg_packet  header_comments;
	ogg_packet	header_codebooks;
	ogg_page    og;

	state->in = in;
	state->read = read_func;
	state->write = write_func;

	state->oy = malloc(sizeof(ogg_sync_state));
	ogg_sync_init(state->oy);

    while(1)
    {
    	buffer = ogg_sync_buffer(state->oy, CHUNKSIZE);
	    bytes = state->read(buffer, 1, CHUNKSIZE, state->in);

    	ogg_sync_wrote(state->oy, bytes);

        if(ogg_sync_pageout(state->oy, &og) == 1)
            break;

        if(chunks++ >= 10) /* Bail if we don't find data in the first 40 kB */
        {
		    if(bytes<CHUNKSIZE)
			    state->lasterror = _("Input truncated or empty.");
    		else
	    		state->lasterror = _("Input is not an Ogg bitstream.");
	    goto err;
	}
    }

    /* BOS loop, starting with a loaded ogg page. */
    if(buffer_chain_newlink(state) < 0)
	    goto err;

    for( read_bos = 1, have_vorbis = 0 ; read_bos; )
    {
	    test_supported = vcedit_supported_stream(state, &og);
	    if(test_supported < 0)
	    {
		    goto err;
	    }
	    else if (test_supported == 0 || have_vorbis )
	    {
		    if(vcedit_add_serial ( state, ogg_page_serialno(&og)) < 0)
			    goto err;
		    if( buffer_chain_push(state, &og) < 0)
			    goto err;
	    }
	    else if (test_supported > 0)
	    {
		    if(buffer_chain_newlink(state) < 0)
			    goto err;
		    state->serial = ogg_page_serialno(&og);
		    if(vcedit_add_serial ( state, ogg_page_serialno(&og)) < 0)
		       goto err;
 
		    state->os = malloc(sizeof(ogg_stream_state));
		    ogg_stream_init(state->os, state->serial);

		    state->vi = malloc(sizeof(vorbis_info));
		    vorbis_info_init(state->vi);

		    state->vc = malloc(sizeof(vorbis_comment));
		    vorbis_comment_init(state->vc);

		    if(ogg_stream_pagein(state->os, &og) < 0)
		    {
			    state->lasterror = 
				    _("Error reading first page of Ogg bitstream.");
			    goto err;
		    }

		    if(ogg_stream_packetout(state->os, &header_main) != 1)
		    {
			    state->lasterror =
				    _("Error reading initial header packet.");
			    goto err;
		    }

		    if(vorbis_synthesis_headerin(state->vi, state->vc,
						 &header_main) < 0)
		    {
			    state->lasterror =
				    _("Ogg bitstream does not contain Vorbis data.");
			    goto err;
		    }
		    have_vorbis = 1;
	    }
	    while(1)
	    {
		    buffer = ogg_sync_buffer(state->oy, CHUNKSIZE);
		    bytes = state->read(buffer, 1, CHUNKSIZE, state->in);

		    if(bytes == 0)
		    {
			    state->lasterror =
				    _("EOF before recognised stream.");
			    goto err;
		    }

		    ogg_sync_wrote(state->oy, bytes);

		    if(ogg_sync_pageout(state->oy, &og) == 1)
			    break;
	    }
	    if(!ogg_page_bos(&og)) {
		    read_bos = 0;
		    page_pending = 1;
	    }
    }

        if(!state->os) {
		state->lasterror = _("Ogg bitstream does not contain a supported data-type.");
		goto err;
	}

	state->mainlen = header_main.bytes;
	state->mainbuf = malloc(state->mainlen);
	memcpy(state->mainbuf, header_main.packet, header_main.bytes);

	if(ogg_page_serialno(&og) == state->serial)
	{
		if(buffer_chain_newlink(state) < 0)
			goto err;
	}

	else
	{
		if(buffer_chain_push(state, &og) < 0)
			goto err;
		page_pending = 0;
	}

	i = 0;
	header = &header_comments;
	while(i<2) {
		while(i<2) {
			int result;
			if(!page_pending)
				result = vcedit_target_pageout(state, &og);
			else
			{
				result = 1;
				page_pending = 0;
			}
			if(result == 0 || result == -2) break; /* Too little data so far */
			else if(result == -1) goto err;
			else if(result == 1)
			{
				ogg_stream_pagein(state->os, &og);
				while(i<2)
				{
					result = ogg_stream_packetout(state->os, header);
					if(result == 0) break;
					if(result == -1)
					{
						state->lasterror = _("Corrupt secondary header.");
						goto err;
					}
					vorbis_synthesis_headerin(state->vi, state->vc, header);
					if(i==1)
					{
						state->booklen = header->bytes;
						state->bookbuf = malloc(state->booklen);
						memcpy(state->bookbuf, header->packet, 
								header->bytes);
					}
					i++;
					header = &header_codebooks;
				}
			}
		}

		buffer = ogg_sync_buffer(state->oy, CHUNKSIZE);
		bytes = state->read(buffer, 1, CHUNKSIZE, state->in);
		if(bytes == 0 && i < 2)
		{
			state->lasterror = _("EOF before end of Vorbis headers.");
			goto err;
		}
		ogg_sync_wrote(state->oy, bytes);
	}

	/* Copy the vendor tag */
	state->vendor = malloc(strlen(state->vc->vendor) +1);
	strcpy(state->vendor, state->vc->vendor);

	/* Headers are done! */
	return 0;

err:
	vcedit_clear_internals(state);
	return -1;
}

int vcedit_write(vcedit_state *state, void *out)
{
	ogg_stream_state streamout;
	ogg_packet header_main;
	ogg_packet header_comments;
	ogg_packet header_codebooks;

	ogg_page ogout, ogin;
	ogg_packet op;
	ogg_int64_t granpos = 0;
	int result;
	char *buffer;
	int bytes;
	int needflush=0, needout=0;

	state->eosin = 0;
	state->extrapage = 0;

	header_main.bytes = state->mainlen;
	header_main.packet = state->mainbuf;
	header_main.b_o_s = 1;
	header_main.e_o_s = 0;
	header_main.granulepos = 0;

	header_codebooks.bytes = state->booklen;
	header_codebooks.packet = state->bookbuf;
	header_codebooks.b_o_s = 0;
	header_codebooks.e_o_s = 0;
	header_codebooks.granulepos = 0;

	ogg_stream_init(&streamout, state->serial);

	_commentheader_out(state->vc, state->vendor, &header_comments);

	ogg_stream_packetin(&streamout, &header_main);
	ogg_stream_packetin(&streamout, &header_comments);
	ogg_stream_packetin(&streamout, &header_codebooks);

	while((result = ogg_stream_flush(&streamout, &ogout)))
	{
		if(state->sidebuf && buffer_chain_writelink(state, out) < 0)
			goto cleanup;
		if(state->write(ogout.header,1,ogout.header_len, out) !=
				(size_t) ogout.header_len)
			goto cleanup;
		if(state->write(ogout.body,1,ogout.body_len, out) != 
				(size_t) ogout.body_len)
			goto cleanup;
	}

	while(state->sidebuf) {
	  if(buffer_chain_writelink(state, out) < 0)
	    goto cleanup;
	}
	if(buffer_chain_newlink(state) < 0)
		goto cleanup;

	while(_fetch_next_packet(state, &op, &ogin))
	{
		int size;
		size = _blocksize(state, &op);
		granpos += size;

		if(needflush)
		{
			if(ogg_stream_flush(&streamout, &ogout))
			{
				if(state->sidebuf &&
				   buffer_chain_writelink(state, out) < 0)
					goto cleanup;
				if(state->write(ogout.header,1,ogout.header_len, 
							out) != (size_t) ogout.header_len)
					goto cleanup;
				if(state->write(ogout.body,1,ogout.body_len, 
							out) != (size_t) ogout.body_len)
					goto cleanup;
			}
		}
		else if(needout)
		{
			if(ogg_stream_pageout(&streamout, &ogout))
			{
				if(state->sidebuf &&
				   buffer_chain_writelink(state, out) < 0)
					goto cleanup;
				if(state->write(ogout.header,1,ogout.header_len, 
							out) != (size_t) ogout.header_len)
					goto cleanup;
				if(state->write(ogout.body,1,ogout.body_len, 
							out) != (size_t) ogout.body_len)
					goto cleanup;
			}
		}

		needflush=needout=0;

		if(op.granulepos == -1)
		{
			op.granulepos = granpos;
			ogg_stream_packetin(&streamout, &op);
		}
		else /* granulepos is set, validly. Use it, and force a flush to 
				account for shortened blocks (vcut) when appropriate */ 
		{
			if(granpos > op.granulepos)
			{
				granpos = op.granulepos;
				ogg_stream_packetin(&streamout, &op);
				needflush=1;
			}
			else 
			{
				ogg_stream_packetin(&streamout, &op);
				needout=1;
			}
		}		
	}

	streamout.e_o_s = 1;
	while(ogg_stream_flush(&streamout, &ogout))
	{
		if(state->sidebuf && buffer_chain_writelink(state, out) < 0)
			goto cleanup;
		if(state->write(ogout.header,1,ogout.header_len, 
					out) != (size_t) ogout.header_len)
			goto cleanup;
		if(state->write(ogout.body,1,ogout.body_len, 
					out) != (size_t) ogout.body_len)
			goto cleanup;
	}

	if (state->extrapage)
	{
		/* This is the first page of a new chain, get rid of the
		 * sidebuffer */
		while(state->sidebuf)
			if(buffer_chain_writelink(state, out) < 0)
				goto cleanup;
		if(state->write(ogin.header,1,ogin.header_len,
		                out) != (size_t) ogin.header_len)
			goto cleanup;
		if (state->write(ogin.body,1,ogin.body_len, out) !=
				(size_t) ogin.body_len)
			goto cleanup;
	}

	state->eosin=0; /* clear it, because not all paths to here do */
	while(!state->eosin) /* We reached eos, not eof */
	{
		/* We copy the rest of the stream (other logical streams)
		 * through, a page at a time. */
		while(1)
		{
			result = ogg_sync_pageout(state->oy, &ogout);
			if(result==0)
                break;
			if(result<0)
				state->lasterror = _("Corrupt or missing data, continuing...");
			else
			{
				/* Don't bother going through the rest, we can just 
				 * write the page out now */
				if(state->write(ogout.header,1,ogout.header_len, 
						out) != (size_t) ogout.header_len) {
					goto cleanup;
                }
				if(state->write(ogout.body,1,ogout.body_len, out) !=
						(size_t) ogout.body_len) {
					goto cleanup;
                }
			}
		}
		buffer = ogg_sync_buffer(state->oy, CHUNKSIZE);
		bytes = state->read(buffer,1, CHUNKSIZE, state->in);
		ogg_sync_wrote(state->oy, bytes);
		if(bytes == 0) 
		{
			state->eosin = 1;
			break;
		}
	}


cleanup:
	ogg_stream_clear(&streamout);

    /* We don't ogg_packet_clear() this, because the memory was allocated in
       _commentheader_out(), so we mirror that here */
    _ogg_free(header_comments.packet);

	free(state->mainbuf);
	free(state->bookbuf);
    state->mainbuf = state->bookbuf = NULL;

	if(!state->eosin)
	{
		state->lasterror =
			_("Error writing stream to output. "
			"Output stream may be corrupted or truncated.");
		return -1;
	}

	return 0;
}
