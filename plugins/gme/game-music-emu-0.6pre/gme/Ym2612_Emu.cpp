#include "Ym2612_Emu.h"

#define YM2612_EMU_CPP
#ifdef USE_GENS
#include "Ym2612_Emu_Gens.cpp"
#else
#include "Ym2612_Emu_MAME.cpp"
#endif
