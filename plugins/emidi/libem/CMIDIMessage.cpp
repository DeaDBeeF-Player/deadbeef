#include <string.h>
#include "CMIDIMessage.hpp"

#if defined (_MSC_VER)
#if defined (_DEBUG)
#define new new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#endif

using namespace dsa;

CMIDIMsg::CMIDIMsg(MsgType type,int ch, const BYTE *data, DWORD length) 
  : m_type(type), m_ch(ch), m_length(length)
{
  if(length) {
    m_data = new BYTE [length];
    memcpy(m_data,data,(size_t)length);
  } else {
    m_data = NULL;
  }
}

CMIDIMsg &CMIDIMsg::operator = (const CMIDIMsg &arg) {
  m_type = arg.m_type;
  m_ch = arg.m_ch;
  m_length = arg.m_length;
  delete [] m_data;
  if(m_length) {
    m_data = new BYTE [m_length];
    memcpy(m_data, arg.m_data, (size_t)arg.m_length);
  } else 
    m_data = NULL;
  return (*this);
}

CMIDIMsg::CMIDIMsg(const CMIDIMsg &arg) {
  m_type = arg.m_type;
  m_ch = arg.m_ch;
  m_length = arg.m_length;
  if(m_length) {
    m_data = new BYTE [m_length];
    memcpy(m_data, arg.m_data, (size_t)arg.m_length);
  } else {
    m_data = NULL;
  }
}

CMIDIMsg::~CMIDIMsg() { 
  delete [] m_data; 
}

const char *CMIDIMsg::c_str() const {
  const char *text[] = {
  // CANNEL
  "NOTE_OFF", "NOTE_ON", "POLYPHONIC_KEY_PRESSURE", "CONTROL_CHANGE", 
  "PROGRAM_CHANGE", "CHANNEL_PRESSURE", "PITCH_BEND_CHANGE",
  // MODE
  "ALL_SOUND_OFF", "RESET_ALL_CONTROLLERS", "LOCAL_CONTROL", "ALL_NOTES_OFF",
  "OMNI_OFF", "OMNI_ON", "POLYPHONIC_OPERATION", "MONOPHONIC_OPERATION",
  // SYSTEM
  "SYSTEM_EXCLUSIVE", "MTC_QUARTER_FRAME", "SONG_POSITION_POINTER", "SONG_SELECT", "TUNE_REQUEST",
  // REALTIME
  "REALTIME_CLOCK", "REALTIME_TICK", "REALTIME_START", "REALTIME_CONTINUE", "REALTIME_STOP", 
  "REALTIME_ACTIVE_SENSE", "REALTIME_SYSTEM_RESET", "UNKNOWN_MESSAGE"
  };
  return text[m_type];
}

CMIDIMsgInterpreter::CMIDIMsgInterpreter() {
  m_state = STATE_WAIT_STATUS;
}

CMIDIMsgInterpreter::~CMIDIMsgInterpreter() {
  m_queue.clear();
}

void CMIDIMsgInterpreter::Reset() {
  m_state = STATE_WAIT_STATUS;
  m_data.clear();
  m_queue.clear();
}

void CMIDIMsgInterpreter::EnqueueMsg() {

  CMIDIMsg::MsgType type;

  switch(m_status>>4) {
    case 0x8:
      type = CMIDIMsg::NOTE_OFF;
      break;
    case 0x9:
      type = CMIDIMsg::NOTE_ON;
      break;
    case 0xA:
      type = CMIDIMsg::POLYPHONIC_KEY_PRESSURE;
      break;
    case 0xB:
      if(m_data[0]==0x78) type = CMIDIMsg::ALL_SOUND_OFF;
      else if(m_data[0]==0x79) type = CMIDIMsg::RESET_ALL_CONTROLLERS;
      else if(m_data[0]==0x7A) type = CMIDIMsg::LOCAL_CONTROL;
      else if(m_data[0]==0x7B) type = CMIDIMsg::ALL_NOTES_OFF;
      else if(m_data[0]==0x7C) type = CMIDIMsg::OMNI_OFF;
      else if(m_data[0]==0x7D) type = CMIDIMsg::OMNI_ON;
      else if(m_data[0]==0x7E) type = CMIDIMsg::MONOPHONIC_OPERATION;
      else if(m_data[0]==0x7F) type = CMIDIMsg::POLYPHONIC_OPERATION;
      else type = CMIDIMsg::CONTROL_CHANGE;
      break;
    case 0xC:
      type = CMIDIMsg::PROGRAM_CHANGE;
      break;
    case 0xD:
      type = CMIDIMsg::CHANNEL_PRESSURE;
      break;
    case 0xE:
      type = CMIDIMsg::PITCH_BEND_CHANGE;
      break;
    case 0xF:
      if(m_status==0xF0) type = CMIDIMsg::SYSTEM_EXCLUSIVE;
      else if(m_status==0xF1) type = CMIDIMsg::MTC_QUARTER_FRAME;
      else if(m_status==0xF2) type = CMIDIMsg::SONG_POSITION_POINTER;
      else if(m_status==0xF3) type = CMIDIMsg::SONG_SELECT;
      else if(m_status==0xF4) type = CMIDIMsg::UNKNOWN_MESSAGE;
      else if(m_status==0xF5) type = CMIDIMsg::UNKNOWN_MESSAGE;
      else if(m_status==0xF6) type = CMIDIMsg::TUNE_REQUEST;
      else type = CMIDIMsg::UNKNOWN_MESSAGE;
      break;
    default:
      type = CMIDIMsg::UNKNOWN_MESSAGE;
      break;
  }

  m_queue.push_back( CMIDIMsg(type,m_status&15,m_data.data(),(DWORD)m_data.size()) );
  m_data.clear();
}

bool CMIDIMsgInterpreter::TransStatus(BYTE data) {

  m_status = data;

  if(!(data&0x80)) {
    m_state = STATE_WAIT_STATUS;
    return false;
  }

  switch(data>>4) {
  case 0x8: case 0x9: case 0xA: case 0xB: case 0xE:
    m_state = STATE_WAIT_DATA2;
    break;
  case 0xC: case 0xD:
    m_state = STATE_WAIT_DATA1;
    break;
  default:
    if(data==0xF0) m_state = STATE_WAIT_EOX; // SYSTEM EXCLUSIVE
    else if(data==0xF1) m_state = STATE_WAIT_DATA1; // MTC QUARTER FRAME
    else if(data==0xF2) m_state = STATE_WAIT_DATA2; // SONG POSITION POINTER
    else if(data==0xF3) m_state = STATE_WAIT_DATA1; // SONG SELECT
    else if(data==0xF4) m_state = STATE_FINISHED; // UNKNOWN
    else if(data==0xF5) m_state = STATE_FINISHED; // UNKNOWN
    else if(data==0xF6) m_state = STATE_FINISHED; // TUNE REQUEST
    else {
      m_state = STATE_WAIT_STATUS;
      return false;
    }
    break;
  }

  return true;
}

bool CMIDIMsgInterpreter::Interpret(BYTE data) {

  // Real-Time Msgsreturn CMIDIMsg::REALTIME_STOP;
  if (0xF8<=data) {
    if (data==0xF8) m_queue.push_back( CMIDIMsg(CMIDIMsg::REALTIME_CLOCK) );
    else if(data==0xF9) m_queue.push_back( CMIDIMsg(CMIDIMsg::REALTIME_TICK) );
    else if(data==0xFA) m_queue.push_back( CMIDIMsg(CMIDIMsg::REALTIME_START) );
    else if(data==0xFB) m_queue.push_back( CMIDIMsg(CMIDIMsg::REALTIME_CONTINUE) );
    else if(data==0xFC) m_queue.push_back( CMIDIMsg(CMIDIMsg::REALTIME_STOP) );
    else if(data==0xFD) m_queue.push_back( CMIDIMsg(CMIDIMsg::UNKNOWN_MESSAGE) );
    else if(data==0xFC) m_queue.push_back( CMIDIMsg(CMIDIMsg::REALTIME_ACTIVE_SENSE) );
    else m_queue.push_back ( CMIDIMsg(CMIDIMsg::REALTIME_ACTIVE_SENSE) ) ;
    return true;
  } 
  // Running Status
  if(m_state==STATE_WAIT_STATUS&&!(data&0x80)) {
    if(m_status&0x80) {
      if(!TransStatus(m_status)) return false;
    } else
      return false;
  }

  // Channel and System Messages
  switch(m_state) {
  case STATE_WAIT_STATUS:
    return TransStatus(data);

  case STATE_WAIT_DATA2:
    if(data&0x80) { 
      m_data.push_back(0);
      m_data.push_back(0);
      EnqueueMsg();
      return TransStatus(data);
    } else {
      m_data.push_back(data);
      m_state = STATE_WAIT_DATA1;
    }
    break;

  case STATE_WAIT_DATA1:
    if(data&0x80) {
      m_data.push_back(0);
      EnqueueMsg();
      return TransStatus(data);
    } else {
      m_data.push_back(data);
      EnqueueMsg();
      m_state = STATE_WAIT_STATUS;
    }
    break;

  case STATE_WAIT_EOX:
    if(data&0x80) {
      EnqueueMsg();
      if(data!=0xF7) 
        return TransStatus(data);
      else
        m_state = STATE_WAIT_STATUS;
    } else 
      m_data.push_back(data);
    break;

  default:
    m_state = STATE_WAIT_STATUS;
    return false;
  }

  return true;
}
