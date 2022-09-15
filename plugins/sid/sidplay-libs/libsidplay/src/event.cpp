/***************************************************************************
                          event.cpp  -  Event schdeduler (based on alarm
                                        from Vice)
                             -------------------
    begin                : Wed May 9 2001
    copyright            : (C) 2001 by Simon White
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
 *  $Log: event.cpp,v $
 *  Revision 1.7  2002/11/21 19:55:38  s_a_white
 *  We now jump to next event directly instead on clocking by a number of
 *  cycles.
 *
 *  Revision 1.6  2002/07/17 19:20:03  s_a_white
 *  More efficient event handling code.
 *
 *  Revision 1.5  2001/10/02 18:24:09  s_a_white
 *  Updated to support safe scheduler interface.
 *
 *  Revision 1.4  2001/09/17 19:00:28  s_a_white
 *  Constructor moved out of line.
 *
 *  Revision 1.3  2001/09/15 13:03:50  s_a_white
 *  timeWarp now zeros m_eventClk instead of m_pendingEventClk which
 *  fixes a inifinite loop problem when driving libsidplay1.
 *
 ***************************************************************************/

#include <string.h>
#include "event.h"

#define EVENT_TIMEWARP_COUNT 0x0FFFFF


EventScheduler::EventScheduler (const char * const name)
:m_name(name),
 m_pendingEventCount(0),
 m_timeWarp(this)
{
    m_pendingEvents.reset();
    m_pendingEvents.m_next = &m_pendingEvents;
    m_pendingEvents.m_prev = &m_pendingEvents;
    reset  ();
}

// Usefull to prevent clock overflowing
void EventScheduler::timeWarp ()
{
    Event *e     = &m_pendingEvents;
    uint   count = m_pendingEventCount;
    while (count--)
    {   // Reduce all event clocks and clip them
        // so none go negative
        event_clock_t clk;
        e   = e->m_next;
        clk = e->m_clk;
        e->m_clk = 0;
        if (clk >= m_eventClk)
            e->m_clk = clk - m_eventClk;
    }
    m_eventClk = 0;
    // Re-schedule the next timeWarp
    schedule (&m_timeWarp, EVENT_TIMEWARP_COUNT);
}

void EventScheduler::reset (void)
{    // Remove all events
    Event *e     = &m_pendingEvents;
    uint   count = m_pendingEventCount;
    while (count--)
    {
        e = e->m_next;
        e->m_pending = false;
    }
    m_pendingEvents.m_next = &m_pendingEvents;
    m_pendingEvents.m_prev = &m_pendingEvents;
    m_pendingEventClk      = m_eventClk = m_schedClk = 0;
    m_pendingEventCount    = 0;
    timeWarp ();
}

// Add event to ordered pending queue
void EventScheduler::schedule (Event *event, event_clock_t cycles)
{
    uint clk = m_eventClk + cycles;
    if (event->m_pending)
        cancelPending (*event);
    event->m_pending = true;
    event->m_clk     = clk;

    {   // Now put in the correct place so we don't need to keep
        // searching the list later.
        Event *e     = m_pendingEvents.m_next;
        uint   count = m_pendingEventCount;
        while (count-- && (e->m_clk <= clk))
            e = e->m_next;
        event->m_next     = e;
        event->m_prev     = e->m_prev;
        e->m_prev->m_next = event;
        e->m_prev         = event;
        m_pendingEventClk = m_pendingEvents.m_next->m_clk;
        m_pendingEventCount++;
    }
}

// Cancel a pending event
void EventScheduler::cancel (Event *event)
{
    if (event->m_pending)
        cancelPending (*event);
}
