#ifndef __MIDI_MESSAGE_HPP__
#define __MIDI_MESSAGE_HPP__
#include <string>
#include <deque>

#include "DsaCommon.hpp"

namespace dsa {

// MIDI Msgs (Channel or exclusive Msg)
class CMIDIMsg {
public:
  enum MsgType {
    // CHANNEL Msg
    NOTE_OFF=0,              // 8n #note #velo
    NOTE_ON,                 // 9n #note #velo
    POLYPHONIC_KEY_PRESSURE, // An #note #data
    CONTROL_CHANGE,          // Bn #ctrl #data
    PROGRAM_CHANGE,          // Cn #data
    CHANNEL_PRESSURE,        // Dn #data
    PITCH_BEND_CHANGE,       // En #data #data
    // MODE Msg
    ALL_SOUND_OFF,         // Bn 78 00
    RESET_ALL_CONTROLLERS, // Bn 79 00
    LOCAL_CONTROL,         // Bn 7A #data
    ALL_NOTES_OFF,         // Bn 7B 00
    OMNI_OFF,              // Bn 7C 00
    OMNI_ON,               // Bn 7D 00
    POLYPHONIC_OPERATION,  // Bn 7E 00
    MONOPHONIC_OPERATION,  // Bn 7F 00
    // SYSTEM Msg
    SYSTEM_EXCLUSIVE,     // F0 ... F7
    MTC_QUARTER_FRAME,    // F1 #data
    SONG_POSITION_POINTER,// F2 #data #data
    SONG_SELECT,          // F3 #data
    TUNE_REQUEST,         // F6
    // REALTIME Msg
    REALTIME_CLOCK,           // F8
    REALTIME_TICK,            // F9
    REALTIME_START,           // FA
    REALTIME_CONTINUE,        // FB
    REALTIME_STOP,            // FC
    REALTIME_ACTIVE_SENSE,    // FE
    REALTIME_SYSTEM_RESET,    // FF
    UNKNOWN_MESSAGE
  };

  MsgType m_type;     // The Msg identifier
  UINT m_ch;               // The channel
  BYTE *m_data;           // The data sequence
  DWORD m_length;         // The length of the data sequence 
  CMIDIMsg(MsgType type=UNKNOWN_MESSAGE,int ch=0, const BYTE *data=NULL, DWORD length=0);
  CMIDIMsg(const CMIDIMsg &);
  ~CMIDIMsg();
  CMIDIMsg &operator = (const CMIDIMsg &arg);
  const char *c_str() const;
};

class CMIDIMsgInterpreter {
private:
  BYTE m_status;
  std::basic_string<BYTE> m_data;
  std::deque<CMIDIMsg> m_queue;
  enum State { STATE_WAIT_STATUS, STATE_WAIT_DATA2, STATE_WAIT_DATA1, STATE_WAIT_EOX, STATE_FINISHED, STATE_ERROR };  
  State m_state;
  void EnqueueMsg();
  bool TransStatus(BYTE data);
public:
  CMIDIMsgInterpreter();
  ~CMIDIMsgInterpreter();
  void Reset();
  bool Interpret(BYTE data); // False if error
  const CMIDIMsg &GetMsg(){ return m_queue.front(); }
  void PopMsg(){ m_queue.pop_front(); }
  size_t GetMsgCount() const { return m_queue.size(); }
};

} // namespace dsa

#endif