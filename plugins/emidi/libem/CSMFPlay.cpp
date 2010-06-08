#include <cstdio>
#include "CSMFPlay.hpp"
#include "COpllDevice.hpp"
#include "CSccDevice.hpp"

#if defined (_MSC_VER)
#if defined (_DEBUG)
#define new new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#endif

using namespace dsa;

CSMFPlay::CSMFPlay(DWORD rate, int mods) {
  m_mods = mods;
  for(int i=0;i<m_mods;i++){
    if(i&1)
      m_module[i].AttachDevice(new CSccDevice(rate,2));
    else
      m_module[i].AttachDevice(new COpllDevice(rate,2));
  }
  m_sample_in_us = 1.0E06/rate;
}

CSMFPlay::~CSMFPlay() {
  for(int i=0;i<m_mods;i++) delete m_module[i].DetachDevice();
}

bool CSMFPlay::Open(const char *filename) {

  FILE *fp;
  DWORD length;
  char *data;

  if((fp=fopen(filename,"rb"))==NULL) {
    // printf("Can't open %s\n",filename);
    return false;
  };

  fseek(fp,0L,SEEK_END);
  length = ftell(fp);
  fseek(fp,0L,SEEK_SET);
  data = new char [length];
  if(fread(data,sizeof(char),length,fp)!=length) {
    // printf("Fatal Error.\n");
    return false;
  };
  fclose(fp);

  if(!m_smf.Load(data,length)) { 
    // printf("Invalid SMF.\n"); 
    delete [] data;
    return false; 
  }

  /*
  printf("[MThd]\n");
  printf("MThd Length: %d\n", m_smf.m_MThd_length);
  printf("Format     : %2d\n", m_smf.m_format);
  printf("TrackNum   : %2d\n",   m_smf.m_track_num);
  printf("TimeBase   : %2d\n\n", m_smf.m_time_base); 
  for(int trk=0; trk<m_smf.m_track_num; trk++) {
    printf("MTrk[%03d]: %d bytes at %p\n",trk,m_smf.m_MTrk_length[trk],m_smf.m_MTrk_data[trk]);
  }
  */

  delete [] data;
  return true;
}

void CSMFPlay::Start() {

  for(int i=0;i<m_mods;i++) m_module[i].Reset();

  m_tempo = (60*1000000)/120;
  m_time_rest = 0.0;

  m_delta.clear();
  m_end_flag.clear();
  m_event.clear();

  m_playing_tracks = m_smf.m_track_num;

  for(int trk=0; trk<m_smf.m_track_num; trk++) {
    m_smf.Seek(trk,0);
    m_delta.push_back(0);
    m_end_flag.push_back(false);
    try {
      m_event.push_back(m_smf.ReadNextEvent(trk));
    } catch (CSMF_Exception) {
      m_end_flag[trk] = true;
    }
  }
}

bool CSMFPlay::Render(short *buf, DWORD length) {
  
  CMIDIMsg msg;
  CMIDIMsgInterpreter mi;
  DWORD idx = 0;
  int trk;

  if( m_playing_tracks <= 0 ) return false;

  while( idx < length ) {

    for(trk=0; trk<m_smf.m_track_num; trk ++) {

      while ( !m_end_flag[trk] && m_delta[trk] == 0 ) { 

        if(m_event[trk].m_type == CSMFEvent::MIDI_EVENT) {
          mi.Interpret((BYTE)m_event[trk].m_status);
          for(DWORD i=0;i<m_event[trk].m_length;i++) mi.Interpret(m_event[trk].m_data[i]);
          while(mi.GetMsgCount()) {
            const CMIDIMsg &msg = mi.GetMsg();
            m_module[(msg.m_ch*2)%m_mods].SendMIDIMsg(msg);
            if(msg.m_ch!=9)
              m_module[(msg.m_ch*2+1)%m_mods].SendMIDIMsg(msg);
            mi.PopMsg();
          }
        } else if(m_event[trk].m_type == CSMFEvent::META_EVENT) {
          if(m_event[trk].m_status == 0xFF2F) { // ENDOFTRACK
            m_end_flag[trk] = true;
            m_playing_tracks--;
            /*
            printf("<META>EndOfTrack</META>\n");
            */
            break;
          } else if(m_event[trk].m_status == 0xFF01) {
            /*
            printf("<META><TEXT>\n");
            for(DWORD i=0;i<m_event[trk].m_length;i++) putchar(m_event[trk].m_data[i]);
            printf("\n</TEXT></META>\n");
            */
          } else if(m_event[trk].m_status == 0xFF51) { // TEMPO
            m_tempo = (m_event[trk].m_data[0]<<16)|(m_event[trk].m_data[1]<<8)|(m_event[trk].m_data[2]);
            //printf("<META>Tempo %06x</META>\n",m_tempo);
          } else {
            //printf("META: %04x\n",m_event[trk].m_status);
          }
        }
        
        try {
          m_event[trk] = m_smf.ReadNextEvent(trk);
          m_delta[trk] = m_event[trk].m_delta;
        } catch (CSMF_Exception e) {
          printf("*%s\n",e.c_str());
          m_end_flag[trk] = true;
          m_playing_tracks--;
        }
      }
    } // endwhile;

    bool done = false;
    double tick_time = (double)m_tempo/m_smf.m_time_base; 

    while(!done) {
      m_time_rest += tick_time;
      while ( m_sample_in_us < m_time_rest) {
        m_time_rest -= m_sample_in_us;

        if(buf) {
          buf[idx*2] = buf[idx*2+1] = 0;
          for(int i=0; i<m_mods; i++) {
            INT32 b[2];
            m_module[i].Render(b);
            buf[idx*2] += (short)(b[0]>>16); 
            buf[idx*2+1] += (short)(b[1]>>16);
          }
        }

        idx++;
        if( length <= idx ) {
          done = true;
          break;
        }
      }
      if(!done) {
        for(trk=0; trk<m_smf.m_track_num; trk++) {
          if((0<m_delta[trk])&&(--m_delta[trk]==0)) done = true;
        }
      }
    }

  } // end while (idx < length )

  return true;
}

