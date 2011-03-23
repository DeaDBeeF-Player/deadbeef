/* UADE
 *
 * Copyright 2005 Heikki Orsila <heikki.orsila@iki.fi>
 *
 * This source code module is dual licensed under GPL and Public Domain.
 * Hence you may use _this_ module (not another code module) in any way you
 * want in your projects.
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <string.h>

#include "uadeipc.h"
#include "ossupport.h"
#include "sysincludes.h"


static int valid_message(struct uade_msg *uc);


void uade_check_fix_string(struct uade_msg *um, size_t maxlen)
{
  uint8_t *s = (uint8_t *) um->data;
  size_t safelen;
  if (um->size == 0) {
    s[0] = 0;
    fprintf(stderr, "zero string detected\n");
  }
  safelen = 0;
  while (s[safelen] != 0 && safelen < maxlen)
    safelen++;
  if (safelen == maxlen) {
    safelen--;
    fprintf(stderr, "too long a string\n");
    s[safelen] = 0;
  }
  if (um->size != (safelen + 1)) {
    fprintf(stderr, "string size does not match\n");
    um->size = safelen + 1;
    s[safelen] = 0;
  }
}


static ssize_t get_more(size_t bytes, struct uade_ipc *ipc)
{
  if (ipc->inputbytes < bytes) {
    ssize_t s = uade_ipc_read(ipc->input, &ipc->inputbuffer[ipc->inputbytes], bytes - ipc->inputbytes);
    if (s <= 0)
      return -1;
    ipc->inputbytes += s;
  }
  return 0;
}


static void copy_from_inputbuffer(void *dst, int bytes, struct uade_ipc *ipc)
{
  if (ipc->inputbytes < bytes) {
    fprintf(stderr, "not enough bytes in input buffer\n");
    exit(-1);
  }
  memcpy(dst, ipc->inputbuffer, bytes);
  memmove(ipc->inputbuffer, &ipc->inputbuffer[bytes], ipc->inputbytes - bytes);
  ipc->inputbytes -= bytes;
}


int uade_parse_u32_message(uint32_t *u1, struct uade_msg *um)
{
  if (um->size != 4)
    return -1;
  *u1 = ntohl(* (uint32_t *) um->data);
  return 0;
}


int uade_parse_two_u32s_message(uint32_t *u1, uint32_t *u2,
				struct uade_msg *um)
{
  if (um->size != 8)
    return -1;
  *u1 = ntohl(((uint32_t *) um->data)[0]);
  *u2 = ntohl(((uint32_t *) um->data)[1]);
  return 0;
}


int uade_receive_message(struct uade_msg *um, size_t maxbytes,
			 struct uade_ipc *ipc)
{
  size_t fullsize;

  if (ipc->state == UADE_INITIAL_STATE) {
    ipc->state = UADE_R_STATE;
  } else if (ipc->state == UADE_S_STATE) {
    fprintf(stderr, "protocol error: receiving in S state is forbidden\n");
    return -1;
  }

  if (ipc->inputbytes < sizeof(*um)) {
    if (get_more(sizeof(*um), ipc))
      return 0;
  }

  copy_from_inputbuffer(um, sizeof(*um), ipc);

  um->msgtype = ntohl(um->msgtype);
  um->size = ntohl(um->size);

  if (!valid_message(um))
    return -1;

  fullsize = um->size + sizeof(*um);
  if (fullsize > maxbytes) {
    fprintf(stderr, "too big a command: %zu\n", fullsize);
    return -1;
  }
  if (ipc->inputbytes < um->size) {
    if (get_more(um->size, ipc))
      return -1;
  }
  copy_from_inputbuffer(&um->data, um->size, ipc);

  if (um->msgtype == UADE_COMMAND_TOKEN)
    ipc->state = UADE_S_STATE;

  return 1;
}


int uade_receive_short_message(enum uade_msgtype msgtype, struct uade_ipc *ipc)
{
  struct uade_msg um;

  if (ipc->state == UADE_INITIAL_STATE) {
    ipc->state = UADE_R_STATE;
  } else if (ipc->state == UADE_S_STATE) {
    fprintf(stderr, "protocol error: receiving (%d) in S state is forbidden\n", msgtype);
    return -1;
  }

  if (uade_receive_message(&um, sizeof(um), ipc) <= 0) {
    fprintf(stderr, "can not receive short message: %d\n", msgtype);
    return -1;
  }
  return (um.msgtype == msgtype) ? 0 : -1;
}


int uade_receive_string(char *s, enum uade_msgtype com,
			size_t maxlen, struct uade_ipc *ipc)
{
  uint8_t commandbuf[UADE_MAX_MESSAGE_SIZE];
  struct uade_msg *um = (struct uade_msg *) commandbuf;
  int ret;

  if (ipc->state == UADE_INITIAL_STATE) {
    ipc->state = UADE_R_STATE;
  } else if (ipc->state == UADE_S_STATE) {
    fprintf(stderr, "protocol error: receiving in S state is forbidden\n");
    return -1;
  }

  ret = uade_receive_message(um, UADE_MAX_MESSAGE_SIZE, ipc);
  if (ret <= 0)
    return ret;
  if (um->msgtype != com)
    return -1;
  if (um->size == 0)
    return -1;
  if (um->size != (strlen((char *) um->data) + 1))
    return -1;
  strlcpy(s, (char *) um->data, maxlen);
  return 1;
}


int uade_send_message(struct uade_msg *um, struct uade_ipc *ipc)
{
  uint32_t size = um->size;

  if (ipc->state == UADE_INITIAL_STATE) {
    ipc->state = UADE_S_STATE;
  } else if (ipc->state == UADE_R_STATE) {
    fprintf(stderr, "protocol error: sending in R state is forbidden\n");
    return -1;
  }

  if (!valid_message(um))
    return -1;
  if (um->msgtype == UADE_COMMAND_TOKEN)
    ipc->state = UADE_R_STATE;
  um->msgtype = htonl(um->msgtype);
  um->size = htonl(um->size);
  if (uade_ipc_write(ipc->output, um, sizeof(*um) + size) < 0)
    return -1;

  return 0;
}


int uade_send_short_message(enum uade_msgtype msgtype, struct uade_ipc *ipc)
{
  struct uade_msg msg = {.msgtype = msgtype};

  if (uade_send_message(&msg, ipc)) {
    fprintf(stderr, "can not send short message: %d\n", msgtype);
    return -1;
  }
  return 0;
}


int uade_send_string(enum uade_msgtype com, const char *str, struct uade_ipc *ipc)
{
  uint32_t size = strlen(str) + 1;
  struct uade_msg um = {.msgtype = ntohl(com), .size = ntohl(size)};

  if (ipc->state == UADE_INITIAL_STATE) {
    ipc->state = UADE_S_STATE;
  } else if (ipc->state == UADE_R_STATE) {
    fprintf(stderr, "protocol error: sending in R state is forbidden\n");
    return -1;
  }

  if ((sizeof(um) + size) > UADE_MAX_MESSAGE_SIZE)
    return -1;
  if (uade_ipc_write(ipc->output, &um, sizeof(um)) < 0)
    return -1;
  if (uade_ipc_write(ipc->output, str, size) < 0)
    return -1;

  return 0;
}


int uade_send_u32(enum uade_msgtype com, uint32_t u, struct uade_ipc *ipc)
{
  uint8_t space[UADE_MAX_MESSAGE_SIZE];
  struct uade_msg *um = (struct uade_msg *) space;
  um->msgtype = com;
  um->size = 4;
  * (uint32_t *) um->data = htonl(u);
  return uade_send_message(um, ipc);
}


int uade_send_two_u32s(enum uade_msgtype com, uint32_t u1, uint32_t u2,
		       struct uade_ipc *ipc)
{
  uint8_t space[UADE_MAX_MESSAGE_SIZE];
  struct uade_msg *um = (struct uade_msg *) space;
  um->msgtype = com;
  um->size = 8;
  ((uint32_t *) um->data)[0] = htonl(u1);
  ((uint32_t *) um->data)[1] = htonl(u2);
  return uade_send_message(um, ipc);
}


void uade_set_peer(struct uade_ipc *ipc, int peer_is_client, const char *input, const char *output)
{
  assert(peer_is_client == 0 || peer_is_client == 1);
  assert(input != NULL);
  assert(output != NULL);

  *ipc = (struct uade_ipc) {.state = UADE_INITIAL_STATE,
			    .input= uade_ipc_set_input(input),
			    .output = uade_ipc_set_output(output)};
}


static int valid_message(struct uade_msg *um)
{
  size_t len;
  if (um->msgtype <= UADE_MSG_FIRST || um->msgtype >= UADE_MSG_LAST) {
    fprintf(stderr, "unknown command: %u\n", (unsigned int) um->msgtype);
    return 0;
  }
  len = sizeof(*um) + um->size;
  if (len > UADE_MAX_MESSAGE_SIZE) {
    fprintf(stderr, "too long a message: %zu\n", len);
    return 0;
  }
  return 1;
}
