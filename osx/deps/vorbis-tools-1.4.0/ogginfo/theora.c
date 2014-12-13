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
  last mod: $Id: theora.c 17072 2010-03-26 05:07:26Z giles $

 ********************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <string.h>
#include "theora.h"

#define theora_read(x,y,z) ( *z = oggpackB_read(x,y) )

static void _tp_readbuffer(oggpack_buffer *opb, char *buf, const long len)
{
  long i;
  long ret;

  for (i = 0; i < len; i++) {
    theora_read(opb, 8, &ret);
    *buf++=(char)ret;
  }
}

static void _tp_readlsbint(oggpack_buffer *opb, long *value)
{
  int i;
  long ret[4];

  for (i = 0; i < 4; i++) {
    theora_read(opb,8,&ret[i]);
  }
  *value = ret[0]|ret[1]<<8|ret[2]<<16|ret[3]<<24;
}

void theora_info_clear(theora_info *c) {
  memset(c,0,sizeof(*c));
}

static int _theora_unpack_info(theora_info *ci, oggpack_buffer *opb){
  long ret;

  theora_read(opb,8,&ret);
  ci->version_major=(unsigned char)ret;
  theora_read(opb,8,&ret);
  ci->version_minor=(unsigned char)ret;
  theora_read(opb,8,&ret);
  ci->version_subminor=(unsigned char)ret;

  theora_read(opb,16,&ret);
  ci->width=ret<<4;
  theora_read(opb,16,&ret);
  ci->height=ret<<4;
  theora_read(opb,24,&ret);
  ci->frame_width=ret;
  theora_read(opb,24,&ret);
  ci->frame_height=ret;
  theora_read(opb,8,&ret);
  ci->offset_x=ret;
  theora_read(opb,8,&ret);
  ci->offset_y=ret;

  theora_read(opb,32,&ret);
  ci->fps_numerator=ret;
  theora_read(opb,32,&ret);
  ci->fps_denominator=ret;
  theora_read(opb,24,&ret);
  ci->aspect_numerator=ret;
  theora_read(opb,24,&ret);
  ci->aspect_denominator=ret;

  theora_read(opb,8,&ret);
  ci->colorspace=ret;
  theora_read(opb,24,&ret);
  ci->target_bitrate=ret;
  theora_read(opb,6,&ret);
  ci->quality=ret;

  theora_read(opb,5,&ret);
  ci->granule_shift = ret;

  theora_read(opb,2,&ret);
  ci->pixelformat=ret;

  /* spare configuration bits */
  if ( theora_read(opb,3,&ret) == -1 )
    return (OC_BADHEADER);

  return(0);
}

void theora_comment_clear(theora_comment *tc){
  if(tc){
    long i;
    for(i=0;i<tc->comments;i++)
      if(tc->user_comments[i])_ogg_free(tc->user_comments[i]);
    if(tc->user_comments)_ogg_free(tc->user_comments);
    if(tc->comment_lengths)_ogg_free(tc->comment_lengths);
    if(tc->vendor)_ogg_free(tc->vendor);
    memset(tc,0,sizeof(*tc));
  }
}

static int _theora_unpack_comment(theora_comment *tc, oggpack_buffer *opb){
  int i;
  long len;

  _tp_readlsbint(opb,&len);
  if(len<0)return(OC_BADHEADER);
  tc->vendor=_ogg_calloc(1,len+1);
  _tp_readbuffer(opb,tc->vendor, len);
  tc->vendor[len]='\0';

  _tp_readlsbint(opb,(long *) &tc->comments);
  if(tc->comments<0)goto parse_err;
  tc->user_comments=_ogg_calloc(tc->comments,sizeof(*tc->user_comments));
  tc->comment_lengths=_ogg_calloc(tc->comments,sizeof(*tc->comment_lengths));
  for(i=0;i<tc->comments;i++){
    _tp_readlsbint(opb,&len);
    if(len<0)goto parse_err;
    tc->user_comments[i]=_ogg_calloc(1,len+1);
    _tp_readbuffer(opb,tc->user_comments[i],len);
    tc->user_comments[i][len]='\0';
    tc->comment_lengths[i]=len;
  }
  return(0);

parse_err:
  theora_comment_clear(tc);
  return(OC_BADHEADER);
}

static int _theora_unpack_tables(theora_info *c, oggpack_buffer *opb){
  /* NOP: ogginfo doesn't use this information */
  return 0;
}

int theora_decode_header(theora_info *ci, theora_comment *cc, ogg_packet *op){
  long ret;
  oggpack_buffer *opb;

  if(!op)return OC_BADHEADER;

  opb = _ogg_malloc(sizeof(oggpack_buffer));
  oggpackB_readinit(opb,op->packet,op->bytes);
  {
    char id[6];
    int typeflag;

    theora_read(opb,8,&ret);
    typeflag = ret;
    if(!(typeflag&0x80)) {
      free(opb);
      return(OC_NOTFORMAT);
    }

    _tp_readbuffer(opb,id,6);
    if(memcmp(id,"theora",6)) {
      free(opb);
      return(OC_NOTFORMAT);
    }

    switch(typeflag){
    case 0x80:
      if(!op->b_o_s){
        /* Not the initial packet */
        free(opb);
        return(OC_BADHEADER);
      }
      if(ci->version_major!=0){
        /* previously initialized info header */
        free(opb);
        return OC_BADHEADER;
      }

      ret = _theora_unpack_info(ci,opb);
      free(opb);
      return(ret);

    case 0x81:
      if(ci->version_major==0){
        /* um... we didn't get the initial header */
        free(opb);
        return(OC_BADHEADER);
      }

      ret = _theora_unpack_comment(cc,opb);
      free(opb);
      return(ret);

    case 0x82:
      if(ci->version_major==0 || cc->vendor==NULL){
        /* um... we didn't get the initial header or comments yet */
        free(opb);
        return(OC_BADHEADER);
      }

      ret = _theora_unpack_tables(ci,opb);
      free(opb);
      return(ret);

    default:
      free(opb);
      /* ignore any trailing header packets for forward compatibility */
      return(OC_NEWPACKET);
    }
  }
  /* I don't think it's possible to get this far, but better safe.. */
  free(opb);
  return(OC_BADHEADER);
}
