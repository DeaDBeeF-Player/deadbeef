#ifndef __DSA_ISOUND_DEVICE_HPP__
#define __DSA_ISOUND_DEVICE_HPP__
#include "DsaCommon.hpp"
// dsa
namespace dsa {

struct SoundDeviceInfo {
  BYTE *name;
  BYTE *desc;
  UINT max_ch;  // Maximum Channels.
  UINT version; // Version no.
};

// Sound Device Interface
// 
class ISoundDevice {
public:
  virtual ~ISoundDevice(){}
  virtual const SoundDeviceInfo &GetDeviceInfo(void) const=0;
  virtual RESULT Reset(void)=0;
  virtual RESULT Render(INT32 buf[2])=0;

  virtual void SetProgram(UINT ch, UINT8 bank, UINT8 prog)=0;
  virtual void SetVelocity(UINT ch, UINT8 vel)=0;
  virtual void SetPan(UINT ch, UINT8 pan)=0;
  virtual void SetVolume(UINT ch, UINT8 vol)=0;
  // coarse: Bend depth at note (-128 to +127)
  // fine  : Bend depth at cent (-128 to +128) 100 cent equals 1 note.
  virtual void SetBend(UINT ch, INT8 coarse, INT8 fine)=0;
  virtual void KeyOn(UINT ch, UINT8 note)=0;
  virtual void KeyOff(UINT ch)=0;

  // For percussions
  virtual void PercKeyOn(UINT8 note)=0;
  virtual void PercKeyOff(UINT8 note)=0;
  virtual void PercSetProgram(UINT8 bank, UINT8 prog)=0;
  virtual void PercSetVelocity(UINT8 note, UINT8 vel)=0;
  virtual void PercSetVolume(UINT8 vol)=0;
};

} // namespace dsa

#endif // __DSA_ISOUND_DEVICE_HPP__

