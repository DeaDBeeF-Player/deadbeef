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

#ifndef __ENVELOPE_H__
#define __ENVELOPE_H__

#include "siddefs.h"

RESID_NAMESPACE_START

// ----------------------------------------------------------------------------
// A 16 bit counter is used to implement the envelope rates, in effect
// dividing the clock to the envelope counter by the currently selected rate
// period.
// In addition, another counter is used to implement the exponential envelope
// decay, in effect further dividing the clock to the envelope counter.
// The period of this counter is set to 1 in the attack state, and is
// successively set to 1, 2, 4, 8, 16, 30 at the envelope counter values
// 255, 93, 54, 26, 14, 6 in the decay and release states.
// ----------------------------------------------------------------------------
class EnvelopeGenerator
{
public:
  EnvelopeGenerator();

  RESID_INLINE void clock();
  RESID_INLINE void clock(cycle_count delta_t);
  void reset();

  void writeCONTROL_REG(reg8);
  void writeATTACK_DECAY(reg8);
  void writeSUSTAIN_RELEASE(reg8);
  reg8 readENV();

  // 8-bit envelope output.
  RESID_INLINE reg8 output();

protected:
  reg16 rate_counter;
  reg16 rate_period;
  reg16 exponential_counter;
  reg8 envelope_counter;
  bool hold_zero;

  reg4 attack;
  reg4 decay;
  reg4 sustain;
  reg4 release;

  reg8 gate;

  enum { ATTACK, DECAY_SUSTAIN, RELEASE } state;

  // Lookup table to convert from attack, decay, or release value to rate
  // counter period.
  static reg16 rate_counter_period[];

  // Lookup table to directly, from the envelope counter, find the current
  // exponential counter period.
  static reg8 exponential_counter_period[];

  // The 16 selectable sustain levels.
  static reg8 sustain_level[];

friend class SID;
};


// ----------------------------------------------------------------------------
// Inline functions.
// The following functions are defined inline because they are called every
// time a sample is calculated.
// ----------------------------------------------------------------------------

#if RESID_INLINING || defined(__ENVELOPE_CC__)

// ----------------------------------------------------------------------------
// SID clocking - 1 cycle.
// ----------------------------------------------------------------------------
RESID_INLINE
void EnvelopeGenerator::clock()
{
  // Check for ADSR delay bug.
  // If the rate counter comparison value is set below the current value of the
  // rate counter, the counter will continue counting up to 2^15 = 0x8000,
  // and then count rate_period twice before the envelope can finally be
  // stepped. In this process one extra rate_counter step is taken.
  // This has been verified by sampling ENV3.
  // A possible explanation for this behavior is that the 16 bit rate counter
  // is compared with a 15 bit comparator for reset and with a 16 bit
  // comparator for envelope steps.
  //
  if ((++rate_counter & 0x7fff) != rate_period) {
    return;
  }

  if (rate_counter & 0x8000) {
    rate_counter = 1;
    return;
  }

  rate_counter = 0;

  // The first envelope step in the attack state also resets the exponential
  // counter. This has been verified by sampling ENV3.
  //
  if (state == ATTACK || ++exponential_counter
      == exponential_counter_period[envelope_counter])
  {
    exponential_counter = 0;

    // Check whether the envelope counter is frozen at zero.
    if (hold_zero) {
      return;
    }

    switch (state) {
    case ATTACK:
      // The envelope counter can flip from 0xff to 0x00 by changing state to
      // release, then to attack. The envelope counter is then frozen at
      // zero; to unlock this situation the state must be changed to release,
      // then to attack. This has been verified by sampling ENV3.
      //
      ++envelope_counter &= 0xff;
      if (envelope_counter == 0xff) {
	state = DECAY_SUSTAIN;
	rate_period = rate_counter_period[decay];
      }
      break;
    case DECAY_SUSTAIN:
      if (envelope_counter != sustain_level[sustain]) {
	--envelope_counter;
      }
      break;
    case RELEASE:
      // The envelope counter can flip from 0x00 to 0xff by changing state to
      // attack, then to release. The envelope counter will then continue
      // counting down in the release state.
      // This has been verified by sampling ENV3.
      // NB! The operation below requires two's complement integer.
      //
      --envelope_counter &= 0xff;
      break;
    }
    
    // When the envelope counter is changed to zero, it is frozen at zero.
    // This has been verified by sampling ENV3.
    //
    if (envelope_counter == 0) {
      hold_zero = true;
    }
  }
}


// ----------------------------------------------------------------------------
// SID clocking - delta_t cycles.
// ----------------------------------------------------------------------------
RESID_INLINE
void EnvelopeGenerator::clock(cycle_count delta_t)
{
  // Check for ADSR delay bug.
  // If the rate counter comparison value is set below the current value of the
  // rate counter, the counter will continue counting up to 2^15 = 0x8000,
  // and then count rate_period twice before the envelope can finally be
  // stepped. In this process one extra rate_counter step is taken.
  // This has been verified by sampling ENV3.
  // A possible explanation for this behavior is that the 16 bit rate counter
  // is compared with a 15 bit comparator for reset and with a 16 bit
  // comparator for envelope steps.
  //
  reg16 rate_counter_15 = rate_counter & 0x7fff;

  // NB! This requires two's complement integer.
  int rate_step = rate_period - rate_counter_15;
  if (rate_step < 0) {
    rate_step += 0x8000;
  }

  for (; delta_t; rate_step = rate_period) {
    if (delta_t < rate_step) {
      rate_counter += delta_t;
      return;
    }

    if ((rate_counter + rate_step) & 0x8000) {
      rate_counter = 1;
      delta_t -= rate_step;
      rate_step = rate_period - 1;
      continue;
    }

    rate_counter = 0;
    delta_t -= rate_step;

    // The first envelope step in the attack state also resets the exponential
    // counter. This has been verified by sampling ENV3.
    //
    if (state == ATTACK	|| ++exponential_counter
	== exponential_counter_period[envelope_counter])
    {
      exponential_counter = 0;

      // Check whether the envelope counter is frozen at zero.
      if (hold_zero) {
	continue;
      }

      switch (state) {
      case ATTACK:
	// The envelope counter can flip from 0xff to 0x00 by changing state to
	// release, then to attack. The envelope counter is then frozen at
	// zero; to unlock this situation the state must be changed to release,
	// then to attack. This has been verified by sampling ENV3.
	//
	++envelope_counter &= 0xff;
	if (envelope_counter == 0xff) {
	  state = DECAY_SUSTAIN;
	  rate_period = rate_counter_period[decay];
	  rate_step = rate_period;
	}
	break;
      case DECAY_SUSTAIN:
	if (envelope_counter != sustain_level[sustain]) {
	  --envelope_counter;
	}
	break;
      case RELEASE:
	// The envelope counter can flip from 0x00 to 0xff by changing state to
	// attack, then to release. The envelope counter will then continue
	// counting down in the release state.
	// This has been verified by sampling ENV3.
	// NB! The operation below requires two's complement integer.
	//
	--envelope_counter &= 0xff;
	break;
      }

      // When the envelope counter is changed to zero, it is frozen at zero.
      // This has been verified by sampling ENV3.
      //
      if (envelope_counter == 0) {
	hold_zero = true;
      }
    }
  }
}


// ----------------------------------------------------------------------------
// Read the envelope generator output.
// ----------------------------------------------------------------------------
RESID_INLINE
reg8 EnvelopeGenerator::output()
{
  return envelope_counter;
}

#endif // RESID_INLINING || defined(__ENVELOPE_CC__)

RESID_NAMESPACE_STOP

#endif // not __ENVELOPE_H__
