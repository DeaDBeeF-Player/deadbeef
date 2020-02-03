//
//  eqpreset.h
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 1/29/20.
//  Copyright © 2020 Alexey Yakovenko. All rights reserved.
//

#ifndef eqpreset_h
#define eqpreset_h

void
eq_preset_save (const char *fname);

int
eq_preset_load (const char *fname, float *preamp, float values[18]);

int
eq_preset_load_fb2k (const char *fname, float values[18]);

#endif /* eqpreset_h */
