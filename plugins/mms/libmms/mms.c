/*
 * Copyright (C) 2002-2004 the xine project
 * Copyright (C) 2004-2012 the libmms project
 * 
 * This file is part of LibMMS, an MMS protocol handling library.
 * This file was originally a part of xine, a free video player.
 *
 * Libmms is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * Libmms is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA
 *
 * $Id: mms.c,v 1.31 2007/12/11 20:35:01 jwrdegoede Exp $
 *
 * MMS over TCP protocol
 *   based on work from major mms
 *   utility functions to handle communication with an mms server
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>

//#define USE_ICONV

#ifdef USE_ICONV
#include <iconv.h>
#else
// use deadbeef's charset conversion instead of iconv
#include <deadbeef/deadbeef.h>
extern DB_functions_t *deadbeef;
#endif

/********** logging **********/
#define lprintf(...) if (getenv("LIBMMS_DEBUG")) fprintf(stderr, __VA_ARGS__)

#define __MMS_C__

#include "bswap.h"
#include "mms.h"
#include "asfheader.h"
#include "uri.h"
#include "mms-common.h"

/* 
 * mms specific types 
 */

#define MMST_PORT 1755

#define BUF_SIZE 102400

#define CMD_HEADER_LEN   40
#define CMD_PREFIX_LEN    8
#define CMD_BODY_LEN   1024 * 16 /* FIXME: make this dynamic */

#define ASF_HEADER_LEN (8192 * 2)


#define MMS_PACKET_ERR        0
#define MMS_PACKET_COMMAND    1
#define MMS_PACKET_ASF_HEADER 2
#define MMS_PACKET_ASF_PACKET 3

#define ASF_HEADER_PACKET_ID_TYPE 2
#define ASF_MEDIA_PACKET_ID_TYPE  4


typedef struct mms_buffer_s mms_buffer_t;
struct mms_buffer_s {
  uint8_t *buffer;
  int pos;
};

typedef struct mms_packet_header_s mms_packet_header_t;
struct mms_packet_header_s {
  uint32_t  packet_len;
  uint8_t   flags;
  uint8_t   packet_id_type;
  uint32_t  packet_seq;
};

struct mms_s {

  int           s;
  
  /* url parsing */
  GURI         *guri;
  char         *url;
  char         *proto;
  char         *host;
  int           port;
  char         *user;
  char         *password;
  char         *uri;

  /* command to send */
  char          scmd[CMD_HEADER_LEN + CMD_BODY_LEN];
  char         *scmd_body; /* pointer to &scmd[CMD_HEADER_LEN] */
  int           scmd_len; /* num bytes written in header */
  
  char          str[1024]; /* scratch buffer to built strings */
  
  /* receive buffer */
  uint8_t       buf[BUF_SIZE];
  int           buf_size;
  int           buf_read;
  off_t         buf_packet_seq_offset; /* packet sequence offset residing in
                                          buf */
  
  uint8_t       asf_header[ASF_HEADER_LEN];
  uint32_t      asf_header_len;
  uint32_t      asf_header_read;
  int           seq_num;
  int           num_stream_ids;
  mms_stream_t  streams[ASF_MAX_NUM_STREAMS];
  uint8_t       packet_id_type;
  off_t         start_packet_seq; /* for live streams != 0, need to keep it around */
  int           need_discont; /* whether we need to set start_packet_seq */
  uint32_t      asf_packet_len;
  uint64_t      file_len;
  uint64_t      time_len; /* playback time in 100 nanosecs (10^-7) */
  uint64_t      preroll;
  uint64_t      asf_num_packets;
  char          guid[37];
  int           bandwidth;
  
  int           has_audio;
  int           has_video;
  int           live_flag;
  int           seekable;
  off_t         current_pos;
  int           eos;

  int *need_abort;
};

static int fallback_io_select(void *data, int socket, int state, int timeout_msec)
{
  fd_set set;
  struct timeval tv = { timeout_msec / 1000, (timeout_msec % 1000) * 1000};
  FD_ZERO(&set);
  FD_SET(socket, &set);
  return select(1, (state == MMS_IO_READ_READY) ? &set : NULL,
                   (state == MMS_IO_WRITE_READY) ? &set : NULL, NULL, &tv);
}

static off_t fallback_io_read(void *data, int socket, char *buf, off_t num, int *need_abort)
{
  off_t len = 0, ret;
/*   lprintf("%d\n", fallback_io_select(data, socket, MMS_IO_READ_READY, 1000)); */
  errno = 0;
  int nretry = 600;
  lprintf ("mms: fallback_io_read: need_abort ptr = %p\n", need_abort);
  while (len < num && nretry > 0 && (!need_abort || !(*need_abort)))
  {
    // original read from upstream libmms
    //ret = (off_t)read(socket, buf + len, num - len);
    for (;;) {
        ret = (off_t)recv(socket, buf + len, num - len, MSG_DONTWAIT);
        if ((ret != EAGAIN && ret != EWOULDBLOCK) || (need_abort && *need_abort)) {
            break;
        }
    }
    if (need_abort && *need_abort) {
        return 0;
    }
    if(ret == 0)
      break; /* EOF */
    if(ret < 0) {
      lprintf("mms: read error @ len = %lld: %s\n", (long long int) len,
              strerror(errno));
      switch(errno)
      {
          case EAGAIN:
              usleep (30000); // sleeping 30ms 200 times will give us about 6 sec of time to complete the request
              nretry--;
            continue;
          default:
            /* if already read something, return it, we will fail next time */
            return len ? len : ret; 
      }
    }
    len += ret;
  }
  return len;
}

static off_t fallback_io_write(void *data, int socket, char *buf, off_t num)
{
  return (off_t)write(socket, buf, num);
}

static int fallback_io_tcp_connect(void *data, const char *host, int port, int *need_abort)
{
  
  struct hostent *h;
  int i, s;
  
#ifdef USE_GETHOSTBYNAME
  h = gethostbyname(host);
  if (h == NULL) {
    lprintf("mms: unable to resolve host: %s\n", host);
    return -1;
  }
  char **h_addr_list = h->h_addr_list;
#else
  char sport[10];
  snprintf (sport, 10, "%d", port);
  struct addrinfo *res;
  for (;;) {
      int err = getaddrinfo (host, sport, NULL, &res);
      if (need_abort && *need_abort) {
          if (res) {
              freeaddrinfo(res);
          }
          return -1;
      }

      if (err == EAI_AGAIN) {
          lprintf ("getaddrinfo again\n");
          continue;
      }
      else if (err == 0) {
          lprintf ("getaddrinfo success\n");
          break;
      }
      lprintf ("getaddrinfo err: %d\n", err);
      return -1;
  }
#endif

  s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);  
  if (s == -1) {
    lprintf("mms: failed to create socket: %s\n", strerror(errno));
    return -1;
  }

  if (fcntl (s, F_SETFL, fcntl (s, F_GETFL) | O_NONBLOCK) == -1) {
    lprintf("mms: failed to set socket flags: %s\n", strerror(errno));
    return -1;
  }

#ifdef USE_GETHOSTBYNAME
  for (i = 0; h_addr_list[i]; i++) {
    struct in_addr ia;
    struct sockaddr_in sin;
 
    memcpy (&ia, h_addr_list[i], 4);
    sin.sin_family = AF_INET;
    sin.sin_addr   = ia;
    sin.sin_port   = htons(port);
#else
  struct addrinfo *rp;
  for (rp = res; rp != NULL; rp = rp->ai_next) {
    struct sockaddr_in sin;
    memset (&sin, 0, sizeof (sin));
    int l = rp->ai_addrlen;
    if (l > sizeof (sin)) {
        l = sizeof (sin);
    }
    memcpy (&sin, rp->ai_addr, l);
#endif
    
    time_t t = time (NULL);
    int error = 0;
    while (!need_abort || !(*need_abort)) {
        int res = connect(s, (struct sockaddr *)&sin, sizeof(sin));
        if (res == -1 && (errno == EINPROGRESS || errno == EALREADY)) {
            if (time (NULL) - t > 3) {
                error = -1;
                break;
            }
            usleep(100000);
            continue;
        }
        else if (res == -1 && errno == EISCONN) {
            break;
        }
        else if (res == -1) {
            error = -1;
            break;
        }
    }
    if (need_abort && *need_abort) {
        lprintf ("fallback_io_tcp_connect: aborted\n");
        s = -1;
        break;
    }
//        if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) ==-1 && errno != EINPROGRESS) {
//            continue;
//        }
    if (error) {
        continue;
    }
    
#ifndef USE_GETHOSTBYNAME
    if (res) {
        freeaddrinfo(res);
    }
#endif
    return s;
  }
#ifndef USE_GETHOSTBYNAME
  if (res) {
      freeaddrinfo(res);
  }
#endif
  close(s);
  return -1;
}


static mms_io_t fallback_io =
  {
    &fallback_io_select,
    NULL,
    &fallback_io_read,
    NULL,
    &fallback_io_write,
    NULL,
    &fallback_io_tcp_connect,
    NULL,
  };

static mms_io_t default_io =   {
    &fallback_io_select,
    NULL,
    &fallback_io_read,
    NULL,
    &fallback_io_write,
    NULL,
    &fallback_io_tcp_connect,
    NULL,
  };


#define io_read(io, args...) ((io) ? (io)->read(io->read_data , ## args) : default_io.read(NULL , ## args))
#define io_write(io, args...) ((io) ? (io)->write(io->write_data , ## args) : default_io.write(NULL , ## args))
#define io_select(io, args...) ((io) ? (io)->select(io->select_data , ## args) : default_io.select(NULL , ## args))
#define io_connect(io, args...) ((io) ? (io)->connect(io->connect_data , ## args) : default_io.connect(NULL , ## args))
  
const mms_io_t* mms_get_default_io_impl()
{
  return &default_io;
}

void mms_set_default_io_impl(const mms_io_t *io)
{
  if(io->select)
  {
    default_io.select = io->select;
    default_io.select_data = io->select_data;
  } else
  {
    default_io.select = fallback_io.select;
    default_io.select_data = fallback_io.select_data;
  }
  if(io->read)
  {
    default_io.read = io->read;
    default_io.read_data = io->read_data;
  } else
  {
    default_io.read = fallback_io.read;
    default_io.read_data = fallback_io.read_data;
  }
  if(io->write)
  {
    default_io.write = io->write;
    default_io.write_data = io->write_data;
  } else
  {
    default_io.write = fallback_io.write;
    default_io.write_data = fallback_io.write_data;
  }
  if(io->connect)
  {
    default_io.connect = io->connect;
    default_io.connect_data = io->connect_data;
  } else
  {
    default_io.connect = fallback_io.connect;
    default_io.connect_data = fallback_io.connect_data;
  }
}

static void mms_buffer_init (mms_buffer_t *mms_buffer, uint8_t *buffer) {
  mms_buffer->buffer = buffer;
  mms_buffer->pos = 0;
}

static void mms_buffer_put_8 (mms_buffer_t *mms_buffer, uint8_t value) {

  mms_buffer->buffer[mms_buffer->pos]     = value          & 0xff;

  mms_buffer->pos += 1;
}

#if 0
static void mms_buffer_put_16 (mms_buffer_t *mms_buffer, uint16_t value) {

  mms_buffer->buffer[mms_buffer->pos]     = value          & 0xff;
  mms_buffer->buffer[mms_buffer->pos + 1] = (value  >> 8)  & 0xff;

  mms_buffer->pos += 2;
}
#endif

static void mms_buffer_put_32 (mms_buffer_t *mms_buffer, uint32_t value) {

  mms_buffer->buffer[mms_buffer->pos]     = value          & 0xff;
  mms_buffer->buffer[mms_buffer->pos + 1] = (value  >> 8)  & 0xff;
  mms_buffer->buffer[mms_buffer->pos + 2] = (value  >> 16) & 0xff;
  mms_buffer->buffer[mms_buffer->pos + 3] = (value  >> 24) & 0xff;

  mms_buffer->pos += 4;
}

static int get_guid (unsigned char *buffer, int offset) {
  int i;
  GUID g;
  
  g.Data1 = LE_32(buffer + offset);
  g.Data2 = LE_16(buffer + offset + 4);
  g.Data3 = LE_16(buffer + offset + 6);
  for(i = 0; i < 8; i++) {
    g.Data4[i] = buffer[offset + 8 + i];
  }
  
  for (i = 1; i < GUID_END; i++) {
    if (!memcmp(&g, &guids[i].guid, sizeof(GUID))) {
      lprintf("mms: GUID: %s\n", guids[i].name);
      return i;
    }
  }
  
  lprintf("mms: unknown GUID: 0x%x, 0x%x, 0x%x, "
           "{ 0x%hx, 0x%hx, 0x%hx, 0x%hx, 0x%hx, 0x%hx, 0x%hx, 0x%hx }\n",
           g.Data1, g.Data2, g.Data3,
           g.Data4[0], g.Data4[1], g.Data4[2], g.Data4[3], 
           g.Data4[4], g.Data4[5], g.Data4[6], g.Data4[7]);

  return GUID_ERROR;
}


static void print_command (char *data, int len) {

#ifdef DEBUG
  int i;
  int dir = LE_32 (data + 36) >> 16;
  int comm = LE_32 (data + 36) & 0xFFFF;

  lprintf ("----------------------------------------------\n");
  if (dir == 3) {
    lprintf ("send command 0x%02x, %d bytes\n", comm, len);
  } else {
    lprintf ("receive command 0x%02x, %d bytes\n", comm, len);
  }
  lprintf ("  start sequence %08x\n", LE_32 (data +  0));
  lprintf ("  command id     %08x\n", LE_32 (data +  4));
  lprintf ("  length         %8x \n", LE_32 (data +  8));
  lprintf ("  protocol       %08x\n", LE_32 (data + 12));
  lprintf ("  len8           %8x \n", LE_32 (data + 16));
  lprintf ("  sequence #     %08x\n", LE_32 (data + 20));
  lprintf ("  len8  (II)     %8x \n", LE_32 (data + 32));
  lprintf ("  dir | comm     %08x\n", LE_32 (data + 36));
  if (len >= 4)
    lprintf ("  prefix1        %08x\n", LE_32 (data + 40));
  if (len >= 8)
    lprintf ("  prefix2        %08x\n", LE_32 (data + 44));

  for (i = (CMD_HEADER_LEN + CMD_PREFIX_LEN); i < (CMD_HEADER_LEN + CMD_PREFIX_LEN + len); i += 1) {
    unsigned char c = data[i];
    
    if ((c >= 32) && (c < 128))
      lprintf ("%c", c);
    else
      lprintf (" %02x ", c);
    
  }
  if (len > CMD_HEADER_LEN)
    lprintf ("\n");
  lprintf ("----------------------------------------------\n");
#endif
}  



static int send_command (mms_io_t *io, mms_t *this, int command,
                         uint32_t prefix1, uint32_t prefix2,
                         int length) {
  int    len8;
  off_t  n;
  mms_buffer_t command_buffer;

  len8 = (length + 7) / 8;

  this->scmd_len = 0;

  mms_buffer_init(&command_buffer, this->scmd);
  mms_buffer_put_32 (&command_buffer, 0x00000001);   /* start sequence */
  mms_buffer_put_32 (&command_buffer, 0xB00BFACE);   /* #-)) */
  mms_buffer_put_32 (&command_buffer, len8 * 8 + 32);
  mms_buffer_put_32 (&command_buffer, 0x20534d4d);   /* protocol type "MMS " */
  mms_buffer_put_32 (&command_buffer, len8 + 4);
  mms_buffer_put_32 (&command_buffer, this->seq_num);
  this->seq_num++;
  mms_buffer_put_32 (&command_buffer, 0x0);          /* timestamp */
  mms_buffer_put_32 (&command_buffer, 0x0);
  mms_buffer_put_32 (&command_buffer, len8 + 2);
  mms_buffer_put_32 (&command_buffer, 0x00030000 | command); /* dir | command */
  /* end of the 40 byte command header */
  
  mms_buffer_put_32 (&command_buffer, prefix1);
  mms_buffer_put_32 (&command_buffer, prefix2);

  if (length & 7)
          memset(this->scmd + length + CMD_HEADER_LEN + CMD_PREFIX_LEN, 0, 8 - (length & 7));

  n = io_write(io,  this->s, this->scmd, len8 * 8 + CMD_HEADER_LEN + CMD_PREFIX_LEN);
  if (n != (len8 * 8 + CMD_HEADER_LEN + CMD_PREFIX_LEN)) {
    return 0;
  }

  print_command (this->scmd, length);

  return 1;
}

static int string_utf16(char *dest, char *src, int dest_len)
{
#ifdef USE_ICONV
  char *ip = src, *op = dest;
  size_t ip_len = strlen(src);
  size_t op_len = dest_len - 2; /* reserve 2 bytes for 0 termination */

  iconv_t url_conv = iconv_open("UTF-16LE", "UTF-8");
  if (iconv(url_conv, &ip, &ip_len, &op, &op_len) == (size_t)-1) {
      iconv_close(url_conv);
    lprintf("mms: Error converting uri to unicode: %s\n", strerror(errno));
    return 0;
  }
  iconv_close(url_conv);
  /* 0 terminate the string */
  *op++ = 0;
  *op++ = 0;

  return op - dest;
#else
  char *ip = src, *op = dest;
  size_t ip_len = strlen(src);
  size_t op_len = dest_len - 2;

  int res = deadbeef->junk_iconv (ip, ip_len, op, ip_len * 2, "UTF-8", "UTF-16LE");
  if (res == -1) {
    lprintf("mms: Error converting uri to unicode: %s\n", strerror(errno));
    return 0;
  }
  op += res;
  *op++ = 0;
  *op++ = 0;

  return op - dest;
#endif
}

/*
 * return packet type
 */
static int get_packet_header (mms_io_t *io, mms_t *this, mms_packet_header_t *header) {
  size_t len;
  int packet_type;
  lprintf ("mms: get_packet_header: need_abort ptr = %p\n", this->need_abort);

  header->packet_len     = 0;
  header->packet_seq     = 0;
  header->flags          = 0;
  header->packet_id_type = 0;
  len = io_read(io,  this->s, this->buf, 8, this->need_abort);
  this->buf_packet_seq_offset = -1;
  if (len != 8)
    goto error;

  if (LE_32(this->buf + 4) == 0xb00bface) {
    /* command packet */
    header->flags = this->buf[3];
    len = io_read(io,  this->s, this->buf + 8, 4, this->need_abort);
    if (len != 4)
      goto error;
    
    header->packet_len = LE_32(this->buf + 8) + 4;
    if (header->packet_len > BUF_SIZE - 12) {
        lprintf("mms: get_packet_header error cmd packet length > bufsize\n");
        header->packet_len = 0;
        return MMS_PACKET_ERR;
    }
    packet_type = MMS_PACKET_COMMAND;
  } else {
    header->packet_seq     = LE_32(this->buf);
    header->packet_id_type = this->buf[4];
    header->flags          = this->buf[5];
    header->packet_len     = (LE_16(this->buf + 6) - 8) & 0xffff;
    if (header->packet_id_type == ASF_HEADER_PACKET_ID_TYPE) {
      packet_type = MMS_PACKET_ASF_HEADER;
    } else {
      packet_type = MMS_PACKET_ASF_PACKET;
    }
  }
  
  return packet_type;
  
error:
  lprintf("mms: error reading packet header\n");
  return MMS_PACKET_ERR;
}


static int get_packet_command (mms_io_t *io, mms_t *this, uint32_t packet_len) {


  int  command = 0;
  size_t len;
  
  len = io_read(io,  this->s, this->buf + 12, packet_len, this->need_abort) ;
  //this->buf_packet_seq_offset = -1; // already set in get_packet_header
  if (len != packet_len) {
    lprintf("mms: error reading command packet\n");
    return 0;
  }

  print_command (this->buf, len);
  
  /* check protocol type ("MMS ") */
  if (LE_32(this->buf + 12) != 0x20534D4D) {
    lprintf("mms: unknown protocol type: %c%c%c%c (0x%08X)\n",
            this->buf[12], this->buf[13], this->buf[14], this->buf[15],
            LE_32(this->buf + 12));  
    return 0;
  }

  command = LE_32 (this->buf + 36) & 0xFFFF;
  lprintf("mms: received command = %02x, len: %d\n", command, packet_len);
    
  return command;
}

static int get_answer (mms_io_t *io, mms_t *this) {
  int command = 0;
  mms_packet_header_t header;

  lprintf ("mms: get_answer: need_abort ptr = %p\n", this->need_abort);
  switch (get_packet_header (io, this, &header)) {
    case MMS_PACKET_ERR:
      break;
    case MMS_PACKET_COMMAND:
      command = get_packet_command (io, this, header.packet_len);
      if (command == 0)
        return 0;

      if (command == 0x1b) {
        if (!send_command (io, this, 0x1b, 0, 0, 0)) {
          lprintf("mms: error sending ping reply\n");
          return 0;
        }
        /* FIXME: limit recursion */
        command = get_answer (io, this);
      }
      break;
    case MMS_PACKET_ASF_HEADER:
      lprintf("mms: unexpected asf header packet\n");
      break;
    case MMS_PACKET_ASF_PACKET:
      lprintf("mms: unexpected asf packet\n");
      break;
  }
  
  return command;
}


static int get_asf_header (mms_io_t *io, mms_t *this) {

  off_t len;
  int stop = 0;
  
  this->asf_header_read = 0;
  this->asf_header_len = 0;

  while (!stop) {
    mms_packet_header_t header;
    int command;

    switch (get_packet_header (io, this, &header)) {
      case MMS_PACKET_ERR:
        return 0;
      case MMS_PACKET_COMMAND:
        command = get_packet_command (io, this, header.packet_len);
        if (command == 0)
          return 0;

        if (command == 0x1b) {
          if (!send_command (io, this, 0x1b, 0, 0, 0)) {
            lprintf("mms: error sending ping reply\n");
            return 0;
          }
          command = get_answer (io, this);
        } else {
          lprintf("mms: unexpected command packet\n");
        }
        break;
      case MMS_PACKET_ASF_HEADER:
      case MMS_PACKET_ASF_PACKET:
        if (header.packet_len + this->asf_header_len > ASF_HEADER_LEN) {
            lprintf("mms: asf packet too large: %d\n", 
                    header.packet_len + this->asf_header_len);
            return 0;
        }
        len = io_read(io,  this->s,
                              this->asf_header + this->asf_header_len, header.packet_len, this->need_abort);
        if (len != header.packet_len) {
           lprintf("mms: error reading asf header\n");
           return 0;
        }
        this->asf_header_len += header.packet_len;
        lprintf("mms: header flags: %d\n", header.flags);
        if ((header.flags == 0X08) || (header.flags == 0X0C))
          stop = 1;
        break;
    }
  }
  return 1;
}

static void interp_stream_properties(mms_t *this, int i)
{
  uint16_t flags;
  uint16_t stream_id;
  int      type;
  int      encrypted;
  int      guid;

  guid = get_guid(this->asf_header, i);
  switch (guid) {
    case GUID_ASF_AUDIO_MEDIA:
      type = ASF_STREAM_TYPE_AUDIO;
      this->has_audio = 1;
      break;

    case GUID_ASF_VIDEO_MEDIA:
    case GUID_ASF_JFIF_MEDIA:
    case GUID_ASF_DEGRADABLE_JPEG_MEDIA:
      type = ASF_STREAM_TYPE_VIDEO;
      this->has_video = 1;
      break;
  
    case GUID_ASF_COMMAND_MEDIA:
      type = ASF_STREAM_TYPE_CONTROL;
      break;

    default:
      type = ASF_STREAM_TYPE_UNKNOWN;
  }

  flags = LE_16(this->asf_header + i + 48);
  stream_id = flags & 0x7F;
  encrypted = flags >> 15;

  lprintf("mms: stream object, stream id: %d, type: %d, encrypted: %d\n",
          stream_id, type, encrypted);

  if (this->num_stream_ids < ASF_MAX_NUM_STREAMS) {
    this->streams[this->num_stream_ids].stream_type = type;
    this->streams[this->num_stream_ids].stream_id = stream_id;
    this->num_stream_ids++;
  } else {
    lprintf("mms: too many streams, skipping\n");
  }
}

static void interp_asf_header (mms_t *this) {

  int i;

  this->asf_packet_len = 0;
  this->num_stream_ids = 0;
  this->asf_num_packets = 0;
  /*
   * parse header
   */
   
  i = 30;
  while ((i + 24) <= this->asf_header_len) {
    
    int guid;
    uint64_t length;

    guid = get_guid(this->asf_header, i);
    length = LE_64(this->asf_header + i + 16);

    if ((i + length) > this->asf_header_len) return;

    switch (guid) {
    
      case GUID_ASF_FILE_PROPERTIES:

        this->asf_packet_len = LE_32(this->asf_header + i + 92);
        if (this->asf_packet_len > BUF_SIZE) {
          lprintf("mms: asf packet len too large: %d\n", this->asf_packet_len);
          this->asf_packet_len = 0;
          break;
        }
        this->file_len       = LE_64(this->asf_header + i + 40);
        this->time_len       = LE_64(this->asf_header + i + 64);
        //this->time_len       = LE_64(this->asf_header + i + 72);
        this->preroll        = LE_64(this->asf_header + i + 80);
        lprintf("mms: file object, packet length = %d (%d)\n",
                this->asf_packet_len, LE_32(this->asf_header + i + 96));
        break;

      case GUID_ASF_STREAM_PROPERTIES:
        interp_stream_properties(this, i + 24);
        break;

      case GUID_ASF_STREAM_BITRATE_PROPERTIES:
        {
          uint16_t streams = LE_16(this->asf_header + i + 24);
          uint16_t stream_id;
          int j;

          for(j = 0; j < streams; j++) {
            int stream_index;
            stream_id = LE_16(this->asf_header + i + 24 + 2 + j * 6);
            for(stream_index = 0; stream_index < this->num_stream_ids; stream_index++) {
              if (this->streams[stream_index].stream_id == stream_id)
                break;
            }
            if (stream_index < this->num_stream_ids) {
              this->streams[stream_index].bitrate = LE_32(this->asf_header + i + 24 + 4 + j * 6);
              this->streams[stream_index].bitrate_pos = i + 24 + 4 + j * 6;
              lprintf("mms: stream id %d, bitrate %d\n", stream_id, 
                      this->streams[stream_index].bitrate);
            } else
              lprintf ("mms: unknown stream id %d in bitrate properties\n",
                        stream_id);
          }
        }
        break;
    
      case GUID_ASF_HEADER_EXTENSION:
        {
          if ((24 + 18 + 4) > length)
            break;

          int size = LE_32(this->asf_header + i + 24 + 18);
          int j = 24 + 18 + 4;
          int l;
          lprintf("mms: Extension header data size: %d\n", size);

          while ((j + 24) <= length) {
            guid = get_guid(this->asf_header, i + j);
            l = LE_64(this->asf_header + i + j + 16);

            if ((j + l) > length)
              break;

            if (guid == GUID_ASF_EXTENDED_STREAM_PROPERTIES &&
                (24 + 64) <= l) {
                  int stream_no = LE_16(this->asf_header + i + j + 24 + 48);
                  int name_count = LE_16(this->asf_header + i + j + 24 + 60);
                  int ext_count = LE_16(this->asf_header + i + j + 24 + 62);
                  int ext_j = 24 + 64;
                  int x;

                  lprintf("mms: l: %d\n", l);
                  lprintf("mms: Stream No: %d\n", stream_no);
                  lprintf("mms: ext_count: %d\n", ext_count);

                  // Loop through the number of stream names
                  for (x = 0; x < name_count && (ext_j + 4) <= l; x++) {
                    int lang_id_index;
                    int stream_name_len;

                    lang_id_index = LE_16(this->asf_header + i + j + ext_j);
                    ext_j += 2;

                    stream_name_len = LE_16(this->asf_header + i + j + ext_j);
                    ext_j += stream_name_len + 2;

                    lprintf("mms: Language id index: %d\n", lang_id_index);
                    lprintf("mms: Stream name Len: %d\n", stream_name_len);
                  }

                  // Loop through the number of extension system info
                  for (x = 0; x < ext_count && (ext_j + 22) <= l; x++) {
                    ext_j += 18;
                    int len = LE_16(this->asf_header + i + j + ext_j);
                    ext_j += 4 + len;
                  }

                  lprintf("mms: ext_j: %d\n", ext_j);
                  // Finally, we arrive at the interesting point: The optional Stream Property Object
                  if ((ext_j + 24) <= l) {
                    guid = get_guid(this->asf_header, i + j + ext_j);
                    int len = LE_64(this->asf_header + i + j + ext_j + 16);
                    if (guid == GUID_ASF_STREAM_PROPERTIES &&
                        (ext_j + len) <= l) {
                      interp_stream_properties(this, i + j + ext_j + 24);
                    }
                  } else {
                    lprintf("mms: Sorry, field not long enough\n");
                  }
            }
            j += l;
          }
        }
        break;

      case GUID_ASF_DATA:
        this->asf_num_packets = LE_64(this->asf_header + i + 40 - 24);
        break;
    }

    lprintf("mms: length: %llu\n", (unsigned long long)length);
    i += length;
  }
}

const static char *const mmst_proto_s[] = { "mms", "mmst", NULL };

static int mmst_valid_proto (char *proto) {
  int i = 0;

  if (!proto)
    return 0;

  while(mmst_proto_s[i]) {
    if (!strcasecmp(proto, mmst_proto_s[i])) {
      return 1;
    }
    i++;
  }
  return 0;
}

/*
 * returns 1 on error
 */
static int mms_tcp_connect(mms_io_t *io, mms_t *this) {
  if (!this->port) this->port = MMST_PORT;

  /* 
   * try to connect 
   */
  lprintf("mms: trying to connect to %s on port %d\n", this->host, this->port);
  this->s = io_connect(io,  this->host, this->port, this->need_abort);
  if (this->s == -1) {
    lprintf("mms: failed to connect to %s\n", this->host);
    return 1;
  }

  lprintf("mms: connected\n");
  return 0;
}

static void mms_gen_guid(char guid[]) {
  static char digit[16] = "0123456789ABCDEF";
  int i = 0;

  srand(time(NULL));
  for (i = 0; i < 36; i++) {
    guid[i] = digit[(int) ((16.0*rand())/(RAND_MAX+1.0))];
  }
  guid[8] = '-'; guid[13] = '-'; guid[18] = '-'; guid[23] = '-';
  guid[36] = '\0';
}

const char *status_to_string(int status)
{
  switch (status) {
  case 0x80070003:
    return "Path not found";
  case 0x80070005:
    return "Access Denied";
  default:
    return "Unknown";
  }
}

/*
 * return 0 on error
 */
int static mms_choose_best_streams(mms_io_t *io, mms_t *this) {
  int     i;
  int     video_stream = -1;
  int     audio_stream = -1;
  int     max_arate    = 0;
  int     min_vrate    = 0;
  int     min_bw_left  = 0;
  int     bandwitdh_left;
  int     res;

  /* command 0x33 */
  /* choose the best quality for the audio stream */
  /* i've never seen more than one audio stream */
  for (i = 0; i < this->num_stream_ids; i++) {
    switch (this->streams[i].stream_type) {
      case ASF_STREAM_TYPE_AUDIO:
        if (this->streams[i].bitrate > max_arate) {
          audio_stream = this->streams[i].stream_id;
          max_arate = this->streams[i].bitrate;
        }
        break;
      default:
        break;
    }
  }
  
  /* choose a video stream adapted to the user bandwidth */
  bandwitdh_left = this->bandwidth - max_arate;
  if (bandwitdh_left < 0) {
    bandwitdh_left = 0;
  }
  lprintf("mms: bandwidth %d, left %d\n", this->bandwidth, bandwitdh_left);

  min_bw_left = bandwitdh_left;
  for (i = 0; i < this->num_stream_ids; i++) {
    switch (this->streams[i].stream_type) {
      case ASF_STREAM_TYPE_VIDEO:
        if (((bandwitdh_left - this->streams[i].bitrate) < min_bw_left) &&
            (bandwitdh_left >= this->streams[i].bitrate)) {
          video_stream = this->streams[i].stream_id;
          min_bw_left = bandwitdh_left - this->streams[i].bitrate;
        }
        break;
      default:
        break;
    }
  }  

  /* choose the lower bitrate of */
  if (video_stream == -1 && this->has_video) {
    for (i = 0; i < this->num_stream_ids; i++) {
      switch (this->streams[i].stream_type) {
        case ASF_STREAM_TYPE_VIDEO:
          if ((this->streams[i].bitrate < min_vrate) ||
              (!min_vrate)) {
            video_stream = this->streams[i].stream_id;
            min_vrate = this->streams[i].bitrate;
          }
          break;
        default:
          break;
      }
    }
  }
    
  lprintf("mms: selected streams: audio %d, video %d\n", audio_stream, video_stream);
  memset (this->scmd_body, 0, 40);
  for (i = 1; i < this->num_stream_ids; i++) {
    this->scmd_body [ (i - 1) * 6 + 2 ] = 0xFF;
    this->scmd_body [ (i - 1) * 6 + 3 ] = 0xFF;
    this->scmd_body [ (i - 1) * 6 + 4 ] = this->streams[i].stream_id ;
    this->scmd_body [ (i - 1) * 6 + 5 ] = this->streams[i].stream_id >> 8;
    if ((this->streams[i].stream_id == audio_stream) ||
        (this->streams[i].stream_id == video_stream)) {
      this->scmd_body [ (i - 1) * 6 + 6 ] = 0x00;
      this->scmd_body [ (i - 1) * 6 + 7 ] = 0x00;
    } else {
      lprintf("mms: disabling stream %d\n", this->streams[i].stream_id);
      this->scmd_body [ (i - 1) * 6 + 6 ] = 0x02;
      this->scmd_body [ (i - 1) * 6 + 7 ] = 0x00;
      
      /* forces the asf demuxer to not choose this stream */
      if (this->streams[i].bitrate_pos) {
        if (this->streams[i].bitrate_pos+3 < ASF_HEADER_LEN) {
          this->asf_header[this->streams[i].bitrate_pos    ] = 0;
          this->asf_header[this->streams[i].bitrate_pos + 1] = 0;
          this->asf_header[this->streams[i].bitrate_pos + 2] = 0;
          this->asf_header[this->streams[i].bitrate_pos + 3] = 0;
        } else {
          lprintf("mms: attempt to write beyond asf header limit\n");
        }
      }
    }
  }

  if (this->streams[0].stream_id < 0) {
    lprintf("mms: invalid stream id: %d\n", this->streams[0].stream_id);
    return 0;
  }

  lprintf("mms: send command 0x33\n");
  if (!send_command (io, this, 0x33, this->num_stream_ids, 
                     0xFFFF | this->streams[0].stream_id << 16, 
                     this->num_stream_ids * 6 + 2)) {
    lprintf("mms: mms_choose_best_streams failed\n");
    return 0;
  }

  if ((res = get_answer (io, this)) != 0x21) {
    lprintf("mms: unexpected response: %02x (0x21)\n", res);
    return 0;
  }

  res = LE_32(this->buf + 40);
  if (res != 0) {
    lprintf("mms: error answer 0x21 status: %08x (%s)\n",
            res, status_to_string(res));
    return 0;
  }

  return 1;
}

/*
 * TODO: error messages
 *       network timing request
 */
/* FIXME: got somewhat broken during xine_stream_t->(void*) conversion */
mms_t *mms_connect (mms_io_t *io, void *data, const char *url, int bandwidth, int *need_abort) {
  mms_t  *this;
  int     res;
  uint32_t openid;
  mms_buffer_t command_buffer;
  
  if (!url)
    return NULL;

  /* FIXME: needs proper error-signalling work */
  this = (mms_t*) calloc (1, sizeof (mms_t));

  this->url             = strdup (url);
  this->s               = -1;
  this->seq_num         = 0;
  this->scmd_body       = this->scmd + CMD_HEADER_LEN + CMD_PREFIX_LEN;
  this->asf_header_len  = 0;
  this->asf_header_read = 0;
  this->num_stream_ids  = 0;
  this->asf_packet_len  = 0;
  this->start_packet_seq= 0;
  this->need_discont    = 1;
  this->buf_size        = 0;
  this->buf_read        = 0;
  this->buf_packet_seq_offset = -1;
  this->has_audio       = 0;
  this->has_video       = 0;
  this->bandwidth       = bandwidth;
  this->current_pos     = 0;
  this->eos             = 0;
  this->need_abort      = need_abort;

  this->guri = gnet_uri_new(this->url);
  if(!this->guri) {
    lprintf("mms: invalid url\n");
    goto fail;
  }
  
  /* MMS wants unescaped (so not percent coded) strings */
  gnet_uri_unescape(this->guri);

  this->proto = this->guri->scheme;
  this->user = this->guri->user;
  this->host = this->guri->hostname;
  this->port = this->guri->port;
  this->password = this->guri->passwd;
  this->uri = gnet_mms_helper(this->guri, 0);

  if(!this->uri)
        goto fail;

  if (!mmst_valid_proto(this->proto)) {
    lprintf("mms: unsupported protocol: %s\n", this->proto);
    goto fail;
  }
  
  if (mms_tcp_connect(io, this)) {
    goto fail;
  }
  
  /*
   * let the negotiations begin...
   */

  /* command 0x1 */
  lprintf("mms: send command 0x01\n");
  mms_buffer_init(&command_buffer, this->scmd_body);
  mms_buffer_put_32 (&command_buffer, 0x0003001C);
  mms_gen_guid(this->guid);
  sprintf(this->str, "NSPlayer/7.0.0.1956; {%s}; Host: %s", this->guid,
          this->host);
  res = string_utf16(this->scmd_body + command_buffer.pos, this->str,
                     CMD_BODY_LEN - command_buffer.pos);

  if(!res)
    goto fail;

  if (!send_command(io, this, 1, 0, 0x0004000b, command_buffer.pos + res)) {
    lprintf("mms: failed to send command 0x01\n");
    goto fail;
  }
  
  if ((res = get_answer (io, this)) != 0x01) {
    lprintf("mms: unexpected response: %02x (0x01)\n", res);
    goto fail;
  }

  res = LE_32(this->buf + 40);
  if (res != 0) {
    lprintf("mms: error answer 0x01 status: %08x (%s)\n",
            res, status_to_string(res));
    goto fail;
  }

  /* TODO: insert network timing request here */
  /* command 0x2 */
  lprintf("mms: send command 0x02\n");
  mms_buffer_init(&command_buffer, this->scmd_body);
  mms_buffer_put_32 (&command_buffer, 0x00000000);
  mms_buffer_put_32 (&command_buffer, 0x00989680);
  mms_buffer_put_32 (&command_buffer, 0x00000002);
  res = string_utf16(this->scmd_body + command_buffer.pos,
                     "\\\\192.168.0.129\\TCP\\1037",
                     CMD_BODY_LEN - command_buffer.pos);
  if(!res)
    goto fail;

  if (!send_command(io, this, 2, 0, 0xffffffff, command_buffer.pos + res)) {
    lprintf("mms: failed to send command 0x02\n");
    goto fail;
  }

  switch (res = get_answer (io, this)) {
    case 0x02:
      /* protocol accepted */
      break;
    case 0x03:
      lprintf("mms: protocol failed\n");
      goto fail;
    default:
      lprintf("mms: unexpected response: %02x (0x02 or 0x03)\n", res);
      goto fail;
  }

  res = LE_32(this->buf + 40);
  if (res != 0) {
    lprintf("mms: error answer 0x02 status: %08x (%s)\n",
            res, status_to_string(res));
    goto fail;
  }

  /* command 0x5 */
  {
    mms_buffer_t command_buffer;
    
    lprintf("mms: send command 0x05\n");
    mms_buffer_init(&command_buffer, this->scmd_body);
    mms_buffer_put_32 (&command_buffer, 0x00000000); /* ?? */
    mms_buffer_put_32 (&command_buffer, 0x00000000); /* ?? */

    res = string_utf16(this->scmd_body + command_buffer.pos,
                       this->uri, CMD_BODY_LEN - command_buffer.pos);
    if(!res)
      goto fail;

    if (!send_command(io, this, 5, 1, 0, command_buffer.pos + res)) {
      lprintf("mms: failed to send command 0x05\n");
      goto fail;
    }
  }
  
  switch (res = get_answer (io, this)) {
    case 0x06:
      {
        int xx, yy;
        /* no authentication required */
        openid = LE_32(this->buf + 48);
      
        /* Warning: sdp is not right here */
        xx = this->buf[62];
        yy = this->buf[63];
        this->live_flag = ((xx == 0) && ((yy & 0xf) == 2));
        this->seekable = !this->live_flag;
        lprintf("mms: openid=%d, live: live_flag=%d, xx=%d, yy=%d\n", openid, this->live_flag, xx, yy);
      }
      break;
    case 0x1A:
      /* authentication request, not yet supported */
      lprintf("mms: authentication request, not yet supported\n");
      goto fail;
      break;
    default:
      lprintf("mms: unexpected response: %02x (0x06 or 0x1A)\n", res);
      goto fail;
  }

  res = LE_32(this->buf + 40);
  if (res != 0) {
    lprintf("mms: error answer 0x06 status: %08x (%s)\n",
            res, status_to_string(res));
    goto fail;
  }

  /* command 0x15 */
  lprintf("mms: send command 0x15\n");
  {
    mms_buffer_t command_buffer;
    mms_buffer_init(&command_buffer, this->scmd_body);
    mms_buffer_put_32 (&command_buffer, 0x00000000);                  /* ?? */
    mms_buffer_put_32 (&command_buffer, 0x00008000);                  /* ?? */
    mms_buffer_put_32 (&command_buffer, 0xFFFFFFFF);                  /* ?? */
    mms_buffer_put_32 (&command_buffer, 0x00000000);                  /* ?? */
    mms_buffer_put_32 (&command_buffer, 0x00000000);                  /* ?? */
    mms_buffer_put_32 (&command_buffer, 0x00000000);                  /* ?? */
    mms_buffer_put_32 (&command_buffer, 0x00000000);                  /* ?? */
    mms_buffer_put_32 (&command_buffer, 0x40AC2000);                  /* ?? */
    mms_buffer_put_32 (&command_buffer, ASF_HEADER_PACKET_ID_TYPE);   /* Header Packet ID type */
    mms_buffer_put_32 (&command_buffer, 0x00000000);                  /* ?? */
    if (!send_command (io, this, 0x15, openid, 0, command_buffer.pos)) {
      lprintf("mms: failed to send command 0x15\n");
      goto fail;
    }
  }
  
  if ((res = get_answer (io, this)) != 0x11) {
    lprintf("mms: unexpected response: %02x (0x11)\n", res);
    goto fail;
  }

  res = LE_32(this->buf + 40);
  if (res != 0) {
    lprintf("mms: error answer 0x11 status: %08x (%s)\n",
            res, status_to_string(res));
    goto fail;
  }

  this->num_stream_ids = 0;

  if (!get_asf_header (io, this))
    goto fail;

  interp_asf_header (this);
  if (!this->asf_packet_len || !this->num_stream_ids)
    goto fail;

  if (!mms_choose_best_streams(io, this)) {
    lprintf("mms: mms_choose_best_streams failed\n");
    goto fail;
  }

  /* command 0x07 */
  this->packet_id_type = ASF_MEDIA_PACKET_ID_TYPE;
  {
    mms_buffer_t command_buffer;
    mms_buffer_init(&command_buffer, this->scmd_body);
    mms_buffer_put_32 (&command_buffer, 0x00000000);                  /* 64 byte float timestamp */
    mms_buffer_put_32 (&command_buffer, 0x00000000);                  
    mms_buffer_put_32 (&command_buffer, 0xFFFFFFFF);                  /* ?? */
    mms_buffer_put_32 (&command_buffer, 0xFFFFFFFF);                  /* first packet sequence */
    mms_buffer_put_8  (&command_buffer, 0xFF);                        /* max stream time limit (3 bytes) */
    mms_buffer_put_8  (&command_buffer, 0xFF);
    mms_buffer_put_8  (&command_buffer, 0xFF);
    mms_buffer_put_8  (&command_buffer, 0x00);                        /* stream time limit flag */
    mms_buffer_put_32 (&command_buffer, this->packet_id_type);    /* asf media packet id type */
    if (!send_command (io, this, 0x07, 1, 0x0001FFFF, command_buffer.pos)) {
      lprintf("mms: failed to send command 0x07\n");
      goto fail;
    }
  }

  lprintf("mms: connect: passed\n");
 
  return this;

fail:
  if (this->s != -1)
    close (this->s);
  if (this->url)
    free(this->url);
  if (this->guri)
    gnet_uri_delete(this->guri);
  if (this->uri)
    free(this->uri);

  free (this);
  return NULL;
}

static int get_media_packet (mms_io_t *io, mms_t *this) {
  mms_packet_header_t header;
  off_t len;
  
  switch (get_packet_header (io, this, &header)) {
    case MMS_PACKET_ERR:
      return 0;
    
    case MMS_PACKET_COMMAND:
      {
        int command;
        command = get_packet_command (io, this, header.packet_len);
      
        switch (command) {
          case 0:
            return 0;

          case 0x1e:
            {
              uint32_t error_code;

              /* Warning: sdp is incomplete. Do not stop if error_code==1 */
              error_code = LE_32(this->buf + CMD_HEADER_LEN);
              lprintf("mms: End of the current stream. Continue=%d\n", error_code);

              if (error_code == 0) {
                this->eos = 1;
                return 0;
              }
              
            }
            break;
  
          case 0x20:
            {
              lprintf("mms: new stream.\n");
              /* asf header */
              if (!get_asf_header (io, this)) {
                lprintf("mms: failed to read new ASF header\n");
                return 0;
              }

              interp_asf_header (this);
              if (!this->asf_packet_len || !this->num_stream_ids)
                return 0;

              if (!mms_choose_best_streams(io, this))
                return 0;

              /* send command 0x07 */
              /* TODO: ugly */
              /* command 0x07 */
              {
                mms_buffer_t command_buffer;
                mms_buffer_init(&command_buffer, this->scmd_body);
                mms_buffer_put_32 (&command_buffer, 0x00000000);                  /* 64 byte float timestamp */
                mms_buffer_put_32 (&command_buffer, 0x00000000);                  
                mms_buffer_put_32 (&command_buffer, 0xFFFFFFFF);                  /* ?? */
                mms_buffer_put_32 (&command_buffer, 0xFFFFFFFF);                  /* first packet sequence */
                mms_buffer_put_8  (&command_buffer, 0xFF);                        /* max stream time limit (3 bytes) */
                mms_buffer_put_8  (&command_buffer, 0xFF);
                mms_buffer_put_8  (&command_buffer, 0xFF);
                mms_buffer_put_8  (&command_buffer, 0x00);                        /* stream time limit flag */
                mms_buffer_put_32 (&command_buffer, ASF_MEDIA_PACKET_ID_TYPE);    /* asf media packet id type */
                if (!send_command (io, this, 0x07, 1, 0x0001FFFF, command_buffer.pos)) {
                  lprintf("mms: failed to send command 0x07\n");
                  return 0;
                }
              }
              this->current_pos = 0;
              
              /* I don't know if this ever happens with none live (and thus
                 seekable streams), but I do know that if it happens all bets
                 with regards to seeking are off */
              this->seekable = 0;
            }
            break;

          case 0x1b:
            {
              if (!send_command (io, this, 0x1b, 0, 0, 0)) {
                lprintf("mms: error sending ping reply\n");
                return 0;
              }
            }
            break;
          
          case 0x05:
            break;
  
          default:
            lprintf("mms: unexpected mms command %02x\n", command);
        }
        this->buf_size = 0;
      }
      break;

    case MMS_PACKET_ASF_HEADER:
      lprintf("mms: unexpected asf header packet\n");
      this->buf_size = 0;
      break;

    case MMS_PACKET_ASF_PACKET:
      {
        /* media packet */

        /* FIXME: probably needs some more sophisticated logic, but
           until we do seeking, this should work */
        if(this->need_discont &&
           header.packet_id_type == ASF_MEDIA_PACKET_ID_TYPE)
        {
          this->need_discont = 0;
          this->start_packet_seq = header.packet_seq;
        }
        
        if (header.packet_len > this->asf_packet_len) {
          lprintf("mms: invalid asf packet len: %d bytes\n", header.packet_len);
          return 0;
        }
    
        /* simulate a seek */
        this->current_pos = (off_t)this->asf_header_len +
          ((off_t)header.packet_seq - this->start_packet_seq) * (off_t)this->asf_packet_len;

        len = io_read(io,  this->s, this->buf, header.packet_len, this->need_abort);
        if (len != header.packet_len) {
          lprintf("mms: error reading asf packet\n");
          return 0;
        }

        /* explicit padding with 0 */
        memset(this->buf + header.packet_len, 0, this->asf_packet_len - header.packet_len);
        if (header.packet_id_type == this->packet_id_type) {
          this->buf_size = this->asf_packet_len;
          this->buf_packet_seq_offset =
            header.packet_seq - this->start_packet_seq;
        } else {
          this->buf_size = 0;
          // Don't set this packet sequence for reuse in seek(), since the
          // subsequence packet may be discontinued.
          //this->buf_packet_seq_offset = header.packet_seq;
          // already set to -1 in get_packet_header
          //this->buf_packet_seq_offset = -1;
        }
      }
      break;
  }

  return 1;
}


int mms_peek_header (mms_t *this, char *data, int maxsize) {

  int len;

  len = (this->asf_header_len < maxsize) ? this->asf_header_len : maxsize;

  memcpy(data, this->asf_header, len);
  return len;
}

int mms_read (mms_io_t *io, mms_t *this, char *data, int len, int *need_abort) {
  int total;

  total = 0;
  while (total < len && !this->eos && (!need_abort || !(*need_abort))) {

    if (this->asf_header_read < this->asf_header_len) {
      int n, bytes_left;

      bytes_left = this->asf_header_len - this->asf_header_read ;

      if ((len - total) < bytes_left)
        n = len-total;
      else
        n = bytes_left;

      memcpy (&data[total], &this->asf_header[this->asf_header_read], n);

      this->asf_header_read += n;
      total += n;
      this->current_pos += n;
    } else {

      int n, bytes_left;

      bytes_left = this->buf_size - this->buf_read;
      if (bytes_left == 0) {
        this->buf_size = this->buf_read = 0;
        if (!get_media_packet (io, this)) {
          lprintf("mms: get_media_packet failed\n");
          return total;
        }
        bytes_left = this->buf_size;
      }

      if ((len - total) < bytes_left)
        n = len - total;
      else
        n = bytes_left;

      memcpy (&data[total], &this->buf[this->buf_read], n);

      this->buf_read += n;
      total += n;
      this->current_pos += n;
    }
  }

  if (need_abort && *need_abort) {
      lprintf ("mms_read aborted\n");
      return -1;
  }
  return total;
}

// To be inline function?
static int mms_request_data_packet (mms_io_t *io, mms_t *this,
  double time_sec, unsigned long first_packet, unsigned long time_msec_limit) {
  /* command 0x07 */
  {
    mms_buffer_t command_buffer;
    //mms_buffer_init(&command_buffer, this->scmd_body);
    //mms_buffer_put_32 (&command_buffer, 0x00000000);                  /* 64 byte float timestamp */
    //mms_buffer_put_32 (&command_buffer, 0x00000000);                  
    memcpy(this->scmd_body, &time_sec, 8);
    mms_buffer_init(&command_buffer, this->scmd_body+8);
    mms_buffer_put_32 (&command_buffer, 0xFFFFFFFF);                  /* ?? */
    //mms_buffer_put_32 (&command_buffer, 0xFFFFFFFF);                  /* first packet sequence */
    mms_buffer_put_32 (&command_buffer, first_packet);                  /* first packet sequence */
    //mms_buffer_put_8  (&command_buffer, 0xFF);                        /* max stream time limit (3 bytes) */
    //mms_buffer_put_8  (&command_buffer, 0xFF);
    //mms_buffer_put_8  (&command_buffer, 0xFF);
    //mms_buffer_put_8  (&command_buffer, 0x00);                        /* stream time limit flag */
    mms_buffer_put_32 (&command_buffer, time_msec_limit & 0x00FFFFFF);/* max stream time limit (3 bytes) */
    mms_buffer_put_32 (&command_buffer, this->packet_id_type);    /* asf media packet id type */
    if (!send_command (io, this, 0x07, 1, 0x0001FFFF, 8+command_buffer.pos)) {
      lprintf("mms: failed to send command 0x07\n");
      return 0;
    }
  }
  /* TODO: adjust current_pos, considering asf_header_read */
  return 1;
}

int mms_request_time_seek (mms_io_t *io, mms_t *this, double time_sec) {
  if (++this->packet_id_type <= ASF_MEDIA_PACKET_ID_TYPE)
    this->packet_id_type = ASF_MEDIA_PACKET_ID_TYPE+1;
  //return mms_request_data_packet (io, this, time_sec, 0xFFFFFFFF, 0x00FFFFFF);
  // also adjust time by preroll
  return mms_request_data_packet (io, this,
                                  time_sec+(double)(this->preroll)/1000,
                                  0xFFFFFFFF, 0x00FFFFFF);
}

// set current_pos to the first byte of the requested packet by peeking at
// the first packet.
// To be inline function?
static int peek_and_set_pos (mms_io_t *io, mms_t *this) {
  uint8_t saved_buf[BUF_SIZE];
  int     saved_buf_size;
  off_t   saved_buf_packet_seq_offset;
  // save buf and buf_size that may be changed in get_media_packet()
  memcpy(saved_buf, this->buf, this->buf_size);
  saved_buf_size = this->buf_size;
  saved_buf_packet_seq_offset = this->buf_packet_seq_offset;
  //this->buf_size = this->buf_read = 0; // reset buf, only if success peeking
  this->buf_size = 0;
  while (!this->eos) {
    // get_media_packet() will set current_pos if data packet is read.
    if (!get_media_packet (io, this)) {
      lprintf("mms: get_media_packet failed\n");
      // restore buf and buf_size that may be changed in get_media_packet()
      memcpy(this->buf, saved_buf, saved_buf_size);
      this->buf_size = saved_buf_size;
      this->buf_packet_seq_offset = saved_buf_packet_seq_offset;
      return 0;
    }
    if (this->buf_size > 0) break;
  }
  // flush header and reset buf_read, only if success peeking
  this->asf_header_read = this->asf_header_len;
  this->buf_read = 0;
  return 1;
  //return this->current_pos;
}

// send time seek request, and update current_pos corresponding to the next
// requested packet
// Note that, the current_pos will always does not less than asf_header_len
int mms_time_seek (mms_io_t *io, mms_t *this, double time_sec) {
  if (!this->seekable)
    return 0;

  if (!mms_request_time_seek (io, this, time_sec)) return 0;
  return peek_and_set_pos (io, this);
}

// http://sdp.ppona.com/zipfiles/MMSprotocol_pdf.zip said that, this
// packet_seq value make no difference in version 9 servers.
// But from my experiment with
// mms://202.142.200.130/tltk/56k/tltkD2006-08-08ID-7209.wmv and
// mms://202.142.200.130/tltk/56k/tltkD2006-09-01ID-7467.wmv (the url may valid
// in only 2-3 months) whose server is version 9, it does response and return
// the requested packet.
int mms_request_packet_seek (mms_io_t *io, mms_t *this,
                             unsigned long packet_seq) {
  if (++this->packet_id_type <= ASF_MEDIA_PACKET_ID_TYPE)
    this->packet_id_type = ASF_MEDIA_PACKET_ID_TYPE+1;
  return mms_request_data_packet (io, this, 0, packet_seq, 0x00FFFFFF);
}

// send packet seek request, and update current_pos corresponding to the next
// requested packet
// Note that, the current_pos will always does not less than asf_header_len
// Not export this function.  Let user use mms_seek() instead?
static int mms_packet_seek (mms_io_t *io, mms_t *this,
                            unsigned long packet_seq) {
  if (!mms_request_packet_seek (io, this, packet_seq)) return 0;
  return peek_and_set_pos (io, this);
}

/*
TODO: To use this table to calculate buf_packet_seq_offset rather than store
and retrieve it from this->buf_packet_seq_offset?
current_packet_seq == (current_pos - asf_header_len) / asf_packet_len
current_packet_seq == -1 if current_pos < asf_header_len
buf_packet_seq_offset indicating which packet sequence are residing in the buf.
Possible status after read(), "last" means last value or unchange.
current_packet_seq | buf_read       | buf_size  | buf_packet_seq_offset
-------------------+----------------+-----------+---------------
<= 0               | 0 (last)       | 0 (last)  | none
<= 0               | 0 (last)       | 0 (last)  | eos at #0
<= 0               | 0 (last)       | 0 (last)  | eos at > #0
<= 0               | 0 (last)       | > 0 (last)| #0
<= 0               | buf_size (last)| > 0 (last)| > #0
> 0                | 0              | 0         | eos at current_packet_seq
> 0                | 0(never happen)| > 0       | (never happen)
> 0                | buf_size       | > 0       | current_packet_seq-1
*/
// TODO: How to handle seek() in multi stream source?
// The stream that follows 0x20 ("new stream") command.
off_t mms_seek (mms_io_t *io, mms_t *this, off_t offset, int origin) {
  off_t dest;
  off_t dest_packet_seq;
  //off_t buf_packet_seq_offset;
  
  if (!this->seekable)
    return this->current_pos;
  
  switch (origin) {
    case SEEK_SET:
      dest = offset;
      break;
    case SEEK_CUR:
      dest = this->current_pos + offset;
      break;
    case SEEK_END:
      //if (this->asf_num_packets == 0) {
      //  //printf ("input_mms: unknown end position in seek!\n");
      //  return this->current_pos;
      //}
      dest = mms_get_length (this) + offset;
    default:
      fprintf (stderr, "input_mms: unknown origin in seek!\n");
      return this->current_pos;
  }

  dest_packet_seq = dest - this->asf_header_len;
  //if (dest_packet_seq > 0) dest_packet_seq /= this->asf_packet_len;
  dest_packet_seq = dest_packet_seq >= 0 ?
    dest_packet_seq / this->asf_packet_len : -1;
#if 0
  // buf_packet_seq_offset will identify which packet sequence are residing in
  // the buf.
#if 1 /* To show both of the alternate styles :D */
  buf_packet_seq_offset = this->current_pos - this->asf_header_len;
  //if (buf_packet_seq_offset > 0) buf_packet_seq_offset /= this->asf_packet_len;
  buf_packet_seq_offset = buf_packet_seq_offset >= 0 ?
    buf_packet_seq_offset / this->asf_packet_len : -1;
  // Note: buf_read == buf_size == 0 may means that it is eos,
  //       eos means that the packet has been peek'ed.
  if (this->buf_read >= this->buf_size && this->buf_size > 0 &&
      buf_packet_seq_offset >= 0 ||
      // assuming packet not peek'ed in the following condition
      /*this->buf_read >= this->buf_size && */this->buf_size == 0 &&
      buf_packet_seq_offset == 0)
    // The buf is all read but the packet has not been peek'ed.
    --buf_packet_seq_offset;
#else
  buf_packet_seq_offset = this->current_pos - this->asf_header_len - 1;
  //if (buf_packet_seq_offset > 0) buf_packet_seq_offset /= this->asf_packet_len;
  buf_packet_seq_offset = buf_packet_seq_offset >= 0 ?
    buf_packet_seq_offset / this->asf_packet_len : -1;
  // Note: buf_read == buf_size == 0 may means that it is eos,
  //       eos means that the packet has been peek'ed.
  if (this->buf_read == 0/* && buf_packet_seq_offset >= 0*/)
    // Since the packet has just been peek'ed.
    ++buf_packet_seq_offset;
#endif
#endif

  if (dest_packet_seq < 0) {
    if (this->buf_packet_seq_offset > 0) {
      if (!mms_request_packet_seek (io, this, 0xFFFFFFFF))
        return this->current_pos;
#if 1
      // clear buf
      this->buf_read = this->buf_size = 0;
      this->buf_packet_seq_offset = -1;
    } else {
#else
      // clear buf
      this->buf_read = this->buf_size;
      // Set this packet sequence not to be reused, since the subsequence
      // packet may be discontinued.
      this->buf_packet_seq_offset = -1;
    // don't reset buf_read if buf_packet_seq_offset < 0, since the previous
    // buf may not be cleared.
    } else if (this->buf_packet_seq_offset == 0) {
#endif
      // reset buf_read
      this->buf_read = 0;
    }
    this->asf_header_read = dest;
    return this->current_pos = dest;
  }
  // dest_packet_seq >= 0
  if (this->asf_num_packets && dest == this->asf_header_len +
      this->asf_num_packets*this->asf_packet_len) {
    // Requesting the packet beyond the last packet, can cause the server to
    // not return any packet or any eos command.  This can cause
    // mms_packet_seek() to hang.
    // This is to allow seeking at end of stream, and avoid hanging.
    --dest_packet_seq;
  }
  if (dest_packet_seq != this->buf_packet_seq_offset) {
    if (this->asf_num_packets && dest_packet_seq >= this->asf_num_packets) {
      // Do not seek beyond the last packet.
      return this->current_pos;
    }
    if (!mms_packet_seek (io, this, this->start_packet_seq + dest_packet_seq))
      return this->current_pos;
    // Check if current_pos is correct.
    // This can happen if the server ignore packet seek command.
    // If so, just return unchanged current_pos, rather than trying to
    // mms_read() to reach the destination pos.
    // It should let the caller to decide to choose the alternate method, such
    // as, mms_time_seek() and/or mms_read() until the destination pos is
    // reached.
    if (dest_packet_seq != this->buf_packet_seq_offset)
      return this->current_pos;
    // This has already been set in mms_packet_seek().
    //if (current_packet_seq < 0)
    //  this->asf_header_read = this->asf_header_len;
    //this->asf_header_read = this->asf_header_len;
  }
  // eos is reached ?
  //if (this->buf_size <= 0) return this->current_pos;
  //this->buf_read = (dest - this->asf_header_len) % this->asf_packet_len;
  this->buf_read = dest -
    (this->asf_header_len + dest_packet_seq*this->asf_packet_len);
  // will never happen ?
  //if (this->buf_size <= this->buf_read) return this->current_pos;
  return this->current_pos = dest;
}


void mms_close (mms_t *this) {
  if (this->s != -1)
    close (this->s);
  if (this->url)
    free(this->url);
  if (this->guri)
    gnet_uri_delete(this->guri);
  if (this->uri)
    free(this->uri);

  free (this);
}

double mms_get_time_length (mms_t *this) {
  return (double)(this->time_len) / 1e7;
}

uint64_t mms_get_raw_time_length (mms_t *this) {
  return this->time_len;
}

uint32_t mms_get_length (mms_t *this) {
  /* we could / should return this->file_len here, but usually this->file_len
     is longer then the calculation below, as usually an asf file contains an
     asf index object after the data stream. However since we do not have a
     (known) way to get to this index object through mms, we return a
     calculated size of what we can get to when we know. */
  if (this->asf_num_packets)
    return this->asf_header_len + this->asf_num_packets*this->asf_packet_len;
  else
    return this->file_len;
}

off_t mms_get_current_pos (mms_t *this) {
  return this->current_pos;
}

uint32_t mms_get_asf_header_len (mms_t *this) {
  return this->asf_header_len;
}

uint64_t mms_get_asf_packet_len (mms_t *this) {
  return this->asf_packet_len;
}

int mms_get_seekable (mms_t *this) {
  return this->seekable;
}
