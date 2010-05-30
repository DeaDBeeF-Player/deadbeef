// Standard Midi file Object
#include <cstdio>
#include <cstring>
#include "CSMF.hpp"

#if defined (_MSC_VER)
#if defined (_DEBUG)
#define new new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#endif

using namespace dsa;

CSMF::CSMF() 
  : m_MThd_length(0), m_format(0), m_track_num(0), m_time_base(0), 
    m_MTrk_length(NULL), m_MTrk_data(NULL), m_MTrk_ptr(NULL), m_MTrk_running_status(NULL)
{
}

CSMF::~CSMF() 
{
  if(m_MTrk_data) {
    for(int i=0; i<m_track_num; i++) 
      delete [] m_MTrk_data[i];
    delete [] m_MTrk_data;
  }
  delete [] m_MTrk_length;
  delete [] m_MTrk_ptr;
  delete [] m_MTrk_running_status;
}

static DWORD ReadDwordBE(BYTE *data)
{
  return (data[0]<<24)|(data[1]<<16)|(data[2]<<8)|data[3];
}

static WORD ReadWordBE(BYTE *data)
{
  return (data[0]<<8)|data[1];
}

bool CSMF::Load(const char *data, DWORD data_length) 
{
  DWORD idx = 0;

  if(data_length<14) return false;

  // Mac binary ‘Îô
  while (1) {
    if(strncmp("MThd",data+idx,4)==0) break;
    idx++;
    if(data_length-4<=idx) return false;
  }
  data += idx; idx = 0;

  m_MThd_length = ReadDwordBE((BYTE *)data+4);
  m_format      = ReadWordBE((BYTE *)data+8);
  m_track_num   = ReadWordBE((BYTE *)data+10);
  m_time_base   = ReadWordBE((BYTE *)data+12);

  if(data_length <= 8 + m_MThd_length) return false;
  idx = 8 + m_MThd_length;

  try { 
    m_MTrk_length = new DWORD [m_track_num];
    m_MTrk_ptr    = new DWORD [m_track_num];
    m_MTrk_running_status = new BYTE [m_track_num];
    m_MTrk_data = new char * [m_track_num];
  } catch (std::bad_alloc) { 
    return false; 
  }
  memset(m_MTrk_data,0, sizeof(char *) * m_track_num);
  memset(m_MTrk_ptr,0, sizeof(DWORD) * m_track_num);
  memset(m_MTrk_running_status,0, sizeof(BYTE) * m_track_num);

  for(int i=0;i<m_track_num;i++) {
    if(strncmp("MTrk",data+idx,4)!=0) return false;
    m_MTrk_length[i] = ReadDwordBE((BYTE *)data+idx+4);
    m_MTrk_data[i] = new char [m_MTrk_length[i]];
    memcpy(m_MTrk_data[i],data+idx+8,m_MTrk_length[i]);
    idx += 8 + m_MTrk_length[i];
    if(data_length < idx) return false;
  }

  return true;
}

BYTE CSMF::ReadNextByte(int trk) {
 if(m_MTrk_length[trk]<=m_MTrk_ptr[trk]) 
   throw CSMF_Exception(CSMF_Exception::SMF_END_TRACK);
 return m_MTrk_data[trk][m_MTrk_ptr[trk]++];
}

DWORD CSMF::ReadNextVnum(int trk) {
  int ret, d;

  ret = d = ReadNextByte(trk);
  ret&=0x7F;
  while(d&0x80) {
    d = ReadNextByte(trk);
    ret = (ret<<7)|(d&0x7f);
  }
  return ret;
}

CSMFEvent CSMF::ReadNextEvent(int trk) {
  
  CSMFEvent event;

  if(m_track_num <= trk) throw CSMF_Exception(CSMF_Exception::SMF_INVALID_TRACK);

  event.m_delta  = ReadNextVnum(trk);
  event.m_status = ReadNextByte(trk);

  if (event.m_status&0x80) {
    event.m_running = false;
    if(event.m_status!=0xFF) m_MTrk_running_status[trk] = (BYTE)(event.m_status&0xFF);
  } else {
    event.m_running = true;
    event.m_status = m_MTrk_running_status[trk];
    m_MTrk_ptr[trk]--;
  }

  switch(event.m_status>>4) {
  case 0x8: 
  case 0x9: 
  case 0xA: 
  case 0xB: 
  case 0xE:
    event.m_type = CSMFEvent::MIDI_EVENT;
    event.m_length = 2;
    break;
  case 0xC: 
  case 0xD:
    event.m_type = CSMFEvent::MIDI_EVENT;
    event.m_length = 1;
    break;
  case 0xF:
    if(event.m_status == 0xFF) {
      event.m_type = CSMFEvent::META_EVENT;
      event.m_status = (0xFF00) | ReadNextByte(trk);
      event.m_length = ReadNextVnum(trk);
    } else if(event.m_status == 0xF0 || event.m_status == 0xF7) {
      event.m_type = CSMFEvent::MIDI_EVENT;
      event.m_length = ReadNextVnum(trk);
    } else {
      event.m_type = CSMFEvent::UNKNOWN_EVENT; // MAY BE REALTIME EVENT;
      event.m_delta = 0;
    }
    break;
  default:
    event.m_type = CSMFEvent::UNKNOWN_EVENT;
    event.m_length = 0;
    break;
  }
  
  event.m_data = (BYTE *)(m_MTrk_data[trk] + m_MTrk_ptr[trk]);
  m_MTrk_ptr[trk] += event.m_length;
  return event; 
}
