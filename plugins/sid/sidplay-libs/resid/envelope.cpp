//  ---------------------------------------------------------------------------
//  This file is part of reSID, a MOS6581 SID emulator engine.
//  Copyright (C) 2002  Dag Lem <resid@nimrod.no>
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//  ---------------------------------------------------------------------------

#define __ENVELOPE_CC__
#include "envelope.h"

RESID_NAMESPACE_START

// ----------------------------------------------------------------------------
// Constructor.
// ----------------------------------------------------------------------------
EnvelopeGenerator::EnvelopeGenerator()
{
  reset();
}

// ----------------------------------------------------------------------------
// SID reset.
// ----------------------------------------------------------------------------
void EnvelopeGenerator::reset()
{
  envelope_counter = 0;

  attack = 0;
  decay = 0;
  sustain = 0;
  release = 0;

  gate = 0;

  rate_counter = 0;
  exponential_counter = 0;

  state = RELEASE;
  rate_period = rate_counter_period[release];
  hold_zero = true;
}


// Rate counter periods are calculated from the Envelope Rates table in
// the Programmer's Reference Guide. The rate counter period is the number of
// cycles between each increment of the envelope counter.
// The rates have been verified by sampling ENV3. 
//
// The rate counter is a 16 bit register which is incremented each cycle.
// When the counter reaches a specific comparison value, the envelope counter
// is incremented (attack) or decremented (decay/release) and the
// counter is zeroed.
//
// NB! Sampling ENV3 shows that the calculated values are not exact.
// It may seem like most calculated values have been rounded (.5 is rounded
// down) and 1 has beed added to the result. A possible explanation for this
// is that the SID designers have used the calculated values directly
// as rate counter comparison values, not considering a one cycle delay to
// zero the counter. This would yield an actual period of comparison value + 1.
//
// The exact rate counter periods can be determined e.g. by counting the number
// of cycles from envelope level 1 to envelope level 255, and dividing the
// number of cycles by 254. CIA1 timer A and B in linked mode can perform
// the cycle count. This is the method used to find the rates below.
// Making a full sample from 8 cycle shifted samples is also possible, however
// it is then necessary to wait exactly the same cycle tuned interval between
// each sample. This is because it is not possible to reset the rate counter
// (the test bit has no influence on the envelope generator whatsoever).
// The time of the first count of the envelope counter can not be exactly
// controlled except possibly by resetting the chip.
//
// To avoid the ADSR delay bug, sampling of ENV3 should be done using
// sustain = release = 0. This ensures that the attack state will not lower
// the current rate counter period. The maximum error from the SID chip is now
// 9 cycles. The code below adds a maximum error of 14 cycles:
//
//     lda #$01
// l1: cmp $d41c
//     bne l1
//     ...
//     lda #$ff
// l2: cmp $d41c
//     bne l2
//
// The maximum timing error is thus 23 cycles, which yields a maximum error
// for the calculated rate period of 23/254 cycles. The described method is
// thus sufficient for exact calculation of rate periods.
//
reg16 EnvelopeGenerator::rate_counter_period[] = {
      9,  //   2ms*1.0MHz/256 =     7.81
     32,  //   8ms*1.0MHz/256 =    31.25
     63,  //  16ms*1.0MHz/256 =    62.50
     95,  //  24ms*1.0MHz/256 =    93.75
    149,  //  38ms*1.0MHz/256 =   148.44
    220,  //  56ms*1.0MHz/256 =   218.75
    267,  //  68ms*1.0MHz/256 =   265.63
    313,  //  80ms*1.0MHz/256 =   312.50
    392,  // 100ms*1.0MHz/256 =   390.63
    977,  // 250ms*1.0MHz/256 =   976.56
   1954,  // 500ms*1.0MHz/256 =  1953.13
   3126,  // 800ms*1.0MHz/256 =  3125.00
   3907,  //   1 s*1.0MHz/256 =  3906.25
  11720,  //   3 s*1.0MHz/256 = 11718.75
  19532,  //   5 s*1.0MHz/256 = 19531.25
  31251   //   8 s*1.0MHz/256 = 31250.00
};


// For decay and release, the clock to the envelope counter is sequentially
// divided by 1, 2, 4, 8, 16, 30 to create a piece-wise linear approximation
// of an exponential at the envelope counter values 93, 54, 26, 14, 6.
// As a special case the period at zero level is 1; this only influences the
// ADSR boundary bug.
// All values have been verified by sampling ENV3.
//
// One extra cycle is spent at envelope level 0x5d in decay and release.
// This is a delay caused by the comparison with the exponential counter,
// and does not affect the rate counter. This has been verified by timing
// 256 consecutive complete envelopes with A = D = R = 1, S = 0, using CIA1
// timer A and B in linked mode. If the rate counter is not affected the
// period of each complete envelope is
// (255 + 162 + 39*2 + 28*4 + 12*8 + 8*16 + 6*30)*32 = 756*32 = 32352
// which corresponds exactly to the timed value divided by the number of
// complete envelopes.
// NB! This one cycle delay is not modeled.
//
// Lookup table to directly, from the envelope counter, find the current
// exponential counter period.
//
reg8 EnvelopeGenerator::exponential_counter_period[] = {
  /* 0x00: */   1, 30, 30, 30, 30, 30, 30, 16,  // 0x06
  /* 0x08: */  16, 16, 16, 16, 16, 16, 16,  8,  // 0x0e
  /* 0x10: */   8,  8,  8,  8,  8,  8,  8,  8,
  /* 0x18: */   8,  8,  8,  4,  4,  4,  4,  4,  // 0x1a
  /* 0x20: */   4,  4,  4,  4,  4,  4,  4,  4,
  /* 0x28: */   4,  4,  4,  4,  4,  4,  4,  4,
  /* 0x30: */   4,  4,  4,  4,  4,  4,  4,  2,  // 0x36
  /* 0x38: */   2,  2,  2,  2,  2,  2,  2,  2,
  /* 0x40: */   2,  2,  2,  2,  2,  2,  2,  2,
  /* 0x48: */   2,  2,  2,  2,  2,  2,  2,  2,
  /* 0x50: */   2,  2,  2,  2,  2,  2,  2,  2,
  /* 0x58: */   2,  2,  2,  2,  2,  2,  1,  1,  // 0x5d
  /* 0x60: */   1,  1,  1,  1,  1,  1,  1,  1,
  /* 0x68: */   1,  1,  1,  1,  1,  1,  1,  1,
  /* 0x70: */   1,  1,  1,  1,  1,  1,  1,  1,
  /* 0x78: */   1,  1,  1,  1,  1,  1,  1,  1,
  /* 0x80: */   1,  1,  1,  1,  1,  1,  1,  1,
  /* 0x88: */   1,  1,  1,  1,  1,  1,  1,  1,
  /* 0x90: */   1,  1,  1,  1,  1,  1,  1,  1,
  /* 0x98: */   1,  1,  1,  1,  1,  1,  1,  1,
  /* 0xa0: */   1,  1,  1,  1,  1,  1,  1,  1,
  /* 0xa8: */   1,  1,  1,  1,  1,  1,  1,  1,
  /* 0xb0: */   1,  1,  1,  1,  1,  1,  1,  1,
  /* 0xb8: */   1,  1,  1,  1,  1,  1,  1,  1,
  /* 0xc0: */   1,  1,  1,  1,  1,  1,  1,  1,
  /* 0xc8: */   1,  1,  1,  1,  1,  1,  1,  1,
  /* 0xd0: */   1,  1,  1,  1,  1,  1,  1,  1,
  /* 0xd8: */   1,  1,  1,  1,  1,  1,  1,  1,
  /* 0xe0: */   1,  1,  1,  1,  1,  1,  1,  1,
  /* 0xe8: */   1,  1,  1,  1,  1,  1,  1,  1,
  /* 0xf0: */   1,  1,  1,  1,  1,  1,  1,  1,
  /* 0xf8: */   1,  1,  1,  1,  1,  1,  1,  1
};


// From the sustain levels it follows that both the low and high 4 bits of the
// envelope counter are compared to the 4-bit sustain value.
// This has been verified by sampling ENV3.
//
reg8 EnvelopeGenerator::sustain_level[] = {
  0x00,
  0x11,
  0x22,
  0x33,
  0x44,
  0x55,
  0x66,
  0x77,
  0x88,
  0x99,
  0xaa,
  0xbb,
  0xcc,
  0xdd,
  0xee,
  0xff,
};


// ----------------------------------------------------------------------------
// Register functions.
// ----------------------------------------------------------------------------
void EnvelopeGenerator::writeCONTROL_REG(reg8 control)
{
  reg8 gate_next = control & 0x01;

  // The rate counter is never reset, thus there will be a delay before the
  // envelope counter starts counting up (attack) or down (release).

  // Gate bit on: Start attack, decay, sustain.
  if (!gate && gate_next) {
    state = ATTACK;
    rate_period = rate_counter_period[attack];

    // Switching to attack state unlocks the zero freeze.
    hold_zero = false;
  }
  // Gate bit off: Start release.
  else if (gate && !gate_next) {
    state = RELEASE;
    rate_period = rate_counter_period[release];
  }

  gate = gate_next;
}

void EnvelopeGenerator::writeATTACK_DECAY(reg8 attack_decay)
{
  attack = (attack_decay >> 4) & 0x0f;
  decay = attack_decay & 0x0f;
  if (state == ATTACK) {
    rate_period = rate_counter_period[attack];
  }
  else if (state == DECAY_SUSTAIN) {
    rate_period = rate_counter_period[decay];
  }
}

void EnvelopeGenerator::writeSUSTAIN_RELEASE(reg8 sustain_release)
{
  sustain = (sustain_release >> 4) & 0x0f;
  release = sustain_release & 0x0f;
  if (state == RELEASE) {
    rate_period = rate_counter_period[release];
  }
}

reg8 EnvelopeGenerator::readENV()
{
  return output();
}

RESID_NAMESPACE_STOP
