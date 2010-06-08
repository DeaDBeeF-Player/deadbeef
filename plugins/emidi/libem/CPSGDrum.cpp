#include <math.h>
#include <string.h>
#include "CPSGDrum.hpp"

using namespace dsa;
using namespace dsa::C;


static CPSGDrum::Instrument inst_table[128] = {
  // { NOTE, VOL, MODE, { AR, DR, SL, SR, RR } }
     {  48,   2,   1,  {  0,  20,  0,  0, 20 } }, // BD
     {  60,  -2,   2,  {  0,  80,  0,  0, 80 } }, // SD
};

CPSGDrum::CPSGDrum(DWORD rate, UINT nch) : m_env(6) {

  if(nch==2) m_nch = 2; else m_nch = 1;
  m_rate = rate;

  for(UINT i=0;i<2; i++)
    m_psg[i] = PSG_new(3579545,rate);

  Reset();

  for(int i=0;i<128;i++) {
    m_note2freq[i] = (WORD)(3579545.0/16/(440.0*pow(2.0,(double)(i-57)/12)));
    if(0xFFF<m_note2freq[i]) m_note2freq[i] = 0xFFF;
  }

  inst_table[35] = inst_table[36] = inst_table[0];
  inst_table[38] = inst_table[40] = inst_table[1];
}

RESULT CPSGDrum::Reset() {

    int i;
  for(i=0;i<2; i++) {
    PSG_reset(m_psg[i]);
    PSG_set_quality(m_psg[i],1);
    memset(m_reg_cache[i],0,128);
    m_rbuf[i].clear();
    m_noise_mode[i] = 0xFF;
  }

  m_env.Reset();
  m_off_channels.clear();
  m_on_channels.clear();

  for(i=0; i<6 ;i++) {
    m_ci[i].keyon = false;
    m_ci[i].note = 0;
    m_ci[i].noise = 0;
    m_off_channels.push_back(i);
  }

  for(i=0; i<128; i++) {
    m_keytable[i]=-1;
    m_velocity[i]=127;
  }

  return SUCCESS;
}

void CPSGDrum::_WriteReg(BYTE reg, BYTE val, UINT id) {
  if(m_reg_cache[id][reg]!=val) {
    PSG_writeReg(m_psg[id], reg, val);
    m_reg_cache[id][reg] = val;  
    if(m_rbuf[id].size()<8192) {
      m_rbuf[id].push_back(PSG_calc(m_psg[id])<<16);
      if(m_env.Update()) {
        for(int ch=0;ch<6;ch++) _UpdateVolume(ch);
      }
    } else {
      throw RuntimeException("Buffer Overflow",__FILE__,__LINE__);
    }
  } 
}

const SoundDeviceInfo &
CPSGDrum::GetDeviceInfo(void) const {

  static SoundDeviceInfo si;
  si.max_ch = 0;
  si.version = 0x0001;
  si.name = (BYTE *)"PSG DRUM";
  si.desc = (BYTE *)"";
  return si;
}

RESULT CPSGDrum::Render(INT32 buf[2]) {

  buf[0] = 0;
  for(UINT i=0;i<2;i++) {
    if(m_rbuf[i].empty()) {
      buf[0] += PSG_calc(m_psg[i]) << 16;
      if(m_env.Update()) {
        for(int ch=0;ch<6;ch++) _UpdateVolume(ch);
      }
    } else
      buf[0] += m_rbuf[i].front();
    m_rbuf[i].pop_front();
  }
  buf[0]<<=1;
  buf[1] = buf[0];

  return SUCCESS;
}

void CPSGDrum::_UpdateFreq(UINT ch) {
  int note = m_ci[ch].note;
  if(note<0) note = 0; else if(127<note) note =127;

  int fnum = m_note2freq[note];
  if(0xFFF < fnum) fnum = 0xFFF;

  _WriteReg((ch%3)*2, fnum&0xff,ch/3);
  _WriteReg((ch%3)*2+1, fnum>>8,ch/3);
}

void CPSGDrum::_UpdateVolume(UINT ch) {

  int vol = m_volume/16 + m_velocity[m_ci[ch].note]/16 + 1;
  vol += m_ci[ch].vol;
  vol = (vol * m_env.GetValue(ch)) >> 8;
  if(vol>15) vol=15;

  _WriteReg(8+(ch%3), vol,ch/3);
}

void CPSGDrum::_UpdateMode(UINT ch) {
  m_noise_mode[ch/3] &= ~(0x09<<(ch%3));
  m_noise_mode[ch/3] |= ((m_ci[ch].noise&2)<<(2+ch%3))|((m_ci[ch].noise&1)<<(ch%3));
  _WriteReg(7,m_noise_mode[ch/3],ch/3);
}

void CPSGDrum::PercKeyOn(UINT8 note) {

  if(note!=35&&note!=36&&note!=38&&note!=40) return;

  if(m_keytable[note]>=0) PercKeyOff(note);

  KeyInfo ki;

  if(m_off_channels.empty()) {
    ki = m_on_channels.front();
    PercKeyOff(ki.note);
    m_on_channels.pop_front();
  } else {
    ki.ch = m_off_channels.front();
    m_off_channels.pop_front();
    std::deque<KeyInfo>::iterator it;
    for(it=m_on_channels.begin();it!=m_on_channels.end();it++) {
      if((*it).ch == ki.ch) {
        m_on_channels.erase(it);
        break;
      }
    }
  }

  ki.note = note;
  m_ci[ki.ch].note = inst_table[note].note;
  m_ci[ki.ch].noise = inst_table[note].noise^3;
  m_ci[ki.ch].vol = inst_table[note].vol;
  m_env.SetParam(ki.ch,inst_table[note].param);
  m_env.KeyOn(ki.ch);
  m_ci[ki.ch].keyon = true;
  m_on_channels.push_back(ki);
  m_keytable[note] = ki.ch;
  
  _UpdateMode(ki.ch);
  _UpdateFreq(ki.ch);
  _UpdateVolume(ki.ch);
}

void CPSGDrum::PercKeyOff(UINT8 note) {

  if(m_keytable[note]<0) return;

  UINT ch = m_keytable[note];
  m_ci[ch].keyon = false;
  m_env.KeyOff(ch);
  m_off_channels.push_back(ch);
  m_keytable[note]=-1;
  _UpdateVolume(ch);

}

void CPSGDrum::PercSetProgram(UINT8 bank, UINT8 prog) {
}

void CPSGDrum::PercSetVelocity(UINT8 note, UINT8 velo) {
  m_velocity[note] = velo;
  for(int ch=0;ch<6;ch++) _UpdateVolume(ch);
}

void CPSGDrum::PercSetVolume(UINT8 vol) {
  m_volume = vol;
  for(int ch=0;ch<6;ch++) _UpdateVolume(ch);
}
