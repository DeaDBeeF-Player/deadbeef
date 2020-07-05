#ifndef scriptable_encoder_h
#define scriptable_encoder_h

#include "scriptable.h"
#include "../plugins/converter/converter.h"

scriptableItem_t *
scriptableEncoderRoot (void);

void
scriptableEncoderLoadPresets (void);

void
scriptableEncoderPresetToConverterEncoderPreset (scriptableItem_t *item, ddb_encoder_preset_t *encoder_preset);

#endif /* scriptable_encoder_h */
