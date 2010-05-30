#ifndef __CSCC_DEVICE_HPP__
#include <deque>
namespace dsa { namespace C {
#include "device/emu2212.h"
}};
#include "DsaCommon.hpp"
#include "ISoundDevice.hpp"

namespace dsa {

class CSccDevice : public ISoundDevice {
public:
  struct Instrument {
    UINT8 wav;
    INT8 oct;
    UINT8 ar, dr, sl, sr, rr;
  };
  enum EnvState { SETTLE, ATTACK, DECAY, SUSTINE, RELEASE, FINISH };
  struct ChannelInfo {
    EnvState env_state;
    UINT32 env_speed;
    UINT32 env_value;
    UINT8 program;
    UINT8 volume;
    UINT8 velocity;
    UINT16 freq;
    UINT8 note;
    INT8  bend_coarse;
    INT8  bend_fine;
    double _bend_fine;
    UINT8 pan;
    bool keyon;
  };
private:
  DWORD m_rate;
  UINT32 m_env_counter, m_env_incr;
  UINT m_nch;
  C::SCC *m_scc[2];
  BYTE m_reg_cache[2][0x100]; 
  UINT16 m_note2freq[128];
  ChannelInfo m_ci[5];
  std::deque<INT32> m_rbuf[2]; // The rendering buffer
  void _UpdateVolume(UINT ch);
  void _UpdateFreq(UINT ch);
  void _UpdateProgram(UINT ch);
  void _WriteReg(BYTE reg, BYTE val, INT pan=-1);
  void _CalcEnvelope(void);
public:
  CSccDevice(DWORD rate=44100, UINT nch=2);
  virtual ~CSccDevice();
  const SoundDeviceInfo &GetDeviceInfo(void) const;
  RESULT Reset(void);
  RESULT Render(INT32 buf[2]);

  void PercKeyOn(UINT8 note){};
  void PercKeyOff(UINT8 note){};
  void PercSetVolume(UINT8 vol){};
  void PercSetVelocity(UINT8 note, UINT8 velo){};
  void PercSetProgram(UINT8 note, UINT8 velo){};

  void SetProgram(UINT ch, UINT8 bank, UINT8 prog);
  void SetVelocity(UINT ch, UINT8 vel);
  void SetPan(UINT ch, UINT8 pan);
  void SetVolume(UINT ch, UINT8 vol);
  void SetBend(UINT ch, INT8 coarse, INT8 fine);
  void KeyOn(UINT ch, UINT8 note);
  void KeyOff(UINT ch);
};


} // namespace 




#endif
