/***************************************************************************
                          main.cpp  -  description
                             -------------------
    begin                : Thu May 11 06:22:40 BST 2000
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
 *  $Log: mos6510.cpp,v $
 *  Revision 1.8  2001/08/05 15:46:38  s_a_white
 *  No longer need to check on which cycle to print debug information.
 *
 *  Revision 1.7  2001/07/14 13:04:34  s_a_white
 *  Accumulator is now unsigned, which improves code readability.
 *
 *  Revision 1.6  2001/03/09 22:27:46  s_a_white
 *  Speed optimisation update.
 *
 *  Revision 1.5  2001/02/13 23:01:10  s_a_white
 *  envReadMemDataByte now used for debugging.
 *
 *  Revision 1.4  2000/12/11 19:03:16  s_a_white
 *  AC99 Update.
 *
 ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "sidtypes.h"
#include "sidendian.h"
#include "sidenv.h"
#include "conf6510.h"
#include "opcodes.h"

#ifdef HAVE_EXCEPTIONS
#   include <new>
#endif

#ifdef MOS6510_STATE_6510
#   include "state6510.h"
#   include "state6510.cpp"
#else

#include "mos6510.h"

// Check to see what type of emulation is required
#ifdef MOS6510_CYCLE_BASED
#   include "cycle_based/mos6510c.i"

#   ifdef MOS6510_SIDPLAY
        // Compile in sidplay code
#       include "cycle_based/sid6510c.i"
#   endif // MOS6510_SIDPLAY
#else
    // Line based emulation code has not been provided
#endif // MOS6510_CYCLE_BASED

void MOS6510::DumpState (void)
{
    uint8_t        opcode, data;
    uint_least16_t operand, address;

    printf(" PC  I  A  X  Y  SP  DR PR NV-BDIZC  Instruction\n");
    printf("%04x ",   instrStartPC);
    printf("%u ",     interrupts.irqs);
    printf("%02x ",   Register_Accumulator);
    printf("%02x ",   Register_X);
    printf("%02x ",   Register_Y);
    printf("01%02x ", endian_16lo8 (Register_StackPointer));
    printf("%02x ",   envReadMemDataByte (0));
    printf("%02x ",   envReadMemDataByte (1));

    if (getFlagN()) printf ("1"); else printf ("0");
    if (getFlagV()) printf ("1"); else printf ("0");
    if (Register_Status & (1 << SR_NOTUSED)) printf ("1"); else printf ("0");
    if (Register_Status & (1 << SR_BREAK))   printf ("1"); else printf ("0");
    if (getFlagD()) printf ("1"); else printf ("0");
    if (getFlagI()) printf ("1"); else printf ("0");
    if (getFlagZ()) printf ("1"); else printf ("0");
    if (getFlagC()) printf ("1"); else printf ("0");

    opcode  = instrOpcode;
    operand = Instr_Operand;
    data    = Cycle_Data;

    switch (opcode)
    {
    case BCCr: case BCSr: case BEQr: case BMIr: case BNEr: case BPLr:
    case BVCr: case BVSr:
        address = (uint_least16_t) (Register_ProgramCounter + (int8_t) operand);
    break;

    default:
        address = Cycle_EffectiveAddress;
    break;
    }

    printf("  %02x ", opcode);

    switch(opcode)
    {
    //Accumulator or Implied addressing
    case ASLn: case LSRn: case ROLn: case RORn:
        printf("      ");
    break;
    //Zero Page Addressing Mode Handler
    case ADCz: case ANDz: case ASLz: case BITz: case CMPz: case CPXz:
    case CPYz: case DCPz: case DECz: case EORz: case INCz: case ISBz:
    case LAXz: case LDAz: case LDXz: case LDYz: case LSRz: case NOPz_:
    case ORAz: case ROLz: case RORz: case SAXz: case SBCz: case SREz:
    case STAz: case STXz: case STYz: case SLOz: case RLAz: case RRAz:
    //ASOz AXSz DCMz INSz LSEz - Optional Opcode Names
        printf("%02x    ", (uint8_t) operand);
        break;
    //Zero Page with X Offset Addressing Mode Handler
    case ADCzx:  case ANDzx: case ASLzx: case CMPzx: case DCPzx: case DECzx:
    case EORzx:  case INCzx: case ISBzx: case LDAzx: case LDYzx: case LSRzx:
    case NOPzx_: case ORAzx: case RLAzx: case ROLzx: case RORzx: case RRAzx:
    case SBCzx:  case SLOzx: case SREzx: case STAzx: case STYzx:
    //ASOzx DCMzx INSzx LSEzx - Optional Opcode Names
        printf("%02x    ", (uint8_t) operand);
        break;
    //Zero Page with Y Offset Addressing Mode Handler
    case LDXzy: case STXzy: case SAXzy: case LAXzy:
    //AXSzx - Optional Opcode Names
        printf("%02x    ", endian_16lo8 (operand));
        break;
    //Absolute Addressing Mode Handler
    case ADCa: case ANDa: case ASLa: case BITa: case CMPa: case CPXa:
    case CPYa: case DCPa: case DECa: case EORa: case INCa: case ISBa:
    case JMPw: case JSRw: case LAXa: case LDAa: case LDXa: case LDYa:
    case LSRa: case NOPa: case ORAa: case ROLa: case RORa: case SAXa:
    case SBCa: case SLOa: case SREa: case STAa: case STXa: case STYa:
    case RLAa: case RRAa:
    //ASOa AXSa DCMa INSa LSEa - Optional Opcode Names
        printf("%02x %02x ", endian_16lo8 (operand), endian_16hi8 (operand));
        break;
    //Absolute With X Offset Addresing Mode Handler
    case ADCax:  case ANDax: case ASLax: case CMPax: case DCPax: case DECax:
    case EORax:  case INCax: case ISBax: case LDAax: case LDYax: case LSRax:
    case NOPax_: case ORAax: case RLAax: case ROLax: case RORax: case RRAax:
    case SBCax:  case SHYax: case SLOax: case SREax: case STAax:
    //ASOax DCMax INSax LSEax SAYax - Optional Opcode Names
        printf("%02x %02x ", endian_16lo8 (operand), endian_16hi8 (operand));
        break;
    //Absolute With Y Offset Addresing Mode Handler
    case ADCay: case ANDay: case CMPay: case DCPay: case EORay: case ISBay:
    case LASay: case LAXay: case LDAay: case LDXay: case ORAay: case RLAay:
    case RRAay: case SBCay: case SHAay: case SHSay: case SHXay: case SLOay:
    case SREay: case STAay:
    //ASOay AXAay DCMay INSax LSEay TASay XASay - Optional Opcode Names
        printf("%02x %02x ", endian_16lo8 (operand), endian_16hi8 (operand));
        break;
    //Immediate and Relative Addressing Mode Handler
    case ADCb: case ANDb: case ANCb_: case ANEb: case ASRb:  case ARRb:

    case CMPb: case CPXb: case CPYb:  case EORb: case LDAb:  case LDXb:
    case LDYb: case LXAb: case NOPb_: case ORAb: case SBCb_: case SBXb:
    //OALb ALRb XAAb - Optional Opcode Names
        printf("%02x    ", endian_16lo8 (operand));
        break;
    case BCCr: case BCSr: case BEQr: case BMIr: case BNEr: case BPLr:
    case BVCr: case BVSr:
        printf("%02x    ", endian_16lo8 (operand));
        break;
    //Indirect Addressing Mode Handler
    case JMPi:
        printf("%02x %02x ", endian_16lo8 (operand), endian_16hi8 (operand));
        break;
    //Indexed with X Preinc Addressing Mode Handler
    case ADCix: case ANDix: case CMPix: case DCPix: case EORix: case ISBix:
    case LAXix: case LDAix: case ORAix: case SAXix: case SBCix: case SLOix:
    case SREix: case STAix: case RLAix: case RRAix:
    //ASOix AXSix DCMix INSix LSEix - Optional Opcode Names
        printf("%02x    ", endian_16lo8 (operand));
        break;
    //Indexed with Y Postinc Addressing Mode Handler
    case ADCiy: case ANDiy: case CMPiy: case DCPiy: case EORiy: case ISBiy:
    case LAXiy: case LDAiy: case ORAiy: case RLAiy: case RRAiy: case SBCiy:
    case SHAiy: case SLOiy: case SREiy: case STAiy:
    //AXAiy ASOiy LSEiy DCMiy INSiy - Optional Opcode Names
        printf("%02x    ", endian_16lo8 (operand));
        break;
    default:
        printf("      ");
        break;
    }

    switch(opcode)
    {
    case ADCb: case ADCz: case ADCzx: case ADCa: case ADCax: case ADCay:
    case ADCix: case ADCiy:
        printf(" ADC"); break;
    case ANCb_:
        printf("*ANC"); break;
    case ANDb: case ANDz: case ANDzx: case ANDa: case ANDax: case ANDay:
    case ANDix: case ANDiy:
        printf(" AND"); break;
    case ANEb: //Also known as XAA
        printf("*ANE"); break;
    case ARRb:
        printf("*ARR"); break;
    case ASLn: case ASLz: case ASLzx: case ASLa: case ASLax:
        printf(" ASL"); break;
    case ASRb: //Also known as ALR
        printf("*ASR"); break;
    case BCCr:
        printf(" BCC"); break;
    case BCSr:
        printf(" BCS"); break;
    case BEQr:
        printf(" BEQ"); break;
    case BITz: case BITa:
        printf(" BIT"); break;
    case BMIr:
        printf(" BMI"); break;
    case BNEr:
        printf(" BNE"); break;
    case BPLr:
        printf(" BPL"); break;
    case BRKn:
        printf(" BRK"); break;
    case BVCr:
        printf(" BVC"); break;
    case BVSr:
        printf(" BVS"); break;
    case CLCn:
        printf(" CLC"); break;
    case CLDn:
        printf(" CLD"); break;
    case CLIn:
        printf(" CLI"); break;
    case CLVn:
        printf(" CLV"); break;
    case CMPb: case CMPz: case CMPzx: case CMPa: case CMPax: case CMPay:
    case CMPix: case CMPiy:
        printf(" CMP"); break;
    case CPXb: case CPXz: case CPXa:
        printf(" CPX"); break;
    case CPYb: case CPYz: case CPYa:
        printf(" CPY"); break;
    case DCPz: case DCPzx: case DCPa: case DCPax: case DCPay: case DCPix:
    case DCPiy: //Also known as DCM
        printf("*DCP"); break;
    case DECz: case DECzx: case DECa: case DECax:
        printf(" DEC"); break;
    case DEXn:
        printf(" DEX"); break;
    case DEYn:
        printf(" DEY"); break;
    case EORb: case EORz: case EORzx: case EORa: case EORax: case EORay:
    case EORix: case EORiy:
        printf(" EOR"); break;
    case INCz: case INCzx: case INCa: case INCax:
        printf(" INC"); break;
    case INXn:
        printf(" INX"); break;
    case INYn:
        printf(" INY"); break;
    case ISBz: case ISBzx: case ISBa: case ISBax: case ISBay: case ISBix:
    case ISBiy: //Also known as INS
        printf("*ISB"); break;
    case JMPw: case JMPi:
        printf(" JMP"); break;
    case JSRw:
        printf(" JSR"); break;
    case LASay:
        printf("*LAS"); break;
    case LAXz: case LAXzy: case LAXa: case LAXay: case LAXix: case LAXiy:
        printf("*LAX"); break;
    case LDAb: case LDAz: case LDAzx: case LDAa: case LDAax: case LDAay:
    case LDAix: case LDAiy:
        printf(" LDA"); break;
    case LDXb: case LDXz: case LDXzy: case LDXa: case LDXay:
        printf(" LDX"); break;
    case LDYb: case LDYz: case LDYzx: case LDYa: case LDYax:
        printf(" LDY"); break;
    case LSRz: case LSRzx: case LSRa: case LSRax: case LSRn:
        printf(" LSR"); break;
    case NOPn_: case NOPb_: case NOPz_: case NOPzx_: case NOPa: case NOPax_:
        if(opcode != NOPn) printf("*");
        else printf(" ");
        printf("NOP"); break;
    case LXAb: //Also known as OAL
        printf("*LXA"); break;
    case ORAb: case ORAz: case ORAzx: case ORAa: case ORAax: case ORAay:
    case ORAix: case ORAiy:
        printf(" ORA"); break;
    case PHAn:
        printf(" PHA"); break;
    case PHPn:
        printf(" PHP"); break;
    case PLAn:
        printf(" PLA"); break;
    case PLPn:
        printf(" PLP"); break;
    case RLAz: case RLAzx: case RLAix: case RLAa: case RLAax: case RLAay:
    case RLAiy:
        printf("*RLA"); break;
    case ROLz: case ROLzx: case ROLa: case ROLax: case ROLn:
        printf(" ROL"); break;
    case RORz: case RORzx: case RORa: case RORax: case RORn:
        printf(" ROR"); break;
    case RRAa: case RRAax: case RRAay: case RRAz: case RRAzx: case RRAix:
    case RRAiy:
        printf("*RRA"); break;
    case RTIn:
        printf(" RTI"); break;
    case RTSn:
        printf(" RTS"); break;
    case SAXz: case SAXzy: case SAXa: case SAXix: //Also known as AXS
        printf("*SAX"); break;
    case SBCb_:
        if(opcode != SBCb) printf("*");
        else printf(" ");
        printf ("SBC"); break;
    case SBCz: case SBCzx: case SBCa: case SBCax: case SBCay: case SBCix:
    case SBCiy:
        printf(" SBC"); break;
    case SBXb:
        printf("*SBX"); break;
    case SECn:
        printf(" SEC"); break;
    case SEDn:
        printf(" SED"); break;
    case SEIn:
        printf(" SEI"); break;
    case SHAay: case SHAiy: //Also known as AXA
        printf("*SHA"); break;
    case SHSay: //Also known as TAS
        printf("*SHS"); break;
    case SHXay: //Also known as XAS
        printf("*SHX"); break;
    case SHYax: //Also known as SAY
        printf("*SHY"); break;
    case SLOz: case SLOzx: case SLOa: case SLOax: case SLOay: case SLOix:
    case SLOiy: //Also known as ASO
        printf("*SLO"); break;
    case SREz: case SREzx: case SREa: case SREax: case SREay: case SREix:
    case SREiy: //Also known as LSE
        printf("*SRE"); break;
    case STAz: case STAzx: case STAa: case STAax: case STAay: case STAix:
    case STAiy:
        printf(" STA"); break;
    case STXz: case STXzy: case STXa:
        printf(" STX"); break;
    case STYz: case STYzx: case STYa:
        printf(" STY"); break;
    case TAXn:
        printf(" TAX"); break;
    case TAYn:
        printf(" TAY"); break;
    case TSXn:
        printf(" TSX"); break;
    case TXAn:
        printf(" TXA"); break;
    case TXSn:
        printf(" TXS"); break;
    case TYAn:
        printf(" TYA"); break;
    default:
        printf("*HLT"); break;
    }

    switch(opcode)
    {
    //Accumulator or Implied addressing
    case ASLn: case LSRn: case ROLn: case RORn:
        printf("n  A");
    break;

    //Zero Page Addressing Mode Handler
    case ADCz: case ANDz: case ASLz: case BITz: case CMPz: case CPXz:
    case CPYz: case DCPz: case DECz: case EORz: case INCz: case ISBz:
    case LAXz: case LDAz: case LDXz: case LDYz: case LSRz: case ORAz:

    case ROLz: case RORz: case SBCz: case SREz: case SLOz: case RLAz:
    case RRAz:
    //ASOz AXSz DCMz INSz LSEz - Optional Opcode Names
        printf("z  %02x {%02x}", (uint8_t) operand, data);
    break;
    case SAXz: case STAz: case STXz: case STYz:
#ifdef MOS6510_DEBUG
    case NOPz_:
#endif
        printf("z  %02x", endian_16lo8 (operand));
    break;

    //Zero Page with X Offset Addressing Mode Handler
    case ADCzx: case ANDzx: case ASLzx: case CMPzx: case DCPzx: case DECzx:
    case EORzx: case INCzx: case ISBzx: case LDAzx: case LDYzx: case LSRzx:
    case ORAzx: case RLAzx: case ROLzx: case RORzx: case RRAzx: case SBCzx:
    case SLOzx: case SREzx:
    //ASOzx DCMzx INSzx LSEzx - Optional Opcode Names
        printf("zx %02x,X", endian_16lo8 (operand));
        printf(" [%04x]{%02x}", address, data);
    break;
    case STAzx: case STYzx:
#ifdef MOS6510_DEBUG
    case NOPzx_:
#endif
        printf("zx %02x,X", endian_16lo8 (operand));
        printf(" [%04x]", address);
    break;

    //Zero Page with Y Offset Addressing Mode Handler
    case LAXzy: case LDXzy:
    //AXSzx - Optional Opcode Names
        printf("zy %02x,Y", endian_16lo8 (operand));
        printf(" [%04x]{%02x}", address, data);
    break;
    case STXzy: case SAXzy:
        printf("zy %02x,Y", endian_16lo8 (operand));
        printf(" [%04x]", address);
    break;

    //Absolute Addressing Mode Handler
    case ADCa: case ANDa: case ASLa: case BITa: case CMPa: case CPXa:
    case CPYa: case DCPa: case DECa: case EORa: case INCa: case ISBa:
    case LAXa: case LDAa: case LDXa: case LDYa: case LSRa: case ORAa:
    case ROLa: case RORa: case SBCa: case SLOa: case SREa: case RLAa:
    case RRAa:
    //ASOa AXSa DCMa INSa LSEa - Optional Opcode Names
        printf("a  %04x {%02x}", operand, data);
    break;
    case SAXa: case STAa: case STXa: case STYa:
#ifdef MOS6510_DEBUG
    case NOPa:
#endif
        printf("a  %04x", operand);
    break;
    case JMPw: case JSRw:
        printf("w  %04x", operand);
    break;

    //Absolute With X Offset Addresing Mode Handler
    case ADCax: case ANDax: case ASLax: case CMPax: case DCPax: case DECax:
    case EORax: case INCax: case ISBax: case LDAax: case LDYax: case LSRax:
    case ORAax: case RLAax: case ROLax: case RORax: case RRAax: case SBCax:
    case SLOax: case SREax:
    //ASOax DCMax INSax LSEax SAYax - Optional Opcode Names
        printf("ax %04x,X", operand);
        printf(" [%04x]{%02x}", address, data);
    break;
    case SHYax: case STAax:
#ifdef MOS6510_DEBUG
    case NOPax_:
#endif
        printf("ax %04x,X", operand);
        printf(" [%04x]", address);
    break;

    //Absolute With Y Offset Addresing Mode Handler
    case ADCay: case ANDay: case CMPay: case DCPay: case EORay: case ISBay:
    case LASay: case LAXay: case LDAay: case LDXay: case ORAay: case RLAay:
    case RRAay: case SBCay: case SHSay: case SLOay: case SREay:
    //ASOay AXAay DCMay INSax LSEay TASay XASay - Optional Opcode Names
        printf("ay %04x,Y", operand);
        printf(" [%04x]{%02x}", address, data);
    break;
    case SHAay: case SHXay: case STAay:
        printf("ay %04x,Y", operand);
        printf(" [%04x]", address);
    break;

    //Immediate Addressing Mode Handler
    case ADCb: case ANDb: case ANCb_: case ANEb: case ASRb:  case ARRb:
    case CMPb: case CPXb: case CPYb:  case EORb: case LDAb:  case LDXb:
    case LDYb: case LXAb: case ORAb: case SBCb_: case SBXb:
    //OALb ALRb XAAb - Optional Opcode Names
#ifdef MOS6510_DEBUG
    case NOPb_:
#endif
        printf("b  #%02x", endian_16lo8 (operand));
    break;

    //Relative Addressing Mode Handler
    case BCCr: case BCSr: case BEQr: case BMIr: case BNEr: case BPLr:
    case BVCr: case BVSr:
        printf("r  #%02x", endian_16lo8 (operand));
        printf(" [%04x]", address);
    break;

    //Indirect Addressing Mode Handler
    case JMPi:
        printf("i  (%04x)", operand);
        printf(" [%04x]", address);
    break;

    //Indexed with X Preinc Addressing Mode Handler
    case ADCix: case ANDix: case CMPix: case DCPix: case EORix: case ISBix:
    case LAXix: case LDAix: case ORAix: case SBCix: case SLOix: case SREix:
    case RLAix: case RRAix:
    //ASOix AXSix DCMix INSix LSEix - Optional Opcode Names
        printf("ix (%02x,X)", endian_16lo8 (operand));
        printf(" [%04x]{%02x}", address, data);
    break;
    case SAXix: case STAix:
        printf("ix (%02x,X)", endian_16lo8 (operand));
        printf(" [%04x]", address);
    break;

    //Indexed with Y Postinc Addressing Mode Handler
    case ADCiy: case ANDiy: case CMPiy: case DCPiy: case EORiy: case ISBiy:
    case LAXiy: case LDAiy: case ORAiy: case RLAiy: case RRAiy: case SBCiy:
    case SLOiy: case SREiy:
    //AXAiy ASOiy LSEiy DCMiy INSiy - Optional Opcode Names
        printf("iy (%02x),Y", endian_16lo8 (operand));
        printf(" [%04x]{%02x}", address, data);
    break;
    case SHAiy: case STAiy:
        printf("iy (%02x),Y", endian_16lo8 (operand));
        printf(" [%04x]", address);
    break;

    default:
    break;
    }

    printf ("\n\n");
    fflush (stdout);
}

#endif // MOS6510_STATE_6510

