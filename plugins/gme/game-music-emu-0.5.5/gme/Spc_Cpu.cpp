// Game_Music_Emu 0.5.5. http://www.slack.net/~ant/

#include "Spc_Cpu.h"

#include "blargg_endian.h"
#include "Snes_Spc.h"

/* Copyright (C) 2004-2006 Shay Green. This module is free software; you
can redistribute it and/or modify it under the terms of the GNU Lesser
General Public License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version. This
module is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
details. You should have received a copy of the GNU Lesser General Public
License along with this module; if not, write to the Free Software Foundation,
Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA */

#include "blargg_source.h"

// Several instructions are commented out (or not even implemented). These aren't
// used by the SPC files tested.

// Optimize performance for the most common instructions, and size for the rest:
//
// 15%  0xF0    BEQ rel
//  8%  0xE4    MOV A,dp
//  4%  0xF5    MOV A,abs+X
//  4%  0xD0    BNE rel
//  4%  0x6F    RET
//  4%  0x3F    CALL addr
//  4%  0xF4    MOV A,dp+X
//  3%  0xC4    MOV dp,A
//  2%  0xEB    MOV Y,dp
//  2%  0x3D    INC X
//  2%  0xF6    MOV A,abs+Y
// (1% and below not shown)

Spc_Cpu::Spc_Cpu( Snes_Spc* e, uint8_t* ram_in ) : ram( ram_in ), emu( *e )
{
	remain_ = 0;
	assert( INT_MAX >= 0x7FFFFFFF ); // requires 32-bit int
	blargg_verify_byte_order();
}

#define READ( addr )            (emu.read( addr ))
#define WRITE( addr, value )    (emu.write( addr, value ))

#define READ_DP( addr )         READ( (addr) + dp )
#define WRITE_DP( addr, value ) WRITE( (addr) + dp, value )

#define READ_PROG( addr )       (ram [addr])
#define READ_PROG16( addr )     GET_LE16( &READ_PROG( addr ) )

int Spc_Cpu::read( spc_addr_t addr )
{
	return READ( addr );
}

void Spc_Cpu::write( spc_addr_t addr, int data )
{
	WRITE( addr, data );
}

// Cycle table derived from text copy of SPC-700 manual (using regular expressions)
static unsigned char const cycle_table [0x100] = {
//  0 1 2 3 4 5 6 7 8 9 A B C D E F
	2,8,4,5,3,4,3,6,2,6,5,4,5,4,6,8, // 0
	2,8,4,5,4,5,5,6,5,5,6,5,2,2,4,6, // 1
	2,8,4,5,3,4,3,6,2,6,5,4,5,4,5,4, // 2
	2,8,4,5,4,5,5,6,5,5,6,5,2,2,3,8, // 3
	2,8,4,5,3,4,3,6,2,6,4,4,5,4,6,6, // 4
	2,8,4,5,4,5,5,6,5,5,4,5,2,2,4,3, // 5
	2,8,4,5,3,4,3,6,2,6,4,4,5,4,5,5, // 6
	2,8,4,5,4,5,5,6,5,5,5,5,2,2,3,6, // 7
	2,8,4,5,3,4,3,6,2,6,5,4,5,2,4,5, // 8
	2,8,4,5,4,5,5,6,5,5,5,5,2,2,12,5,// 9
	3,8,4,5,3,4,3,6,2,6,4,4,5,2,4,4, // A
	2,8,4,5,4,5,5,6,5,5,5,5,2,2,3,4, // B
	3,8,4,5,4,5,4,7,2,5,6,4,5,2,4,9, // C
	2,8,4,5,5,6,6,7,4,5,4,5,2,2,6,3, // D
	2,8,4,5,3,4,3,6,2,4,5,3,4,3,4,3, // E
	2,8,4,5,4,5,5,6,3,4,5,4,2,2,4,3  // F
};

// The C,mem instructions are hardly used, so a non-inline function is used for
// the common access code.
unsigned Spc_Cpu::mem_bit( spc_addr_t pc )
{
	unsigned addr = READ_PROG16( pc );
	unsigned t = READ( addr & 0x1FFF ) >> (addr >> 13);
	return (t << 8) & 0x100;
}

spc_time_t Spc_Cpu::run( spc_time_t cycle_count )
{
	remain_ = cycle_count;
	
	uint8_t* const ram = this->ram; // cache
	
	// Stack pointer is kept one greater than usual SPC stack pointer to allow
	// common pre-decrement and post-increment memory instructions that some
	// processors have. Address wrap-around isn't supported.
	#define PUSH( v )       (*--sp = uint8_t (v))
	#define PUSH16( v )     (sp -= 2, SET_LE16( sp, v ))
	#define POP()           (*sp++)
	#define SET_SP( v )     (sp = ram + 0x101 + (v))
	#define GET_SP()        (sp - 0x101 - ram)

	uint8_t* sp;
	SET_SP( r.sp );
	
	// registers
	unsigned pc = (unsigned) r.pc;
	int a = r.a;
	int x = r.x;
	int y = r.y;
	
	// status flags
	
	const int st_n = 0x80;
	const int st_v = 0x40;
	const int st_p = 0x20;
	const int st_b = 0x10;
	const int st_h = 0x08;
	const int st_i = 0x04;
	const int st_z = 0x02;
	const int st_c = 0x01;
	
	#define IS_NEG (nz & 0x880)
	
	#define CALC_STATUS( out ) do {\
		out = status & ~(st_n | st_z | st_c);\
		out |= (c >> 8) & st_c;\
		out |= (dp >> 3) & st_p;\
		if ( IS_NEG ) out |= st_n;\
		if ( !(nz & 0xFF) ) out |= st_z;\
	} while ( 0 )       

	#define SET_STATUS( in ) do {\
		status = in & ~(st_n | st_z | st_c | st_p);\
		c = in << 8;\
		nz = (in << 4) & 0x800;\
		nz |= ~in & st_z;\
		dp = (in << 3) & 0x100;\
	} while ( 0 )
	
	int status;
	int c;  // store C as 'c' & 0x100.
	int nz; // Z set if (nz & 0xFF) == 0, N set if (nz & 0x880) != 0
	unsigned dp; // direct page base
	{
		int temp = r.status;
		SET_STATUS( temp );
	}

	goto loop;
	
	unsigned data; // first operand of instruction and temporary across function calls
	
	// Common endings for instructions
cbranch_taken_loop: // compare and branch
	pc += (BOOST::int8_t) READ_PROG( pc );
	remain_ -= 2;
inc_pc_loop: // end of instruction with an operand
	pc++;
loop:
	
	check( (unsigned) pc < 0x10000 );
	check( (unsigned) GET_SP() < 0x100 );
	
	check( (unsigned) a < 0x100 );
	check( (unsigned) x < 0x100 );
	check( (unsigned) y < 0x100 );
	
	unsigned opcode = READ_PROG( pc );
	pc++;
	// to do: if pc is at end of memory, this will get wrong byte
	data = READ_PROG( pc );
	
	if ( remain_ <= 0 )
		goto stop;
	
	remain_ -= cycle_table [opcode];
	
	// Use 'data' for temporaries whose lifetime crosses read/write calls, otherwise
	// use a local temporary.
	switch ( opcode )
	{
	
	#define BRANCH( cond ) {\
		pc++;\
		int offset = (BOOST::int8_t) data;\
		if ( cond ) {\
			pc += offset;\
			remain_ -= 2;\
		}\
		goto loop;\
	}
	
// Most-Common

	case 0xF0: // BEQ (most common)
		BRANCH( !(uint8_t) nz )
	
	case 0xD0: // BNE
		BRANCH( (uint8_t) nz )
	
	case 0x3F: // CALL
		PUSH16( pc + 2 );
		pc = READ_PROG16( pc );
		goto loop;
	
	case 0x6F: // RET
		pc = POP();
		pc += POP() * 0x100;
		goto loop;

#define CASE( n )   case n:

// Define common address modes based on opcode for immediate mode. Execution
// ends with data set to the address of the operand.
#define ADDR_MODES( op )\
	CASE( op - 0x02 ) /* (X) */\
		data = x + dp;\
		pc--;\
		goto end_##op;\
	CASE( op + 0x0F ) /* (dp)+Y */\
		data = READ_PROG16( data + dp ) + y;\
		goto end_##op;\
	CASE( op - 0x01 ) /* (dp+X) */\
		data = READ_PROG16( uint8_t (data + x) + dp );\
		goto end_##op;\
	CASE( op + 0x0E ) /* abs+Y */\
		data += y;\
		goto abs_##op;\
	CASE( op + 0x0D ) /* abs+X */\
		data += x;\
	CASE( op - 0x03 ) /* abs */\
	abs_##op:\
		pc++;\
		data += 0x100 * READ_PROG( pc );\
		goto end_##op;\
	CASE( op + 0x0C ) /* dp+X */\
		data = uint8_t (data + x);\
	CASE( op - 0x04 ) /* dp */\
		data += dp;\
	end_##op:

// 1. 8-bit Data Transmission Commands. Group I

	ADDR_MODES( 0xE8 ) // MOV A,addr
	// case 0xE4: // MOV a,dp (most common)
	mov_a_addr:
		a = nz = READ( data );
		goto inc_pc_loop;
	case 0xBF: // MOV A,(X)+
		data = x + dp;
		x = uint8_t (x + 1);
		pc--;
		goto mov_a_addr;
	
	case 0xE8: // MOV A,imm
		a = data;
		nz = data;
		goto inc_pc_loop;
	
	case 0xF9: // MOV X,dp+Y
		data = uint8_t (data + y);
	case 0xF8: // MOV X,dp
		data += dp;
		goto mov_x_addr;
	case 0xE9: // MOV X,abs
		data = READ_PROG16( pc );
		pc++;
	mov_x_addr:
		data = READ( data );
	case 0xCD: // MOV X,imm
		x = data;
		nz = data;
		goto inc_pc_loop;
	
	case 0xFB: // MOV Y,dp+X
		data = uint8_t (data + x);
	case 0xEB: // MOV Y,dp
		data += dp;
		goto mov_y_addr;
	case 0xEC: // MOV Y,abs
		data = READ_PROG16( pc );
		pc++;
	mov_y_addr:
		data = READ( data );
	case 0x8D: // MOV Y,imm
		y = data;
		nz = data;
		goto inc_pc_loop;

// 2. 8-BIT DATA TRANSMISSION COMMANDS, GROUP 2

	ADDR_MODES( 0xC8 ) // MOV addr,A
		WRITE( data, a );
		goto inc_pc_loop;
	
	{
		int temp;
	case 0xCC: // MOV abs,Y
		temp = y;
		goto mov_abs_temp;
	case 0xC9: // MOV abs,X
		temp = x;
	mov_abs_temp:
		WRITE( READ_PROG16( pc ), temp );
		pc += 2;
		goto loop;
	}
	
	case 0xD9: // MOV dp+Y,X
		data = uint8_t (data + y);
	case 0xD8: // MOV dp,X
		WRITE( data + dp, x );
		goto inc_pc_loop;
	
	case 0xDB: // MOV dp+X,Y
		data = uint8_t (data + x);
	case 0xCB: // MOV dp,Y
		WRITE( data + dp, y );
		goto inc_pc_loop;

	case 0xFA: // MOV dp,dp
		data = READ( data + dp );
	case 0x8F: // MOV dp,#imm
		pc++;
		WRITE_DP( READ_PROG( pc ), data );
		goto inc_pc_loop;
	
// 3. 8-BIT DATA TRANSMISSIN COMMANDS, GROUP 3.
	
	case 0x7D: // MOV A,X
		a = x;
		nz = x;
		goto loop;
	
	case 0xDD: // MOV A,Y
		a = y;
		nz = y;
		goto loop;
	
	case 0x5D: // MOV X,A
		x = a;
		nz = a;
		goto loop;
	
	case 0xFD: // MOV Y,A
		y = a;
		nz = a;
		goto loop;
	
	case 0x9D: // MOV X,SP
		x = nz = GET_SP();
		goto loop;
	
	case 0xBD: // MOV SP,X
		SET_SP( x );
		goto loop;
	
	//case 0xC6: // MOV (X),A (handled by MOV addr,A in group 2)
	
	case 0xAF: // MOV (X)+,A
		WRITE_DP( x, a );
		x++;
		goto loop;
	
// 5. 8-BIT LOGIC OPERATION COMMANDS
	
#define LOGICAL_OP( op, func )\
	ADDR_MODES( op ) /* addr */\
		data = READ( data );\
	case op: /* imm */\
		nz = a func##= data;\
		goto inc_pc_loop;\
	{   unsigned addr;\
	case op + 0x11: /* X,Y */\
		data = READ_DP( y );\
		addr = x + dp;\
		pc--;\
		goto addr_##op;\
	case op + 0x01: /* dp,dp */\
		data = READ_DP( data );\
	case op + 0x10: /*dp,imm*/\
		pc++;\
		addr = READ_PROG( pc ) + dp;\
	addr_##op:\
		nz = data func READ( addr );\
		WRITE( addr, nz );\
		goto inc_pc_loop;\
	}
	
	LOGICAL_OP( 0x28, & ); // AND
	
	LOGICAL_OP( 0x08, | ); // OR
	
	LOGICAL_OP( 0x48, ^ ); // EOR
	
// 4. 8-BIT ARITHMETIC OPERATION COMMANDS

	ADDR_MODES( 0x68 ) // CMP addr
		data = READ( data );
	case 0x68: // CMP imm
		nz = a - data;
		c = ~nz;
		nz &= 0xFF;
		goto inc_pc_loop;
	
	case 0x79: // CMP (X),(Y)
		data = READ_DP( x );
		nz = data - READ_DP( y );
		c = ~nz;
		nz &= 0xFF;
		goto loop;
	
	case 0x69: // CMP (dp),(dp)
		data = READ_DP( data );
	case 0x78: // CMP dp,imm
		pc++;
		nz = READ_DP( READ_PROG( pc ) ) - data;
		c = ~nz;
		nz &= 0xFF;
		goto inc_pc_loop;
	
	case 0x3E: // CMP X,dp
		data += dp;
		goto cmp_x_addr;
	case 0x1E: // CMP X,abs
		data = READ_PROG16( pc );
		pc++;
	cmp_x_addr:
		data = READ( data );
	case 0xC8: // CMP X,imm
		nz = x - data;
		c = ~nz;
		nz &= 0xFF;
		goto inc_pc_loop;
	
	case 0x7E: // CMP Y,dp
		data += dp;
		goto cmp_y_addr;
	case 0x5E: // CMP Y,abs
		data = READ_PROG16( pc );
		pc++;
	cmp_y_addr:
		data = READ( data );
	case 0xAD: // CMP Y,imm
		nz = y - data;
		c = ~nz;
		nz &= 0xFF;
		goto inc_pc_loop;
	
	{
		int addr;
	case 0xB9: // SBC (x),(y)
	case 0x99: // ADC (x),(y)
		pc--; // compensate for inc later
		data = READ_DP( x );
		addr = y + dp;
		goto adc_addr;
	case 0xA9: // SBC dp,dp
	case 0x89: // ADC dp,dp
		data = READ_DP( data );
	case 0xB8: // SBC dp,imm
	case 0x98: // ADC dp,imm
		pc++;
		addr = READ_PROG( pc ) + dp;
	adc_addr:
		nz = READ( addr );
		goto adc_data;
		
// catch ADC and SBC together, then decode later based on operand
#undef CASE
#define CASE( n ) case n: case (n) + 0x20:
	ADDR_MODES( 0x88 ) // ADC/SBC addr
		data = READ( data );
	case 0xA8: // SBC imm
	case 0x88: // ADC imm
		addr = -1; // A
		nz = a;
	adc_data: {
		if ( opcode & 0x20 )
			data ^= 0xFF; // SBC
		int carry = (c >> 8) & 1;
		int ov = (nz ^ 0x80) + carry + (BOOST::int8_t) data; // sign-extend
		int hc = (nz & 15) + carry;
		c = nz += data + carry;
		hc = (nz & 15) - hc;
		status = (status & ~(st_v | st_h)) | ((ov >> 2) & st_v) | ((hc >> 1) & st_h);
		if ( addr < 0 ) {
			a = (uint8_t) nz;
			goto inc_pc_loop;
		}
		WRITE( addr, (uint8_t) nz );
		goto inc_pc_loop;
	}
	
	}
	
// 6. ADDITION & SUBTRACTION COMMANDS

#define INC_DEC_REG( reg, n )\
		nz = reg + n;\
		reg = (uint8_t) nz;\
		goto loop;

	case 0xBC: INC_DEC_REG( a, 1 )  // INC A
	case 0x3D: INC_DEC_REG( x, 1 )  // INC X
	case 0xFC: INC_DEC_REG( y, 1 )  // INC Y
	
	case 0x9C: INC_DEC_REG( a, -1 ) // DEC A
	case 0x1D: INC_DEC_REG( x, -1 ) // DEC X
	case 0xDC: INC_DEC_REG( y, -1 ) // DEC Y

	case 0x9B: // DEC dp+X
	case 0xBB: // INC dp+X
		data = uint8_t (data + x);
	case 0x8B: // DEC dp
	case 0xAB: // INC dp
		data += dp;
		goto inc_abs;
	case 0x8C: // DEC abs
	case 0xAC: // INC abs
		data = READ_PROG16( pc );
		pc++;
	inc_abs:
		nz = ((opcode >> 4) & 2) - 1;
		nz += READ( data );
		WRITE( data, (uint8_t) nz );
		goto inc_pc_loop;
	
// 7. SHIFT, ROTATION COMMANDS

	case 0x5C: // LSR A
		c = 0;
	case 0x7C:{// ROR A
		nz = ((c >> 1) & 0x80) | (a >> 1);
		c = a << 8;
		a = nz;
		goto loop;
	}
	
	case 0x1C: // ASL A
		c = 0;
	case 0x3C:{// ROL A
		int temp = (c >> 8) & 1;
		c = a << 1;
		nz = c | temp;
		a = (uint8_t) nz;
		goto loop;
	}
	
	case 0x0B: // ASL dp
		c = 0;
		data += dp;
		goto rol_mem;
	case 0x1B: // ASL dp+X
		c = 0;
	case 0x3B: // ROL dp+X
		data = uint8_t (data + x);
	case 0x2B: // ROL dp
		data += dp;
		goto rol_mem;
	case 0x0C: // ASL abs
		c = 0;
	case 0x2C: // ROL abs
		data = READ_PROG16( pc );
		pc++;
	rol_mem:
		nz = (c >> 8) & 1;
		nz |= (c = READ( data ) << 1);
		WRITE( data, (uint8_t) nz );
		goto inc_pc_loop;
	
	case 0x4B: // LSR dp
		c = 0;
		data += dp;
		goto ror_mem;
	case 0x5B: // LSR dp+X
		c = 0;
	case 0x7B: // ROR dp+X
		data = uint8_t (data + x);
	case 0x6B: // ROR dp
		data += dp;
		goto ror_mem;
	case 0x4C: // LSR abs
		c = 0;
	case 0x6C: // ROR abs
		data = READ_PROG16( pc );
		pc++;
	ror_mem: {
		int temp = READ( data );
		nz = ((c >> 1) & 0x80) | (temp >> 1);
		c = temp << 8;
		WRITE( data, nz );
		goto inc_pc_loop;
	}

	case 0x9F: // XCN
		nz = a = (a >> 4) | uint8_t (a << 4);
		goto loop;

// 8. 16-BIT TRANSMISION COMMANDS

	case 0xBA: // MOVW YA,dp
		a = READ_DP( data );
		nz = (a & 0x7F) | (a >> 1);
		y = READ_DP( uint8_t (data + 1) );
		nz |= y;
		goto inc_pc_loop;
	
	case 0xDA: // MOVW dp,YA
		WRITE_DP( data, a );
		WRITE_DP( uint8_t (data + 1), y );
		goto inc_pc_loop;
	
// 9. 16-BIT OPERATION COMMANDS

	case 0x3A: // INCW dp
	case 0x1A:{// DECW dp
		data += dp;
		
		// low byte
		int temp = READ( data );
		temp += ((opcode >> 4) & 2) - 1; // +1 for INCW, -1 for DECW
		nz = ((temp >> 1) | temp) & 0x7F;
		WRITE( data, (uint8_t) temp );
		
		// high byte
		data = uint8_t (data + 1) + dp;
		temp >>= 8;
		temp = uint8_t (temp + READ( data ));
		nz |= temp;
		WRITE( data, temp );
		
		goto inc_pc_loop;
	}
		
	case 0x9A: // SUBW YA,dp
	case 0x7A: // ADDW YA,dp
	{
		// read 16-bit addend
		int temp = READ_DP( data );
		int sign = READ_DP( uint8_t (data + 1) );
		temp += 0x100 * sign;
		status &= ~(st_v | st_h);
		
		// to do: fix half-carry for SUBW (it's probably wrong)
		
		// for SUBW, negate and truncate to 16 bits
		if ( opcode & 0x80 ) {
			temp = (temp ^ 0xFFFF) + 1;
			sign = temp >> 8;
		}
		
		// add low byte (A)
		temp += a;
		a = (uint8_t) temp;
		nz = (temp | (temp >> 1)) & 0x7F;
		
		// add high byte (Y)
		temp >>= 8;
		c = y + temp;
		nz = (nz | c) & 0xFF;
		
		// half-carry (temporary avoids CodeWarrior optimizer bug)
		unsigned hc = (c & 15) - (y & 15);
		status |= (hc >> 4) & st_h;
		
		// overflow if sign of YA changed when previous sign and addend sign were same
		status |= (((c ^ y) & ~(y ^ sign)) >> 1) & st_v;
		
		y = (uint8_t) c;
		
		goto inc_pc_loop;
	}
	
	case 0x5A: { // CMPW YA,dp
		int temp = a - READ_DP( data );
		nz = ((temp >> 1) | temp) & 0x7F;
		temp = y + (temp >> 8);
		temp -= READ_DP( uint8_t (data + 1) );
		nz |= temp;
		c = ~temp;
		nz &= 0xFF;
		goto inc_pc_loop;
	}
	
// 10. MULTIPLICATION & DIVISON COMMANDS

	case 0xCF: { // MUL YA
		unsigned temp = y * a;
		a = (uint8_t) temp;
		nz = ((temp >> 1) | temp) & 0x7F;
		y = temp >> 8;
		nz |= y;
		goto loop;
	}
	
	case 0x9E: // DIV YA,X
	{
		// behavior based on SPC CPU tests
		
		status &= ~(st_h | st_v);
		
		if ( (y & 15) >= (x & 15) )
			status |= st_h;
		
		if ( y >= x )
			status |= st_v;
		
		unsigned ya = y * 0x100 + a;
		if ( y < x * 2 )
		{
			a = ya / x;
			y = ya - a * x;
		}
		else
		{
			a = 255 - (ya - x * 0x200) / (256 - x);
			y = x   + (ya - x * 0x200) % (256 - x);
		}
		
		nz = (uint8_t) a;
		a = (uint8_t) a;
		
		goto loop;
	}
	
// 11. DECIMAL COMPENSATION COMMANDS
	
	// seem unused
	// case 0xDF: // DAA
	// case 0xBE: // DAS
	
// 12. BRANCHING COMMANDS

	case 0x2F: // BRA rel
		pc += (BOOST::int8_t) data;
		goto inc_pc_loop;
	
	case 0x30: // BMI
		BRANCH( IS_NEG )
	
	case 0x10: // BPL
		BRANCH( !IS_NEG )
	
	case 0xB0: // BCS
		BRANCH( c & 0x100 )
	
	case 0x90: // BCC
		BRANCH( !(c & 0x100) )
	
	case 0x70: // BVS
		BRANCH( status & st_v )
	
	case 0x50: // BVC
		BRANCH( !(status & st_v) )
	
	case 0x03: // BBS dp.bit,rel
	case 0x23:
	case 0x43:
	case 0x63:
	case 0x83:
	case 0xA3:
	case 0xC3:
	case 0xE3:
		pc++;
		if ( (READ_DP( data ) >> (opcode >> 5)) & 1 )
			goto cbranch_taken_loop;
		goto inc_pc_loop;
	
	case 0x13: // BBC dp.bit,rel
	case 0x33:
	case 0x53:
	case 0x73:
	case 0x93:
	case 0xB3:
	case 0xD3:
	case 0xF3:
		pc++;
		if ( !((READ_DP( data ) >> (opcode >> 5)) & 1) )
			goto cbranch_taken_loop;
		goto inc_pc_loop;
	
	case 0xDE: // CBNE dp+X,rel
		data = uint8_t (data + x);
		// fall through
	case 0x2E: // CBNE dp,rel
		pc++;
		if ( READ_DP( data ) != a )
			goto cbranch_taken_loop;
		goto inc_pc_loop;
	
	case 0xFE: // DBNZ Y,rel
		y = uint8_t (y - 1);
		BRANCH( y )
	
	case 0x6E: { // DBNZ dp,rel
		pc++;
		unsigned temp = READ_DP( data ) - 1;
		WRITE_DP( (uint8_t) data, (uint8_t) temp );
		if ( temp )
			goto cbranch_taken_loop;
		goto inc_pc_loop;
	}
	
	case 0x1F: // JMP (abs+X)
		pc = READ_PROG16( pc ) + x;
		// fall through
	case 0x5F: // JMP abs
		pc = READ_PROG16( pc );
		goto loop;
	
// 13. SUB-ROUTINE CALL RETURN COMMANDS
	
	case 0x0F:{// BRK
		check( false ); // untested
		PUSH16( pc + 1 );
		pc = READ_PROG16( 0xFFDE ); // vector address verified
		int temp;
		CALC_STATUS( temp );
		PUSH( temp );
		status = (status | st_b) & ~st_i;
		goto loop;
	}
	
	case 0x4F: // PCALL offset
		pc++;
		PUSH16( pc );
		pc = 0xFF00 + data;
		goto loop;
	
	case 0x01: // TCALL n
	case 0x11:
	case 0x21:
	case 0x31:
	case 0x41:
	case 0x51:
	case 0x61:
	case 0x71:
	case 0x81:
	case 0x91:
	case 0xA1:
	case 0xB1:
	case 0xC1:
	case 0xD1:
	case 0xE1:
	case 0xF1:
		PUSH16( pc );
		pc = READ_PROG16( 0xFFDE - (opcode >> 3) );
		goto loop;
	
// 14. STACK OPERATION COMMANDS

	{
		int temp;
	case 0x7F: // RET1
		temp = POP();
		pc = POP();
		pc |= POP() << 8;
		goto set_status;
	case 0x8E: // POP PSW
		temp = POP();
	set_status:
		SET_STATUS( temp );
		goto loop;
	}
	
	case 0x0D: { // PUSH PSW
		int temp;
		CALC_STATUS( temp );
		PUSH( temp );
		goto loop;
	}

	case 0x2D: // PUSH A
		PUSH( a );
		goto loop;
	
	case 0x4D: // PUSH X
		PUSH( x );
		goto loop;
	
	case 0x6D: // PUSH Y
		PUSH( y );
		goto loop;
	
	case 0xAE: // POP A
		a = POP();
		goto loop;
	
	case 0xCE: // POP X
		x = POP();
		goto loop;
	
	case 0xEE: // POP Y
		y = POP();
		goto loop;
	
// 15. BIT OPERATION COMMANDS

	case 0x02: // SET1
	case 0x22:
	case 0x42:
	case 0x62:
	case 0x82:
	case 0xA2:
	case 0xC2:
	case 0xE2:
	case 0x12: // CLR1
	case 0x32:
	case 0x52:
	case 0x72:
	case 0x92:
	case 0xB2:
	case 0xD2:
	case 0xF2: {
		data += dp;
		int bit = 1 << (opcode >> 5);
		int mask = ~bit;
		if ( opcode & 0x10 )
			bit = 0;
		WRITE( data, (READ( data ) & mask) | bit );
		goto inc_pc_loop;
	}
		
	case 0x0E: // TSET1 abs
	case 0x4E:{// TCLR1 abs
		data = READ_PROG16( pc );
		pc += 2;
		unsigned temp = READ( data );
		nz = temp & a;
		temp &= ~a;
		if ( !(opcode & 0x40) )
			temp |= a;
		WRITE( data, temp );
		goto loop;
	}
	
	case 0x4A: // AND1 C,mem.bit
		c &= mem_bit( pc );
		pc += 2;
		goto loop;
	
	case 0x6A: // AND1 C,/mem.bit
		check( false ); // untested
		c &= ~mem_bit( pc );
		pc += 2;
		goto loop;
	
	case 0x0A: // OR1 C,mem.bit
		check( false ); // untested
		c |= mem_bit( pc );
		pc += 2;
		goto loop;
	
	case 0x2A: // OR1 C,/mem.bit
		check( false ); // untested
		c |= ~mem_bit( pc );
		pc += 2;
		goto loop;
	
	case 0x8A: // EOR1 C,mem.bit
		c ^= mem_bit( pc );
		pc += 2;
		goto loop;
	
	case 0xEA: { // NOT1 mem.bit
		data = READ_PROG16( pc );
		pc += 2;
		unsigned temp = READ( data & 0x1FFF );
		temp ^= 1 << (data >> 13);
		WRITE( data & 0x1FFF, temp );
		goto loop;
	}
	
	case 0xCA: { // MOV1 mem.bit,C
		data = READ_PROG16( pc );
		pc += 2;
		unsigned temp = READ( data & 0x1FFF );
		unsigned bit = data >> 13;
		temp = (temp & ~(1 << bit)) | (((c >> 8) & 1) << bit);
		WRITE( data & 0x1FFF, temp );
		goto loop;
	}
	
	case 0xAA: // MOV1 C,mem.bit
		c = mem_bit( pc );
		pc += 2;
		goto loop;
	
// 16. PROGRAM STATUS FLAG OPERATION COMMANDS

	case 0x60: // CLRC
		c = 0;
		goto loop;
		
	case 0x80: // SETC
		c = ~0;
		goto loop;
	
	case 0xED: // NOTC
		c ^= 0x100;
		goto loop;
		
	case 0xE0: // CLRV
		status &= ~(st_v | st_h);
		goto loop;
	
	case 0x20: // CLRP
		dp = 0;
		goto loop;
	
	case 0x40: // SETP
		dp = 0x100;
		goto loop;
	
	case 0xA0: // EI
		check( false ); // untested
		status |= st_i;
		goto loop;
	
	case 0xC0: // DI
		check( false ); // untested
		status &= ~st_i;
		goto loop;
	
// 17. OTHER COMMANDS

	case 0x00: // NOP
		goto loop;
	
	//case 0xEF: // SLEEP
	//case 0xFF: // STOP
	
	} // switch
	
	// unhandled instructions fall out of switch so emulator can catch them
	
stop:
	pc--;
	
	{
		int temp;
		CALC_STATUS( temp );
		r.status = (uint8_t) temp;
	}
	
	r.pc = pc;
	r.sp = (uint8_t) GET_SP();
	r.a  = (uint8_t) a;
	r.x  = (uint8_t) x;
	r.y  = (uint8_t) y;
	
	return remain_;
}
