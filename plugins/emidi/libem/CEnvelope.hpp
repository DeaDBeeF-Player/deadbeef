#ifndef __CENVELOPE_HPP__
#define __CENVELOPE_HPP__
#include "DsaCommon.hpp"

namespace dsa {

class CEnvelope {
public:
  enum EnvState { SETTLE, ATTACK, DECAY, SUSTINE, RELEASE, FINISH };
  struct Param { 
    UINT32 ar, dr, sl, sr, rr; 
  };
  struct ChannelInfo {
    EnvState state;
    UINT32 speed; 
    UINT32 value;
    Param  param;
  };
private:
  UINT m_ch;
  ChannelInfo *m_ci;
  UINT32 m_clock;
  UINT32 m_rate;
  UINT32 m_cnt;
  UINT32 m_inc;
  UINT32 _CalcSpeed(UINT32 ms);
public:
  CEnvelope(UINT ch);
  ~CEnvelope();
  void Reset(UINT32 clock=44100, UINT32 rate=60);
  void KeyOn(UINT ch);
  void KeyOff(UINT ch);
  bool Update();
  void SetParam(UINT ch, const Param &param);
  UINT32 GetValue(UINT ch) const;
};

} //namespace dsa
#endif