#ifndef __CDeviceOpll_H__
#define __CDeviceOpll_H__
#include <deque>

#include "ISoundDevice.hpp"

namespace dsa {

namespace C {
#include "device/emu2413.h"
} // namespace 

class COpllDevice : public ISoundDevice {
public:
  struct PercInfo{
    UINT8 volume;
    UINT8 vcache[5];
    UINT8 velocity[5];
    UINT8 keymap;
    UINT8 bank;
    UINT8 prog;
  };
  struct ChannelInfo {
    UINT16 fnum;
    UINT8 bank;
    UINT8 program;
    UINT8 octave;
    UINT8 velocity;
    UINT8 volume;
    UINT8 note;
    UINT8 pan;
    INT8  bend_coarse;
    INT8  bend_fine;
    bool  keyon;
    double _bend_fine;
  };
private:
  UINT m_nch;
  C::OPLL *m_opll[2];
  BYTE m_reg_cache[2][0x80];
  ChannelInfo m_ci[9];
  PercInfo m_pi;
  std::deque<INT32> m_rbuf[2]; // The rendering buffer

  void _UpdateFreq(UINT ch);
  void _UpdateVolume(UINT ch);
  void _PercUpdateVolume(BYTE note);
  void _WriteReg(BYTE reg, BYTE val, INT pan=-1);

public:
  COpllDevice(DWORD rate=44100, UINT nch=2);
  virtual ~COpllDevice();
  
  const SoundDeviceInfo &GetDeviceInfo(void) const;
  RESULT Reset(void);
  RESULT Render(INT32 buf[2]);

  void SetProgram(UINT ch, UINT8 bank, UINT8 prog);
  void SetVelocity(UINT ch, UINT8 vel);
  void SetPan(UINT ch, UINT8 pan);
  void SetVolume(UINT ch, UINT8 vol);
  void SetBend(UINT ch, INT8 coarse, INT8 fine);
  void KeyOn(UINT ch, UINT8 note);
  void KeyOff(UINT ch);

  void PercKeyOn(UINT8 note);
  void PercKeyOff(UINT8 note);
  void PercSetProgram(UINT8 bank, UINT8 prog);
  void PercSetVelocity(UINT8 note, UINT8 vel);
  void PercSetVolume(UINT8 vol);

};

} // namespace dsa

#endif