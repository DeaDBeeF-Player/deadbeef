#ifndef __CSMF_HPP__
#define __CSMF_HPP__

#include "CMIDIMessage.hpp"

namespace dsa {

// SMF Events (include MIDI, SYSEX and META events)
class CSMFEvent {
public:
  DWORD m_delta;     // The delta Time
  enum EventType { MIDI_EVENT, SYSEX_EVENT, META_EVENT, UNKNOWN_EVENT };
  EventType m_type;  // The event identifier
  bool m_running;    // True if this event is decided by running status.
  WORD m_status;     // The status byte. (META_EVENT is a word 0xFF??. Otherwise 1 byte.)
  BYTE *m_data;      // The pointer to the data sequence.
  DWORD m_length;    // The length of the data sequence.
};

// SMF itself
class CSMF {
public:
  DWORD m_MThd_length;
  WORD  m_format;
  WORD  m_track_num;
  WORD  m_time_base;
  DWORD *m_MTrk_length;
  char **m_MTrk_data;
  BYTE  *m_MTrk_running_status;
  DWORD *m_MTrk_ptr;

public:
  CSMF();
  ~CSMF();
  bool Load(const char *filename);
  bool Load(const char *data, DWORD data_length);
  void Seek(int trk, DWORD pos){ m_MTrk_ptr[trk]=pos; }
  DWORD ReadNextVnum(int trk);
  BYTE ReadNextByte(int trk);
  CSMFEvent ReadNextEvent(int trk);
};

class CSMF_Exception {
public:
  enum ExceptionID { SMF_END_TRACK=0, SMF_INVALID_TRACK, SMF_UNKNOWN_ERROR };
  CSMF_Exception(ExceptionID id) : m_id(id) {}
  ExceptionID m_id;
  const char *c_str () const { 
    static const char *Msg[] = {"Track buffer is end.", "Invalid Track number.", "Unknown Error"};
    return Msg[m_id]; 
  }
};

} // namespace dsa

#endif //__SMF_H__
