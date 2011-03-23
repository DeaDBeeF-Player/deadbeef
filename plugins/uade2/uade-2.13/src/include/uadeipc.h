#ifndef _UADEIPC_H_
#define _UADEIPC_H_

#include <stdlib.h>
#include <stdint.h>

#include "uadeutils.h"

#define UADE_MAX_MESSAGE_SIZE (4096)

enum uade_msgtype {
  UADE_MSG_FIRST = 0,
  UADE_COMMAND_ACTIVATE_DEBUGGER,
  UADE_COMMAND_CHANGE_SUBSONG,
  UADE_COMMAND_CONFIG,
  UADE_COMMAND_SCORE,
  UADE_COMMAND_PLAYER,
  UADE_COMMAND_MODULE,
  UADE_COMMAND_READ,
  UADE_COMMAND_REBOOT,
  UADE_COMMAND_SET_SUBSONG,
  UADE_COMMAND_IGNORE_CHECK,
  UADE_COMMAND_SONG_END_NOT_POSSIBLE,
  UADE_COMMAND_SET_NTSC,
  UADE_COMMAND_FILTER,
  UADE_COMMAND_SET_FREQUENCY,
  UADE_COMMAND_SET_PLAYER_OPTION,
  UADE_COMMAND_SET_RESAMPLING_MODE,
  UADE_COMMAND_SPEED_HACK,
  UADE_COMMAND_TOKEN,
  UADE_COMMAND_USE_TEXT_SCOPE,
  UADE_REPLY_MSG,
  UADE_REPLY_CANT_PLAY,
  UADE_REPLY_CAN_PLAY,
  UADE_REPLY_SONG_END,
  UADE_REPLY_SUBSONG_INFO,
  UADE_REPLY_PLAYERNAME,
  UADE_REPLY_MODULENAME,
  UADE_REPLY_FORMATNAME,
  UADE_REPLY_DATA,
  UADE_MSG_LAST
};

struct uade_msg {
  uint32_t msgtype;
  uint32_t size;
  uint8_t data[0];
} __attribute__((packed));

enum uade_control_state {
  UADE_INITIAL_STATE = 0,
  UADE_R_STATE,
  UADE_S_STATE
};

struct uade_ipc {
  void *input;
  void *output;
  unsigned int inputbytes;
  char inputbuffer[UADE_MAX_MESSAGE_SIZE];
  enum uade_control_state state;
};

void uade_check_fix_string(struct uade_msg *um, size_t maxlen);
int uade_parse_u32_message(uint32_t *u1, struct uade_msg *um);
int uade_parse_two_u32s_message(uint32_t *u1, uint32_t *u2, struct uade_msg *um);
int uade_receive_message(struct uade_msg *um, size_t maxbytes, struct uade_ipc *ipc);
int uade_receive_short_message(enum uade_msgtype msgtype, struct uade_ipc *ipc);
int uade_receive_string(char *s, enum uade_msgtype msgtype, size_t maxlen, struct uade_ipc *ipc);
int uade_send_message(struct uade_msg *um, struct uade_ipc *ipc);
int uade_send_short_message(enum uade_msgtype msgtype, struct uade_ipc *ipc);
int uade_send_string(enum uade_msgtype msgtype, const char *str, struct uade_ipc *ipc);
int uade_send_u32(enum uade_msgtype com, uint32_t u, struct uade_ipc *ipc);
int uade_send_two_u32s(enum uade_msgtype com, uint32_t u1, uint32_t u2, struct uade_ipc *ipc);
void uade_set_peer(struct uade_ipc *ipc, int peer_is_client, const char *input, const char *output);

#endif
