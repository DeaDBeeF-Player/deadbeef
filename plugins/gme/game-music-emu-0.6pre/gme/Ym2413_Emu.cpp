// Game_Music_Emu $vers. http://www.slack.net/~ant/

#include "Ym2413_Emu.h"

extern "C" {
#include "../vgmplay/VGMPlay/chips/emu2413.h"
}

Ym2413_Emu::Ym2413_Emu() { opll = 0; }

Ym2413_Emu::~Ym2413_Emu()
{
	if ( opll ) OPLL_delete( (OPLL *) opll );
}

int Ym2413_Emu::set_rate( int sample_rate, int clock_rate )
{
	if ( opll )
	{
        OPLL_delete( (OPLL *) opll );
		opll = 0;
	}
	
	opll = OPLL_new( clock_rate, sample_rate );
	if ( !opll )
		return 1;
    
    OPLL_SetChipMode( (OPLL *) opll, 0 );
	
	reset();
	return 0;
}

void Ym2413_Emu::reset()
{
	OPLL_reset( (OPLL *) opll );
	OPLL_SetMuteMask( (OPLL *) opll, 0 );
}

void Ym2413_Emu::write( int addr, int data )
{
	OPLL_writeIO( (OPLL *) opll, 0, addr );
	OPLL_writeIO( (OPLL *) opll, 1, data );
}

void Ym2413_Emu::mute_voices( int mask )
{
	OPLL_SetMuteMask( (OPLL *) opll, mask );
}

void Ym2413_Emu::run( int pair_count, sample_t* out )
{
	e_int32 bufMO[ 1024 ];
	e_int32 bufRO[ 1024 ];
	e_int32 * buffers[2] = { bufMO, bufRO };

	while (pair_count > 0)
	{
		int todo = pair_count;
		if (todo > 1024) todo = 1024;
		OPLL_calc_stereo( (OPLL *) opll, buffers, todo, -1 );

		for (int i = 0; i < todo; i++)
		{
			int output_l, output_r;
			int output = bufMO [i];
			output += bufRO [i];
			output *= 3;
			output_l = output + out [0];
			output_r = output + out [1];
			if ( (short)output_l != output_l ) output_l = 0x7FFF ^ ( output_l >> 31 );
			if ( (short)output_r != output_r ) output_r = 0x7FFF ^ ( output_r >> 31 );
			out [0] = output_l;
			out [1] = output_r;
			out += 2;
		}

		pair_count -= todo;
	}
}
