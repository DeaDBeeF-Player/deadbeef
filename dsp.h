//
//  dsp.h
//  deadbeef
//
//  Created by Oleksiy Yakovenko on 09/02/2017.
//  Copyright © 2017 Alexey Yakovenko. All rights reserved.
//

#ifndef dsp_h
#define dsp_h

void
dsp_free (void);

void
dsp_reset (void);

int
streamer_dsp_chain_save (void);

void
dsp_chain_free (ddb_dsp_context_t *dsp_chain);

void
streamer_dsp_postinit (void);

void
streamer_set_dsp_chain_real (ddb_dsp_context_t *chain);

void
streamer_dsp_init (void);

void
streamer_set_dsp_chain_real (ddb_dsp_context_t *chain);

float
dsp_apply (void);

ddb_dsp_context_t *
dsp_clone (ddb_dsp_context_t *from);

#endif /* dsp_h */
