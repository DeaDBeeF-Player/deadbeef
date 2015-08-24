#ifndef _NP_NES_DMC_H_
#define _NP_NES_DMC_H_

void* NES_DMC_np_Create(int clock, int rate);
void NES_DMC_np_Destroy(void *chip);
void NES_DMC_np_Reset(void *chip);
void NES_DMC_np_SetRate(void* chip, double rate);
void NES_DMC_np_SetPal(void* chip, bool is_pal);
void NES_DMC_np_SetAPU(void* chip, void* apu_);
UINT32 NES_DMC_np_Render(void* chip, INT32 b[2]);
void NES_DMC_np_SetMemory(void* chip, const UINT8* r);
bool NES_DMC_np_Write(void* chip, UINT32 adr, UINT32 val);
bool NES_DMC_np_Read(void* chip, UINT32 adr, UINT32* val);
void NES_DMC_np_SetClock(void* chip, double rate);
void NES_DMC_np_SetOption(void* chip, int id, int val);
int NES_DMC_np_GetDamp(void* chip);
void NES_DMC_np_SetMask(void* chip, int m);
void NES_DMC_np_SetStereoMix(void* chip, int trk, INT16 mixl, INT16 mixr);

#endif	// _NP_NES_DMC_H_
