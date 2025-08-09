#ifndef scriptable_encoder_h
#define scriptable_encoder_h

#include "scriptable.h"
#include "../../plugins/converter/converter.h"

#ifdef __cplusplus
extern "C" {
#endif

void
scriptableEncoderInit(DB_functions_t *deadbeef);

scriptableItem_t *
scriptableEncoderRoot (scriptableItem_t *scriptableRoot);

void
scriptableEncoderLoadPresets (scriptableItem_t *scriptableRoot);

void
scriptableEncoderPresetToConverterEncoderPreset (scriptableItem_t *item, ddb_encoder_preset_t *encoder_preset);

#ifdef __cplusplus
}
#endif

#endif /* scriptable_encoder_h */
