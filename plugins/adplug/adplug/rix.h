/*
 * Adplug - Replayer for many OPL2/OPL3 audio file formats.
 * Copyright (C) 1999 - 2006 Simon Peter, <dn.tlp@gmx.net>, et al.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * rix.h - Softstar RIX OPL Format Player by palxex <palxex.ys168.com>
 *                                           BSPAL <BSPAL.ys168.com>
 */

#include "player.h"

class CrixPlayer: public CPlayer
{
 public:
  static CPlayer *factory(Copl *newopl);

  CrixPlayer(Copl *newopl);
  ~CrixPlayer();

  bool load(const std::string &filename, const CFileProvider &fp);
  bool update();
  void rewind(int subsong);
  float getrefresh();
  unsigned int getsubsongs();
  unsigned int getsubsong();

  std::string gettype()
    { return std::string("Softstar RIX OPL Music Format"); };

 protected:	
  int song = 0;
  typedef struct {
    uint8_t v[14];
  } ADDT;

  int flag_mkf;
  uint8_t *file_buffer;
  uint8_t *rix_buf;  /* rix files' f_buffer */
  uint16_t f_buffer[300];//9C0h-C18h
  uint16_t a0b0_data2[11];
  uint8_t a0b0_data3[18];
  uint8_t a0b0_data4[18];
  uint8_t a0b0_data5[96];
  uint8_t addrs_head[96];
  uint16_t insbuf[28];
  uint16_t displace[11];
  ADDT reg_bufs[18];
  uint32_t pos,length;
  uint8_t index;

  static const uint8_t adflag[18];
  static const uint8_t reg_data[18];
  static const uint8_t ad_C0_offs[18];
  static const uint8_t modify[28];
  static const uint8_t bd_reg_data[124];
  uint8_t for40reg[18];
  static const uint16_t mus_time;
  uint32_t I,T;
  uint16_t mus_block;
  uint16_t ins_block;
  uint8_t rhythm;
  uint8_t music_on;
  uint8_t pause_flag;
  uint16_t band;
  uint8_t band_low;
  uint16_t e0_reg_flag;
  uint8_t bd_modify;
  int sustain;
  int play_end;

#define ad_08_reg() ad_bop(8,0)    /**/
  inline void ad_20_reg(uint16_t);              /**/
  inline void ad_40_reg(uint16_t);              /**/
  inline void ad_60_reg(uint16_t);              /**/
  inline void ad_80_reg(uint16_t);              /**/
  inline void ad_a0b0_reg(uint16_t);            /**/
  inline void ad_a0b0l_reg(uint16_t,uint16_t,uint16_t); /**/
  inline void ad_a0b0l_reg_(uint16_t,uint16_t,uint16_t); /**/
  inline void ad_bd_reg();                  /**/
  inline void ad_bop(uint16_t,uint16_t);                     /**/
  inline void ad_C0_reg(uint16_t);              /**/
  inline void ad_E0_reg(uint16_t);              /**/
  inline uint16_t ad_initial();                 /**/
  inline uint16_t ad_test();                    /**/
  inline void crc_trans(uint16_t,uint16_t);         /**/
  inline void data_initial();               /* done */
  inline void init();                       /**/
  inline void ins_to_reg(uint16_t,uint16_t*,uint16_t);  /**/
  inline void int_08h_entry();    /**/
  inline void music_ctrl();                 /**/
  inline void Pause();                      /**/
  inline void prepare_a0b0(uint16_t,uint16_t);      /**/
  inline void rix_90_pro(uint16_t);             /**/
  inline void rix_A0_pro(uint16_t,uint16_t);        /**/
  inline void rix_B0_pro(uint16_t,uint16_t);        /**/
  inline void rix_C0_pro(uint16_t,uint16_t);        /**/
  inline void rix_get_ins();                /**/
  inline uint16_t rix_proc();                   /**/
  inline void set_new_int();
  inline void switch_ad_bd(uint16_t);           /**/
};
