#ifndef scriptable_dsp_h
#define scriptable_dsp_h

#include "scriptable.h"
#include "../deadbeef.h"

scriptableItem_t *
scriptableDspRoot (void);

void
scriptableDspLoadPresets (void);

scriptableItem_t *
scriptableDspNodeItemFromDspContext (ddb_dsp_context_t *context);

scriptableItem_t *
scriptableDspConfigFromDspChain (ddb_dsp_context_t *chain);

#endif /* scriptable_dsp_h */
