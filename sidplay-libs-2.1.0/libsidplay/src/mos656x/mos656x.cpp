/***************************************************************************

                          mos656x.cpp  -  Minimal VIC emulation
                             -------------------
    begin                : Wed May 21 2001
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

#include "sidendian.h"
#include "mos656x.h"

#define MOS6567R56A_FIRST_DMA_LINE 0x30
#define MOS6567R56A_LAST_DMA_LINE  0xf7

#define MOS6567R8_FIRST_DMA_LINE   0x30
#define MOS6567R8_LAST_DMA_LINE    0xf7

#define MOS6569_FIRST_DMA_LINE     0x30
#define MOS6569_LAST_DMA_LINE      0xff

const char *MOS656X::credit =
{   // Optional information
    "*MOS656X (VICII) Emulation:\0"
    "\tCopyright (C) 2001 Simon White <sidplay2@email.com>\0"
};


MOS656X::MOS656X (EventContext *context)
:Event("VIC Raster"),
 event_context(*context)
{
    chip  (MOS6569);
}

void MOS656X::reset ()
{
    icr          = idr = ctrl1 = 0;
    raster_irq   = 0;
    y_scroll     = 0;
    raster_y     = yrasters - 1;
    raster_x     = xrasters - 1;
    bad_lines_enabled = false;
    event_context.schedule (this, 1);
    m_accessClk  = 0;
}

void MOS656X::chip (mos656x_model_t model)
{
    switch (model)
    {
    // Seems to be an older NTSC chip
    case MOS6567R56A:
        yrasters = 262;
        xrasters = 64;
        first_dma_line = MOS6567R56A_FIRST_DMA_LINE;
        last_dma_line  = MOS6567R56A_LAST_DMA_LINE;
    break;

    // NTSC Chip
    case MOS6567R8:
        yrasters = 263;
        xrasters = 65;
        first_dma_line = MOS6567R8_FIRST_DMA_LINE;
        last_dma_line  = MOS6567R8_LAST_DMA_LINE;
    break;

    // PAL Chip
    case MOS6569:
        yrasters = 312;
        xrasters = 63;
        first_dma_line = MOS6569_FIRST_DMA_LINE;
        last_dma_line  = MOS6569_LAST_DMA_LINE;
    break;
    }

    reset ();
}

uint8_t MOS656X::read (uint_least8_t addr)
{
    if (addr > 0x3f) return 0;
    if (addr > 0x2e) return 0xff;
 
    switch (addr)
    {
    case 0x11:    // Control register 1 
        return (raster_y & 0x100) >> 1;
    case 0x12:    // Raster counter
        return raster_y & 0xFF; 
    case 0x19:    // IRQ flags 
        return idr; 
    case 0x1a:    // IRQ mask 
        return icr | 0xf0; 
    default: return regs[addr];
    }
}

void MOS656X::write (uint_least8_t addr, uint8_t data)
{
    if (addr > 0x3f) return;

    regs[addr] = data;

    switch (addr)
    {
    case 0x11: // Control register 1
        endian_16hi8 (raster_irq, data >> 7);
        ctrl1    = data;
        y_scroll = data & 7;

        if (raster_x < 11)
            break;

		// In line $30, the DEN bit controls if Bad Lines can occur
		if (raster_y == 0x30 && data & 0x10)
			bad_lines_enabled = true;
 
		// Bad Line condition?
        bad_line = (raster_y >= first_dma_line) &&
                   (raster_y <= last_dma_line)  &&
                   ((raster_y & 7) == y_scroll) &&
                   bad_lines_enabled;
        
        // Start DMA
        if (bad_line && (raster_x < 54))
        {
            busaccess(false);
            if (raster_x < 52)
                event_context.schedule (this, 3);
        }
        break;

    case 0x12: // Raster counter
        endian_16lo8 (raster_irq, data);
        break;

    case 0x19: // IRQ flags
        idr &= ((~data & 0x0f) | 0x80);
        if (idr == 0x80)
            trigger (0);
        break;

    case 0x1a: // IRQ mask 
        icr = data & 0x0f; 
        trigger (icr & idr); 
        break;
    }
}


void MOS656X::trigger (int irq)
{
    if (!irq)
    {   // Clear any requested IRQs
        if (idr & MOS656X_INTERRUPT_REQUEST)
            interrupt (false);
        idr = 0;
        return;
    }

    idr |= irq;
    if (icr & idr)
    {
        if (!(idr & MOS656X_INTERRUPT_REQUEST))
        {
            idr |= MOS656X_INTERRUPT_REQUEST;
            interrupt (true);
        }
    }
}

void MOS656X::event (void)
{
    event_clock_t delay = 1;

    switch (raster_x)
    {
    case 0:  // IRQ occurred (xraster != 0)
        if (raster_y != yrasters - 1)
        {
            raster_y++;
            // Trigger raster IRQ if IRQ line reached
            if (raster_y == raster_irq)
                trigger (MOS656X_INTERRUPT_RST);
            delay = 11;
        }
        break;
    case 1:  // Vertical blank (line 0)
		// Trigger raster IRQ if IRQ in line 0
        raster_y = 0;
        if (raster_y == raster_irq)
            trigger (MOS656X_INTERRUPT_RST);
        delay = 10;
        break;

    case 11: // Start bad line
    {   // In line $30, the DEN bit controls if Bad Lines can occur
        if (raster_y == first_dma_line)
	        bad_lines_enabled = (ctrl1 & 0x10) != 0;

        // Test for bad line condition
        bad_line = (raster_y >= first_dma_line) &&
                   (raster_y <= last_dma_line)  &&
                   ((raster_y & 7) == y_scroll) &&
                   bad_lines_enabled;

        delay = xrasters - 11;
        if (bad_line)
        {
            delay = 3;
            busaccess (false);
        }
        break;
    }

    case 14: // Start DMA
        addrctrl (false);
        delay = 40;
        break;
    case 54: // End DMA
        busaccess (true);
        addrctrl  (true);
        delay = xrasters - 54;
        break;

    case 12:
    case 13:
        break;

    default:
        if (bad_line && (raster_x < 54))
        {
            addrctrl (false);
            delay = 54 - raster_x;
        }
        else
        {   // Skip to the end of raster
            busaccess (true);
            delay = xrasters - raster_x;
        }
    }

    raster_x += delay;
    raster_x %= xrasters;
    event_context.schedule (this, delay);
}
