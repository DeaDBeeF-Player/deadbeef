#ifndef _TEXT_SCOPE_H_
#define _TEXT_SCOPE_H_

#include "uadeconfig.h"

#ifdef UADE_CONFIG_TEXT_SCOPE
#define TEXT_SCOPE(cycles, voice, e, value)      \
    do {                                         \
        if (use_text_scope)                      \
            text_scope(cycles, voice, e, value); \
    } while (0)
#else
#define TEXT_SCOPE(cycles, voice, e, value) do {} while (0)
#endif

enum PaulaEventType {PET_VOL, PET_PER, PET_DAT, PET_LEN, PET_LCH, PET_LCL};

void text_scope(unsigned long cycles, int voice, enum PaulaEventType e,
		int value);

#endif
