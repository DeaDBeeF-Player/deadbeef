/***************************************************************************
                          event.h  -  Event scheduler (based on alarm
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

#ifndef _event_h_
#define _event_h_

#include <stdio.h>
#include "sidtypes.h"

typedef uint_fast32_t event_clock_t;
#define EVENT_CONTEXT_MAX_PENDING_EVENTS 0x100

class SID_EXTERN Event
{
private:
    friend  class EventScheduler;
    const   char * const m_name;
    event_clock_t m_clk;

    /* This variable is set by the event context
       when it is scheduled */
    bool m_pending;

    /* Link to the next and previous events in the
       list.  */
    Event *m_next, *m_prev;

public:
    Event(const char * const name)
        : m_name(name),
          m_pending(false) {}

    virtual void event (void) = 0;
};

// Public Event Context
class EventContext
{
public:
    virtual void cancel   (Event *event) = 0;
    virtual void schedule (Event *event, event_clock_t cycles) = 0;
    virtual event_clock_t getTime (void) const = 0;
    virtual event_clock_t getTime (event_clock_t clock) const = 0;
};

// Private Event Context Object (The scheduler)
class EventScheduler: public EventContext
{
private:
    const char * const m_name;
    event_clock_t  m_eventClk, m_schedClk;
    uint  m_pendingEventClk;
    uint  m_pendingEventCount;

    class SID_EXTERN EventDummy: public Event
    {
    private:
        void event (void) {;}
    public:
        EventDummy () : Event("Bad Event: Dummy") {;}
        void reset() {
            m_clk = 0;
            m_pending = false;
            m_next = m_prev = NULL;
        }
    } m_pendingEvents;

    class SID_EXTERN EventTimeWarp: public Event
    {
    private:
        EventScheduler &m_scheduler;

        void event (void)
        {
            m_scheduler.timeWarp ();
        }

    public:
        EventTimeWarp (EventScheduler *context)
        :Event("Time Warp"),
         m_scheduler(*context)
        {;}
    } m_timeWarp;
    friend class EventTimeWarp;

private:
    void timeWarp (void);
    void dispatch (void)
    {
        Event &e = *m_pendingEvents.m_next;
        cancelPending (e);
        //printf ("Event \"%s\"\n", e.m_name);
        e.event ();
    }

    void cancelPending (Event &event)
    {
        event.m_pending      = false;
        event.m_prev->m_next = event.m_next;
        event.m_next->m_prev = event.m_prev;
        m_pendingEventClk    = m_pendingEvents.m_next->m_clk;
        m_pendingEventCount--;
    }

public:
    EventScheduler (const char * const name);
    void cancel    (Event *event);
    void reset     (void);
    void schedule  (Event *event, event_clock_t cycles);

    void clock (void)
    {
        if (m_pendingEventCount)
        {
            event_clock_t delta = m_pendingEventClk - m_eventClk;
            m_schedClk  += delta;
            m_eventClk  += delta;
            dispatch ();
        }
    }

    event_clock_t getTime (void) const
    {   return m_schedClk; }
    event_clock_t getTime (event_clock_t clock) const
    {   return m_schedClk - clock; }
};

#endif // _event_h_
