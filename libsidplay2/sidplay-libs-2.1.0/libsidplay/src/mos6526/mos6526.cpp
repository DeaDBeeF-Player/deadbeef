/***************************************************************************
                          mos6526.cpp  -  CIA Timer
                             -------------------
    begin                : Wed Jun 7 2000
    copyright            : (C) 2000 by Simon White
    email                : s_a_white@email.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
/***************************************************************************
 *  $Log: mos6526.cpp,v $
 *  Revision 1.10  2002/12/16 22:12:24  s_a_white
 *  Simulate serial input from data port A to prevent kernel lockups.
 *
 *  Revision 1.9  2002/11/20 22:50:27  s_a_white
 *  Reload count when timers are stopped
 *
 *  Revision 1.8  2002/10/02 19:49:21  s_a_white
 *  Revert previous change as was incorrect.
 *
 *  Revision 1.7  2002/09/11 22:30:47  s_a_white
 *  Counter interval writes now go to a new register call prescaler.  This is
 *  copied to the timer latch/counter as appropriate.
 *
 *  Revision 1.6  2002/09/09 22:49:06  s_a_white
 *  Proper idr clear if interrupt was only internally pending.
 *
 *  Revision 1.5  2002/07/20 08:34:52  s_a_white
 *  Remove unnecessary and pointless conts.
 *
 *  Revision 1.4  2002/03/03 22:04:08  s_a_white
 *  Tidy.
 *
 *  Revision 1.3  2001/07/14 13:03:33  s_a_white
 *  Now uses new component classes and event generation.
 *
 *  Revision 1.2  2001/03/23 23:21:38  s_a_white
 *  Removed redundant reset funtion.  Timer b now gets initialised properly.
 *  Switch case now allows write/read from timer b.
 *
 *  Revision 1.1  2001/03/21 22:41:45  s_a_white
 *  Non faked CIA emulation with NMI support.  Removal of Hacked VIC support
 *  off CIA timer.
 *
 *  Revision 1.8  2001/03/09 23:44:30  s_a_white
 *  Integrated more 6526 features.  All timer modes and interrupts correctly
 *  supported.
 *
 *  Revision 1.7  2001/02/21 22:07:10  s_a_white
 *  Prevent re-triggering of interrupt if it's already active.
 *
 *  Revision 1.6  2001/02/13 21:00:01  s_a_white
 *  Support for real interrupts.
 *
 *  Revision 1.4  2000/12/11 18:52:12  s_a_white
 *  Conversion to AC99
 *
 ***************************************************************************/

#include "sidendian.h"
#include "mos6526.h"

enum
{
    INTERRUPT_TA      = 1 << 0,
    INTERRUPT_TB      = 1 << 1,
    INTERRUPT_ALARM   = 1 << 2,
    INTERRUPT_SP      = 1 << 3,
    INTERRUPT_FLAG    = 1 << 4,
    INTERRUPT_REQUEST = 1 << 7
};

const char *MOS6526::credit =
{   // Optional information
    "*MOS6526 (CIA) Emulation:\0"
    "\tCopyright (C) 2001 Simon White <sidplay2@email.com>\0"
};


MOS6526::MOS6526 (EventContext *context)
:idr(0),
 event_context(*context),
 event_ta(this),
 event_tb(this)
{
    reset ();
}

void MOS6526::reset (void)
{
    ta  = ta_latch = 0xffff;
    tb  = tb_latch = 0xffff;
    cra = crb = 0;
    // Clear off any IRQs
    trigger (0);
    cnt_high  = true;
    icr = idr = 0;
    m_accessClk = 0;
    dpa = 0xf0;
}

uint8_t MOS6526::read (uint_least8_t addr)
{
    event_clock_t cycles;
    if (addr > 0x0f) return 0;

    cycles       = event_context.getTime (m_accessClk);
    m_accessClk += cycles;

    // Sync up timers
    if ((cra & 0x21) == 0x01)
        ta -= cycles;
    if ((crb & 0x61) == 0x01)
        tb -= cycles;

    switch (addr)
    {
    case 0x0: // Simulate a serial port
        dpa = ((dpa << 1) | (dpa >> 7)) & 0xff;
        if (dpa & 0x80)
            return 0xc0;
        return 0;
    case 0x4: return endian_16lo8 (ta);
    case 0x5: return endian_16hi8 (ta);
    case 0x6: return endian_16lo8 (tb);
    case 0x7: return endian_16hi8 (tb);

    case 0xd:
    {   // Clear IRQs, and return interrupt
        // data register
        uint8_t ret = idr;
        trigger (0);
        return ret;
    }

    case 0x0e: return cra;
    case 0x0f: return crb;
    default:  return regs[addr];
    }
}

void MOS6526::write (uint_least8_t addr, uint8_t data)
{
    event_clock_t cycles;
    if (addr > 0x0f) return;

    regs[addr]   = data;
    cycles       = event_context.getTime (m_accessClk);
    m_accessClk += cycles;

    // Sync up timers
    if ((cra & 0x21) == 0x01)
        ta -= cycles;
    if ((crb & 0x61) == 0x01)
        tb -= cycles;

    switch (addr)
    {
    case 0x4: endian_16lo8 (ta_latch, data); break;
    case 0x5:
        endian_16hi8 (ta_latch, data);
        if (!(cra & 0x01)) // Reload timer if stopped
            ta = ta_latch;
    break;

    case 0x6: endian_16lo8 (tb_latch, data); break;
    case 0x7:
        endian_16hi8 (tb_latch, data);
        if (!(crb & 0x01)) // Reload timer if stopped
            tb = tb_latch;
    break;

    case 0xd:
        if (data & 0x80)
            icr |= data & 0x1f;
        else
            icr &= ~data;
        trigger (idr);
    break;

    case 0x0e:
        // Check for forced load
        cra = data;
        if (data & 0x10)
        {
            cra &= (~0x10);
            ta   = ta_latch;
        }

        if ((data & 0x21) == 0x01)
        {   // Active
            event_context.schedule (&event_ta, (event_clock_t) ta + 1);
        } else
        {   // Inactive
            ta = ta_latch;
            event_context.cancel (&event_ta);
        }
    break;

    case 0x0f:
        // Check for forced load
        crb = data;
        if (data & 0x10)
        {
            crb &= (~0x10);
            tb   = tb_latch;
        }

        if ((data & 0x61) == 0x01)
        {   // Active
            event_context.schedule (&event_tb, (event_clock_t) tb + 1);
        } else
        {   // Inactive
            tb = tb_latch;
            event_context.cancel (&event_tb);
        }
    break;

    default:
    break;
    }
}

void MOS6526::trigger (int irq)
{
    if (!irq)
    {   // Clear any requested IRQs
        if (idr & INTERRUPT_REQUEST)
            interrupt (false);
        idr = 0;
        return;
    }

    idr |= irq;
    if (icr & idr)
    {
        if (!(idr & INTERRUPT_REQUEST))
        {
            idr |= INTERRUPT_REQUEST;
            interrupt (true);
        }
    }
}

void MOS6526::ta_event (void)
{   // Timer Modes
    event_clock_t cycles;
    uint8_t mode = cra & 0x21;

    if (mode == 0x21)
    {
        if (ta--)
            return;
    }

    cycles       = event_context.getTime (m_accessClk);
    m_accessClk += cycles;

    ta = ta_latch;
    if (cra & 0x08)
    {   // one shot, stop timer A
        cra &= (~0x01);
    } else if (mode == 0x01)
    {   // Reset event
        event_context.schedule (&event_ta, (event_clock_t) ta + 1);
    }
    trigger (INTERRUPT_TA);
    
    switch (crb & 0x61)
    {
    case 0x01: tb -= cycles; break;
    case 0x41:
    case 0x61:
        tb_event ();
    break;
    }
}
    
void MOS6526::tb_event (void)
{   // Timer Modes
    uint8_t mode = crb & 0x61;
    switch (mode)
    {
    case 0x01:
        break;

    case 0x21:
    case 0x41:
        if (tb--)
            return;
    break;

    case 0x61:
        if (cnt_high)
        {
            if (tb--)
                return;
        }
    break;
    
    default:
        return;
    }

    m_accessClk = event_context.getTime ();
    tb = tb_latch;
    if (crb & 0x08)
    {   // one shot, stop timer A
        crb &= (~0x01);
    } else if (mode == 0x01)
    {   // Reset event
        event_context.schedule (&event_tb, (event_clock_t) tb + 1);
    }
    trigger (INTERRUPT_TB);
}
