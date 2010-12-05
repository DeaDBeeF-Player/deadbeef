/*
 Copyright (C) 2000-2005  Heikki Orsila
 Copyright (C) 2000-2005  Michael Doering

 This module is dual licensed under the GNU GPL and the Public Domain.
 Hence you may use _this_ module (not another code module) in any way you
 want in your projects.

 About security:

 This module tries to avoid any buffer overruns by not copying anything but
 hard coded strings (such as "FC13"). This doesn't
 copy any data from modules to program memory. Any memory writing with
 non-hard-coded data is an error by assumption. This module will only
 determine the format of a given module.

 Occasional memory reads over buffer ranges can occur, but they will of course
 be fixed when spotted :P The worst that can happen with reading over the
 buffer range is a core dump :)
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <uadeutils.h>
#include <amifilemagic.h>

#define FILEMAGIC_DEBUG 0

#if FILEMAGIC_DEBUG
#define amifiledebug(fmt, args...) do { fprintf(stderr, "%s:%d: %s: " fmt, __FILE__, __LINE__, __func__, ## args); } while(0)
#else
#define amifiledebug(fmt, args...) 
#endif

#define WAV_HEADER_LEN 44

enum {
  MOD_UNDEFINED = 0,
  MOD_SOUNDTRACKER25_NOISETRACKER10,
  MOD_NOISETRACKER12,
  MOD_NOISETRACKER20,
  MOD_STARTREKKER4,
  MOD_STARTREKKER8,
  MOD_AUDIOSCULPTURE4,
  MOD_AUDIOSCULPTURE8,
  MOD_PROTRACKER,
  MOD_FASTTRACKER,
  MOD_NOISETRACKER,
  MOD_PTK_COMPATIBLE,
  MOD_SOUNDTRACKER24
};


#define S15_HEADER_LENGTH 600
#define S31_HEADER_LENGTH 1084


static int chk_id_offset(unsigned char *buf, int bufsize,
			 const char *patterns[], int offset, char *pre);


/* Do not use '\0'. They won't work in patterns */
static const char *offset_0000_patterns[] = {
  /* ID: Prefix: Desc: */
  "DIGI Booster", "DIGI",	/* Digibooster */
  "OKTASONG", "OKT",		/* Oktalyzer */
  "SYNTRACKER", "SYNMOD",	/* Syntracker */
  "OBISYNTHPACK", "OSP",	/* Synthpack */
  "SOARV1.0", "SA",		/* Sonic Arranger */
  "AON4", "AON4",              /* Art Of Noise (4ch) */
  "AON8", "AON8",              /* Art Of Noise (8ch) */
  "ARP.", "MTP2",       	/* HolyNoise / Major Tom */
  "AmBk", "ABK",		/* Amos ABK */
  "FUCO", "BSI",		/* FutureComposer BSI */
  "MMU2", "DSS",		/* DSS */
  "GLUE", "GLUE",		/* GlueMon */
  "ISM!", "IS",			/* In Stereo */
  "IS20", "IS20",		/* In Stereo 2 */
  "SMOD", "FC13",		/* FC 1.3 */
  "FC14", "FC14",		/* FC 1.4 */
  "MMDC", "MMDC",		/* Med packer */
  "MSOB", "MSO",		/* Medley */
  "MODU", "NTP",		/* Novotrade */
/* HIPPEL-ST CONFLICT: "COSO", "SOC",*/		/* Hippel Coso */
  "BeEp", "JAM",		/* Jamcracker */
  "ALL ", "DM1",		/* Deltamusic 1 */
  "YMST", "YM",			/* MYST ST-YM */
  "AMC ", "AMC",		/* AM-Composer */
  "P40A", "P40A",		/* The Player 4.0a */
  "P40B", "P40B",		/* The Player 4.0b */
  "P41A", "P41A",		/* The Player 4.1a */
  "P50A", "P50A",               /* The Player 5.0a */
  "P60A", "P60A",		/* The Player 6.0a */
  "P61A", "P61A",		/* The Player 6.1a */
  "SNT!", "PRU2",		/* Prorunner 2 */
  "MEXX_TP2", "TP2",		/* Tracker Packer 2 */
  "CPLX_TP3", "TP3",		/* Tracker Packer 3 */
  "MEXX", "TP1",		/* Tracker Packer 2 */
  "PM40", "PM40",		/* Promizer 4.0 */
  "FC-M", "FC-M",		/* FC-M */
  "E.M.S. V6.", "EMSV6",	/* EMS version 6 */
  "MCMD", "MCMD_org",		/* 0x00 MCMD format */
  "STP3", "STP3",		/* Soundtracker Pro 2 */
  "MTM", "MTM",			/* Multitracker */
  "Extended Module:", "XM",	/* Fasttracker2 */
  "MLEDMODL", "ML",		/* Musicline Editor */
  "FTM", "FTM",			/* Face The Music */
  "MXTX", "MXTX",		/* Maxtrax*/
  "M1.0", "FUZZ",		/* Fuzzac*/
  "MSNG", "TPU",		/* Dirk Bialluch*/
  "YM!", "",                    /* stplay -- intentionally sabotaged */
  "ST1.2 ModuleINFO", "",       /* Startrekker AM .NT -- intentionally sabotaged */
  "AudioSculpture10", "",       /* Audiosculpture .AS -- intentionally sabotaged */
  NULL, NULL
};

static const char *offset_0024_patterns[] = {
  /* ID: Prefix: Desc: */
  "UNCLEART", "DL",		/* Dave Lowe WT */
  "DAVELOWE", "DL_deli",	/* Dave Lowe Deli */
  "J.FLOGEL", "JMF",		/* Janko Mrsic-Flogel */
  "BEATHOVEN", "BSS",		/* BSS */
  "FREDGRAY", "GRAY",		/* Fred Gray */
  "H.DAVIES", "HD",		/* Howie Davies */
  "RIFFRAFF", "RIFF",		/* Riff Raff */
  "!SOPROL!", "SPL",		/* Soprol */
  "F.PLAYER", "FP",		/* F.Player */
  "S.PHIPPS", "CORE",		/* Core Design */
  "DAGLISH!", "BDS",		/* Benn Daglish */
  NULL, NULL
};


/* check for 'pattern' in 'buf'.
   the 'pattern' must lie inside range [0, maxlen) in the buffer.
   returns true if pattern is at buf[offset], otherwrise false
 */
static int patterntest(const unsigned char *buf, const char *pattern,
		       int offset, int bytes, int maxlen)
{
  if ((offset + bytes) <= maxlen)
    return (memcmp(buf + offset, pattern, bytes) == 0) ? 1 : 0;
  return 0;
}


static int tronictest(unsigned char *buf, size_t bufsize)
{
  size_t a = read_be_u16(&buf[0x02]) + read_be_u16(&buf[0x06]) +
             read_be_u16(&buf[0x0a]) + read_be_u16(&buf[0x0e]) + 0x10;

  if (((a + 2) >= bufsize) || (a & 1))
    return 0;			/* size  & btst #0, d1; */

  a = read_be_u16(&buf[a]) + a;
  if (((a + 8) >= bufsize) || (a & 1))
    return 0;			/*size & btst #0,d1 */

  if (read_be_u32(&buf[a + 4]) != 0x5800b0)
    return 0;

  amifiledebug("tronic recognized\n");

  return 1;
}

static int tfmxtest(unsigned char *buf, size_t bufsize, char *pre)
{
  if (bufsize <= 0x208)
    return 0;

  if (strncmp((char *) buf, "TFHD", 4) == 0) {
    if (buf[0x8] == 0x01) {
      strcpy(pre, "TFHD1.5");	/* One File TFMX format by Alexis NASR */
      return 1;
    } else if (buf[0x8] == 0x02) {
      strcpy(pre, "TFHDPro");
      return 1;
    } else if (buf[0x8] == 0x03) {
      strcpy(pre, "TFHD7V");
      return 1;
    }
  }

  if (strncasecmp((char *) buf, "TFMX", 4) == 0) {
    if (strncmp((char *) &buf[4], "-SONG", 5) == 0 ||
	strncmp((char *) &buf[4], "_SONG ", 6) == 0 ||
	strncasecmp((char *) &buf[4], "SONG", 4) == 0 ||
	buf[4] == 0x20) {
      strcpy(pre, "MDAT");	/*default TFMX: TFMX Pro */

      if (strncmp((char *) &buf[10], "by", 2) == 0 ||
	  strncmp((char *) &buf[16], "  ", 2) == 0 ||
	  strncmp((char *) &buf[16], "(Empty)", 7) == 0 ||
	  /* Lethal Zone */
	  (buf[16] == 0x30 && buf[17] == 0x3d) ||
	  (buf[4] == 0x20)){ 

	if (read_be_u32(&buf[464]) == 0x00000000) {
	  uint16_t x = read_be_u16(&buf[14]);
	  if ((x != 0x0e60) || /* z-out title */
	      (x == 0x0860 && bufsize > 4645 && read_be_u16(&buf[4644]) != 0x090c) || /* metal law */
	      (x == 0x0b20 && bufsize > 5121 && read_be_u16(&buf[5120]) != 0x8c26) || /* bug bomber */
	      (x == 0x0920 && bufsize > 3977 && read_be_u16(&buf[3876]) != 0x9305)) { /* metal preview */
	    strcpy(pre, "TFMX1.5");	/*TFMX 1.0 - 1.6 */
	  }
	}
	return 1;
	
      } else if (((buf[0x0e] == 0x08 && buf[0x0f] == 0xb0) &&	/* BMWi */
		  (buf[0x140] == 0x00 && buf[0x141] == 0x0b) &&	/*End tackstep 1st subsong */
		  (buf[0x1d2] == 0x02 && buf[0x1d3] == 0x00) &&	/*Trackstep datas */
		  (buf[0x200] == 0xff && buf[0x201] == 0x00 &&	/*First effect */
		   buf[0x202] == 0x00 && buf[0x203] == 0x00 &&
		   buf[0x204] == 0x01 && buf[0x205] == 0xf4 &&
		   buf[0x206] == 0xff && buf[0x207] == 0x00)) ||
		 ((buf[0x0e] == 0x0A && buf[0x0f] == 0xb0) && /* B.C Kid */
		  (buf[0x140] == 0x00 && buf[0x141] == 0x15) && /*End tackstep 1st subsong */
		  (buf[0x1d2] == 0x02 && buf[0x1d3] == 0x00) && /*Trackstep datas */
		  (buf[0x200] == 0xef && buf[0x201] == 0xfe &&	/*First effect */
		   buf[0x202] == 0x00 && buf[0x203] == 0x03 &&
		   buf[0x204] == 0x00 && buf[0x205] == 0x0d &&
		   buf[0x206] == 0x00 && buf[0x207] == 0x00))) {
	strcpy(pre, "TFMX7V");	/* "special cases TFMX 7V */
	return 1;

      } else {
	int e, i, s, t;

	/* Trackstep datas offset */
	s = read_be_u32(&buf[0x1d0]);
	if (s == 0x00000000) {
	  /* unpacked */
	  s = 0x00000800;
	}

	for (i = 0; i < 0x3d; i += 2) {
	  if (read_be_u16(&buf[0x140 + i]) != 0x0000) { /* subsong */
	    /* Start of subsongs Trackstep data :) */
	    t = read_be_u16(&buf[0x100 + i]) * 16 + s;
	    /* End of subsongs Trackstep data :) */
	    e = read_be_u16(&buf[0x140 + i]) * 16 + s;
	    if (e < bufsize) {
	      for (; t < e && (t + 6) < bufsize; t += 2) {
		if (read_be_u16(&buf[t]) == 0xeffe &&
		    read_be_u32(&buf[t + 2]) == 0x0003ff00 &&
		    buf[t + 6] == 0x00) {
		  strcpy(pre, "TFMX7V");	/*TFMX 7V */
		  return 1;
		}
	      }
	    }
	  }
	}
      }
    }
  }
  return 0;
}

/* Calculate Module length:	Just need at max 1084	*/
/*				data in buf for a 	*/
/*				succesful calculation	*/
/* returns:				 		*/
/* 		 -1 for no mod				*/
/*		 1 for a mod with good length		*/
static size_t modlentest(unsigned char *buf, size_t bufsize, size_t filesize,
			 int header)
{
  int i;
  int no_of_instr;
  int smpl = 0;
  int plist;
  int maxpattern = 0;

  if (header > bufsize)
    return -1;			/* no mod */

  if (header == S15_HEADER_LENGTH)   {
    no_of_instr = 15;
    plist = header - 128;
  } else if (header == S31_HEADER_LENGTH) {
    no_of_instr = 31;
    plist = header - 4 - 128;
  } else {
    return -1;
  }

  for (i = 0; i < 128; i++) {
    if (buf[plist + i] > maxpattern)
      maxpattern = buf[plist + i];
  }

  if (maxpattern > 100)
    return -1;

  for (i = 0; i < no_of_instr; i++)
    smpl += 2 * read_be_u16(&buf[42 + i * 30]);	/* add sample length in bytes*/

  return header + (maxpattern + 1) * 1024 + smpl;
}


static void modparsing(unsigned char *buf, size_t bufsize, size_t header, int max_pattern, int pfx[], int pfxarg[])
{
  int offset;
  int i, j, fx;
  unsigned char fxarg;
  
  for (i = 0; i < max_pattern; i++) {
    for (j = 0; j < 256; j++) {
      offset = header + i * 1024 + j * 4;

      if ((offset + 4) > bufsize)
	return;

      fx = buf[offset + 2] & 0x0f;
      fxarg = buf[offset + 3];
      
      if (fx == 0) {
	if (fxarg != 0 )
	  pfx[fx] += 1;
	pfxarg[fx] = (pfxarg[fx] > fxarg) ? pfxarg[fx] : fxarg;

      } else if (1 <= fx && fx <= 13) {
	pfx[fx] +=1;
	pfxarg[fx] = (pfxarg[fx] > fxarg) ? pfxarg[fx] : fxarg;

      } else if (fx == 14) {
	pfx[((fxarg >> 4) & 0x0f) + 16] +=1;

      } else if (fx == 15) {
	if (fxarg > 0x1f)
	  pfx[14] +=1;
	else
	  pfx[15] +=1;
	pfxarg[15] = (pfxarg[15] > fxarg) ? pfxarg[15] : fxarg;
      }
    }
  }

}


static int mod32check(unsigned char *buf, size_t bufsize, size_t realfilesize,
		      const char *path, int verbose)
{
  /* mod patterns at file offset 0x438 */
  char *mod_patterns[] = { "M.K.", ".M.K", NULL};
  /* startrekker patterns at file offset 0x438 */
  char *startrekker_patterns[] = { "FLT4", "FLT8", "EXO4", "EXO8", NULL};

  int max_pattern = 0;
  int i, j, t, ret;
  int pfx[32];
  int pfxarg[32];

  /* instrument var */
  int vol, slen, srep, sreplen;

  int has_slen_sreplen_zero = 0; /* sreplen empty of non looping instrument */
  int no_slen_sreplen_zero = 0; /* sreplen */

  int has_slen_sreplen_one = 0;
  int no_slen_sreplen_one = 0;

  int no_slen_has_volume = 0;
  int finetune_used = 0;

  size_t calculated_size;

  /* returns:	 0 for undefined                            */
  /* 		 1 for a Soundtracker2.5/Noisetracker 1.0   */
  /*		 2 for a Noisetracker 1.2		    */
  /*		 3 for a Noisetracker 2.0		    */
  /*		 4 for a Startrekker 4ch		    */
  /*		 5 for a Startrekker 8ch		    */
  /*		 6 for Audiosculpture 4 ch/fm		    */
  /*		 7 for Audiosculpture 8 ch/fm		    */
  /*		 8 for a Protracker 			    */
  /*		 9 for a Fasttracker			    */
  /*		 10 for a Noisetracker (M&K!)		    */
  /*		 11 for a PTK Compatible		    */
  /*		 12 for a Soundtracker 31instr. with repl in bytes	    */

  /* Special cases first */
  if (patterntest(buf, "M&K!", (S31_HEADER_LENGTH - 4), 4, bufsize))
    return MOD_NOISETRACKER;	/* Noisetracker (M&K!) */
  
  if (patterntest(buf, "M!K!", (S31_HEADER_LENGTH - 4), 4, bufsize))
    return MOD_PROTRACKER;		/* Protracker (100 patterns) */

  if (patterntest(buf, "N.T.", (S31_HEADER_LENGTH - 4), 4, bufsize))
    return MOD_NOISETRACKER20;		/* Noisetracker2.x */

  for (i = 0; startrekker_patterns[i]; i++) {
    if (patterntest(buf, startrekker_patterns[i], (S31_HEADER_LENGTH - 4), 4, bufsize)) {
      t = 0;
      for (j = 0; j < 30 * 0x1e; j = j + 0x1e) {
	if (buf[0x2a + j] == 0 && buf[0x2b + j] == 0 && buf[0x2d + j] != 0) {
	  t = t + 1;		/* no of AM instr. */
	}
      }
      if (t > 0) {
	if (buf[0x43b] == '4'){
	  ret = MOD_AUDIOSCULPTURE4;	/* Startrekker 4 AM / ADSC */
	} else { 		
	  ret = MOD_AUDIOSCULPTURE8;	/* Startrekker 8 AM / ADSC */	
	}
      } else {
	if (buf[0x43b] == '4'){
	  ret = MOD_STARTREKKER4;	/* Startrekker 4ch */
	} else { 		
	  ret = MOD_STARTREKKER8;	/* Startrekker 8ch */	
	}
      }
      return ret;
    }
  }

  calculated_size = modlentest(buf, bufsize, realfilesize, S31_HEADER_LENGTH);

  if (calculated_size == -1)
  return MOD_UNDEFINED;


  for (i = 0; mod_patterns[i]; i++) {
    if (patterntest(buf, mod_patterns[i], S31_HEADER_LENGTH - 4, 4, bufsize)) {
      /* seems to be a generic M.K. MOD                              */
      /* only spam filesize message when it's a tracker module */

    if (calculated_size != realfilesize) {
      fprintf(stderr, "uade: file size is %zd but calculated size for a mod file is %zd (%s).\n", realfilesize, calculated_size, path);
    }

    if (calculated_size > realfilesize) {
        fprintf(stderr, "uade: file is truncated and won't get played (%s)\n", path);
      return MOD_UNDEFINED;
    }

    if (calculated_size < realfilesize) {
        fprintf(stderr, "uade: file has trailing garbage behind the actual module data. Please fix it. (%s)\n", path);
    }

    /* parse instruments */
    for (i = 0; i < 31; i++) {
      vol = buf[45 + i * 30];
      slen = ((buf[42 + i * 30] << 8) + buf[43 + i * 30]) * 2;
      srep = ((buf[46 + i * 30] << 8) + buf[47 + i * 30]) *2;
      sreplen = ((buf[48 + i * 30] << 8) + buf[49 + i * 30]) * 2;
      /* fprintf (stderr, "%d, slen: %d, %d (srep %d, sreplen %d), vol: %d\n",i, slen, srep+sreplen,srep, sreplen, vol); */

      if (vol > 64)
        return MOD_UNDEFINED;

      if (buf[44 + i * 30] != 0) {
        if (buf[44+i*30] > 15) {
  	  return MOD_UNDEFINED;
        } else {
	  finetune_used++;
        }
      }

      if (slen > 0 && (srep + sreplen) > slen) {
        /* Old Noisetracker /Soundtracker with repeat offset in bytes */
        return MOD_SOUNDTRACKER24;
      }

      if (srep == 0) {
        if (slen > 0) {
	  if (sreplen == 2){
	    has_slen_sreplen_one++;
	  }
	  if (sreplen == 0){
	    has_slen_sreplen_zero++;
	  }
        } else {
	  if (sreplen > 0){
	    no_slen_sreplen_one++;
	  } else {
	    no_slen_sreplen_zero++;
	  }
	  if (vol > 0)
	    no_slen_has_volume++;
        }
       }
     }

      for (i = 0; i < 128; i++) {
	if (buf[1080 - 130 + 2 + i] > max_pattern)
	  max_pattern = buf[1080 - 130 + 2 + i];
      }
      
      if (max_pattern > 100) {
	/* pattern number can only be  0 <-> 100 for mod*/
	return MOD_UNDEFINED;
      }

      memset (pfx, 0, sizeof (pfx));
      memset (pfxarg, 0, sizeof (pfxarg));
      modparsing(buf, bufsize, S31_HEADER_LENGTH-4, max_pattern, pfx, pfxarg);

      /* and now for let's see if we can spot the mod */

      /* FX used:					  		     */
      /* DOC Soundtracker 2.x(2.5):	0,1,2(3,4)	    a,b,c,d,e,f	     */
      /* Noisetracker 1.x:		0,1,2,3,4	    a,b,c,d,e,f      */
      /* Noisetracker 2.x:		0,1,2,3,4           a,b,c,d,e,f      */
      /* Protracker:			0,1,2,3,4,5,6,7   9,a,b,c,d,e,f	+e## */
      /* PC tracker:			0,1,2,3,4,5,6,7,8,9,a,b,c,d,e,f +e## */

      for (j = 17; j <= 31; j++) {
	if (pfx[j] != 0 || finetune_used >0) /* Extended fx used */ {
	  if (buf[0x3b7] != 0x7f && buf[0x3b7] != 0x78) {
	    return MOD_FASTTRACKER; /* Definetely Fasttracker*/
	  } else {
	    return MOD_PROTRACKER; /* Protracker*/
	  }
	}
      }

      if ((buf[0x3b7] == 0x7f) && 
	  (has_slen_sreplen_zero <= has_slen_sreplen_one) &&
	  (no_slen_sreplen_zero <=no_slen_sreplen_one))
	return MOD_PROTRACKER; /* Protracker */

      if (buf[0x3b7] >0x7f)
	return MOD_PTK_COMPATIBLE; /* Protracker compatible */

      if ((buf[0x3b7] == 0) && 
	  (has_slen_sreplen_zero >  has_slen_sreplen_one) &&
	  (no_slen_sreplen_zero > no_slen_sreplen_one)){
	if (pfx[0x10] == 0) {
	  /* probl. Fastracker or Protracker compatible */
	  return MOD_PTK_COMPATIBLE;
	}
	  /* FIXME: Investigate
	  else {
	  return MOD_PROTRACKER; // probl. Protracker
	  } */
      }
	    
      if (pfx[0x05] != 0 || pfx[0x06] != 0 || pfx[0x07] != 0 ||
	  pfx[0x09] != 0) {
	/* Protracker compatible */
	return MOD_PTK_COMPATIBLE;
      }

      if ((buf[0x3b7] >0 && buf[0x3b7] <= buf[0x3b6]) && 
	  (has_slen_sreplen_zero <= has_slen_sreplen_one) &&
	  (no_slen_sreplen_zero == 1) &&
	  (no_slen_sreplen_zero <= no_slen_sreplen_one))    
	return MOD_NOISETRACKER12; // Noisetracker 1.2

      if ((buf[0x3b7] <0x80) && 
	  (has_slen_sreplen_zero <= has_slen_sreplen_one) &&
	  (no_slen_sreplen_zero <=no_slen_sreplen_one))    
	return MOD_NOISETRACKER20; // Noisetracker 2.x

      if ((buf[0x3b7] <0x80) && 
	  (pfx[0x0e] ==0) &&
	  (has_slen_sreplen_zero <= has_slen_sreplen_one) &&
	  (no_slen_sreplen_zero >=no_slen_sreplen_one))    
	return MOD_SOUNDTRACKER25_NOISETRACKER10; // Noisetracker 1.x

      return MOD_PTK_COMPATIBLE; // Protracker compatible
    }
  }

  return MOD_UNDEFINED;
}


static int mod15check(unsigned char *buf, size_t bufsize, size_t realfilesize,
		      const char *path)
/* pattern parsing based on Sylvain 'Asle' Chipaux'	*/
/* Modinfo-V2						*/
/*							*/
/* returns:	 0 for an undefined mod 		*/
/* 		 1 for a DOC Soundtracker mod		*/
/*		 2 for a Ultimate ST mod		*/
/*		 3 for a Mastersoundtracker		*/
/*		 4 for a SoundtrackerV2.0 -V4.0		*/
{
  int i = 0, j = 0;
  int slen = 0;
  int srep = 0;
  int sreplen = 0;
  int vol = 0;

  int noof_slen_zero_sreplen_zero = 0;
  int noof_slen_zero_vol_zero = 0;
  int srep_bigger_slen = 0;
  int srep_bigger_ffff = 0;
  int st_xy = 0;
  
  int max_pattern = 1;
  int pfx[32];
  int pfxarg[32];

  size_t calculated_size;

  /* sanity checks */
  if (bufsize < 0x1f3)
    return 0;			/* file too small */

  if (bufsize < 2648+4 || realfilesize <2648+4) /* size 1 pattern + 1x 4 bytes Instrument :) */
    return 0;

  calculated_size = modlentest(buf, bufsize, realfilesize, S15_HEADER_LENGTH);
  if (calculated_size == -1)
    return 0; /* modlentest failed */

  if (calculated_size != realfilesize) {
      return 0 ;
    }

  if (calculated_size > realfilesize) {
      fprintf(stderr, "uade: file is truncated and won't get played (%s)\n", path);
      return 0 ;
    }



  /* check for 15 instruments */
  if (buf[0x1d6] != 0x00 && buf[0x1d6] < 0x81 && buf[0x1f3] !=1) {
    for (i = 0; i < 128; i++) {	/* pattern list table: 128 posbl. entries */
      max_pattern=(buf[600 - 130 + 2 + i] > max_pattern) ? buf[600 - 130 + 2 + i] : max_pattern;
    }
    if (max_pattern > 63)
      return 0;   /* pattern number can only be  0 <-> 63 for mod15 */
  } else {
    return 0;
  }

  /* parse instruments */
  for (i = 0; i < 15; i++) {
    vol = buf[45 + i * 30];
    slen = ((buf[42 + i * 30] << 8) + buf[43 + i * 30]) * 2;
    srep = ((buf[46 + i * 30] << 8) + buf[47 + i * 30]);
    sreplen = ((buf[48 + i * 30] << 8) + buf[49 + i * 30]) * 2;
    /* fprintf (stderr, "%d, slen: %d, %d (srep %d, sreplen %d), vol: %d\n",i, slen, srep+sreplen,srep, sreplen, vol); */

    if (vol > 64 && buf[44+i*30] != 0) return 0; /* vol and finetune */

    if (slen == 0) {

      if (vol == 0)
	noof_slen_zero_vol_zero++;

      if (sreplen == 0 )
	noof_slen_zero_sreplen_zero++;

    } else {
      if ((srep+sreplen) > slen)
	srep_bigger_slen++;
    }
       	
    /* slen < 9999 */
    slen = (buf[42 + i * 30] << 8) + buf[43 + i * 30];
    if (slen <= 9999) {
      /* repeat offset + repeat size*2 < word size */
      srep = ((buf[48 + i * 30] << 8) + buf[49 + i * 30]) * 2 +
	((buf[46 + i * 30] << 8) + buf[47 + i * 30]);
      if (srep > 0xffff) srep_bigger_ffff++;
    }

    if  (buf[25+i*30] ==':' && buf [22+i*30] == '-' &&
	 ((buf[20+i*30] =='S' && buf [21+i*30] == 'T') ||
	  (buf[20+i*30] =='s' && buf [21+i*30] == 't'))) st_xy++;
  }

  /* parse pattern data -> fill pfx[] with number of times fx being used*/
  memset (pfx, 0, sizeof (pfx));
  memset (pfxarg, 0, sizeof (pfxarg));

  modparsing(buf, bufsize, S15_HEADER_LENGTH, max_pattern, pfx, pfxarg);

  /* and now for let's see if we can spot the mod */

/* FX used:					  */
/* Ultimate ST:			0,1,2		  */
/* MasterSoundtracker:		0,1,2,    c,  e,f */
/* DOC-Soundtracker V2.2:	0,1,2,a,b,c,d,e,f */
/* Soundtracker I-VI		0,1,2,3,4,5,6,7,8,9,a,b,c,d,e,f*/


  /* Check for fx used between 0x3 <-> 0xb for some weird ST II-IV mods */ 
  for (j = 0x5; j < 0xa; j++) {
    if (pfx[j] != 0)
      return 4; /* ST II-IV */
  }

  for (j = 0x0c; j < 0x11; j++) {
    if (pfx[j] != 0) {

      if (pfx[0x0d] != 0 && pfxarg[0x0d] != 0)
	return 4; /* ST II-IV */

      if (pfx[0x0b] != 0 || pfx[0x0d] != 0 || pfx[0x0a]!= 0 ) {
	return 1;	/* DOC ST */
      } else {
	if (pfxarg[1] > 0xe || pfxarg[2] > 0xe)
	  return 1;	/* DOC ST */

	return 3;	/* Master ST */
      }
    }
  }

  /* pitchbend out of range ? */
  if ((pfxarg[1] > 0 && pfxarg[1] <0x1f) ||
      (pfxarg[2] > 0 && pfxarg [2] <0x1f) ||
      pfx [0] >2) return 1; // ST style Arpeggio, Pitchbends ???
  
  if (pfx[1] > 0 || pfx[2] > 0)
    return 2; /* nope UST like fx */

  /* the rest of the files has no fx. so check instruments */
  if (st_xy!=0 && noof_slen_zero_vol_zero == 0 &&
      noof_slen_zero_sreplen_zero == 0 && buf[0x1d7] == 120) {
    return 3;
  }

  /* no fx, no loops... let's simply guess :)*/
  if (srep_bigger_slen == 0 && srep_bigger_ffff == 0 &&
      ((st_xy != 0 && buf[0x1d7] != 120 ) || st_xy==0))
    return 2;

  return 3; /* anything is played as normal soundtracker */
}

/* Reject WAV files so that uadefs doesn't cause bad behaviour */
static int is_wav_file(unsigned char *buf, size_t size)
{
	if (size < WAV_HEADER_LEN)
		return 0;

	if (memcmp(buf, "RIFF", 4))
		return 0;

	if (memcmp(buf + 8, "WAVEfmt ", 8))
		return 0;

	if (memcmp(buf + 36, "data", 4))
		return 0;

	return 1;
}

void uade_filemagic(unsigned char *buf, size_t bufsize, char *pre,
		    size_t realfilesize, const char *path, int verbose)
{
  /* char filemagic():
     detects formats like e.g.: tfmx1.5, hip, hipc, fc13, fc1.4      
     - tfmx 1.5 checking based on both tfmx DT and tfmxplay by jhp, 
     and the EP by Don Adan/WT.
     - tfmx 7v checking based on info by don adan, the amore file
     ripping description and jhp's desc of the tfmx format.
     - other checks based on e.g. various player sources from Exotica 
     or by checking bytes with a hexeditor
     by far not complete...

     NOTE: Those Magic ID checks are quite lame compared to the checks the
     amiga replayer do... well, after all we are not ripping. so they
     have to do at the moment :)
   */

  int i, modtype, t;

  struct modtype {
    int e;
    char *str;
  };

  struct modtype mod32types[] = {
    {.e = MOD_SOUNDTRACKER25_NOISETRACKER10, .str = "MOD_NTK"},
    {.e = MOD_NOISETRACKER12, .str = "MOD_NTK1"},
    {.e = MOD_NOISETRACKER20, .str = "MOD_NTK2"},
    {.e = MOD_STARTREKKER4, .str = "MOD_FLT4"},
    {.e = MOD_STARTREKKER8, .str = "MOD_FLT8"},
    {.e = MOD_AUDIOSCULPTURE4, .str = "MOD_ADSC4"},
    {.e = MOD_AUDIOSCULPTURE8, .str = "MOD_ADSC8"},
    {.e = MOD_PROTRACKER, .str = "MOD"},
    {.e = MOD_FASTTRACKER, .str = "MOD_COMP"},
    {.e = MOD_NOISETRACKER, .str = "MOD_NTKAMP"},
    {.e = MOD_PTK_COMPATIBLE, .str = "MOD_COMP"},
    {.e = MOD_SOUNDTRACKER24, .str = "MOD_DOC"},
    {.str = NULL}
  };

  struct modtype mod15types[] = {
    {.e = 1, .str = "MOD15"},
    {.e = 2, .str = "MOD15_UST"},
    {.e = 3, .str = "MOD15_MST"},
    {.e = 4, .str = "MOD15_ST-IV"},
    {.str = NULL}
  };

  /* Mark format unknown by default */
  pre[0] = 0;

  if (is_wav_file(buf, bufsize)) {
    strcpy(pre, "reject");
    return;
  }

  modtype = mod32check(buf, bufsize, realfilesize, path, verbose);
  if (modtype != MOD_UNDEFINED) {
    for (t = 0; mod32types[t].str != NULL; t++) {
      if (modtype == mod32types[t].e) {
	strcpy(pre, mod32types[t].str);
	return;
      }
    }
  }

  /* 0x438 == S31_HEADER_LENGTH - 4 */
  if (((buf[0x438] >= '1' && buf[0x438] <= '3')
       && (buf[0x439] >= '0' && buf[0x439] <= '9') && buf[0x43a] == 'C'
       && buf[0x43b] == 'H') || ((buf[0x438] >= '2' && buf[0x438] <= '8')
				 && buf[0x439] == 'C' && buf[0x43a] == 'H'
				 && buf[0x43b] == 'N')
      || (buf[0x438] == 'T' && buf[0x439] == 'D' && buf[0x43a] == 'Z')
      || (buf[0x438] == 'O' && buf[0x439] == 'C' && buf[0x43a] == 'T'
	  && buf[0x43b] == 'A') || (buf[0x438] == 'C' && buf[0x439] == 'D'
				    && buf[0x43a] == '8'
				    && buf[0x43b] == '1')) {
    strcpy(pre, "MOD_PC");	/*Multichannel Tracker */

  } else if (buf[0x2c] == 'S' && buf[0x2d] == 'C' && buf[0x2e] == 'R'
	     && buf[0x2f] == 'M') {
    strcpy(pre, "S3M");		/*Scream Tracker */
    
  } else if ((buf[0] == 0x60 && buf[2] == 0x60 && buf[4] == 0x48
	      && buf[5] == 0xe7) || (buf[0] == 0x60 && buf[2] == 0x60
				     && buf[4] == 0x41 && buf[5] == 0xfa)
	     || (buf[0] == 0x60 && buf[1] == 0x00 && buf[4] == 0x60
		 && buf[5] == 0x00 && buf[8] == 0x48 && buf[9] == 0xe7)
	     || (buf[0] == 0x60 && buf[1] == 0x00 && buf[4] == 0x60
		 && buf[5] == 0x00 && buf[8] == 0x60 && buf[9] == 0x00
		 && buf[12] == 0x60 && buf[13] == 0x00 && buf[16] == 0x48
		 && buf[17] == 0xe7)) {
    strcpy(pre, "SOG");		/* Hippel */
    
  } else if (buf[0x348] == '.' && buf[0x349] == 'Z' && buf[0x34A] == 'A'
	     && buf[0x34B] == 'D' && buf[0x34c] == 'S' && buf[0x34d] == '8'
	     && buf[0x34e] == '9' && buf[0x34f] == '.') {
    strcpy(pre, "MKII");	/* Mark II */

  } else if (read_be_u16(&buf[0x00])  == 0x2b7c &&
	     read_be_u16(&buf[0x08])  == 0x2b7c &&
	     read_be_u16(&buf[0x10])  == 0x2b7c &&
	     read_be_u16(&buf[0x18])  == 0x2b7c &&
	     read_be_u32(&buf[0x20]) == 0x303c00ff &&
	     read_be_u32(&buf[0x24]) == 0x32004eb9 &&
	     read_be_u16(&buf[0x2c])  == 0x4e75)	{
    strcpy(pre, "JPO");	/* Steve Turner*/
        
  } else if (((buf[0] == 0x08 && buf[1] == 0xf9 && buf[2] == 0x00
	       && buf[3] == 0x01) && (buf[4] == 0x00 && buf[5] == 0xbb
				      && buf[6] == 0x41 && buf[7] == 0xfa)
	      && ((buf[0x25c] == 0x4e && buf[0x25d] == 0x75)
		  || (buf[0x25c] == 0x4e && buf[0x25d] == 0xf9)))
	     || ((buf[0] == 0x41 && buf[1] == 0xfa)
		 && (buf[4] == 0xd1 && buf[5] == 0xe8)
		 && (((buf[0x230] == 0x4e && buf[0x231] == 0x75)
		      || (buf[0x230] == 0x4e && buf[0x231] == 0xf9))
		     || ((buf[0x29c] == 0x4e && buf[0x29d] == 0x75)
			 || (buf[0x29c] == 0x4e && buf[0x29d] == 0xf9))
		     ))) {
    strcpy(pre, "SID1");	/* SidMon1 */

  } else if (buf[0] == 0x4e && buf[1] == 0xfa &&
	     buf[4] == 0x4e && buf[5] == 0xfa &&
	     buf[8] == 0x4e && buf[9] == 0xfa &&
	     buf[2] == 0x00 && buf[6] == 0x06 && buf[10] == 0x07) {
	     if (buf[3] == 0x2a && buf[7] == 0xfc && buf[11] == 0x7c) {
	        strcpy(pre, "SA_old");
	    } else if (buf[3] == 0x1a && buf[7] == 0xc6 && buf[11] == 0x3a) {
	        strcpy(pre, "SA");
	    }

  } else if (buf[0] == 0x4e && buf[1] == 0xfa &&
	     buf[4] == 0x4e && buf[5] == 0xfa &&
	     buf[8] == 0x4e && buf[9] == 0xfa &&
	     buf[0xc] == 0x4e && buf[0xd] == 0xfa) {
    for (i = 0x10; i < 256; i = i + 2) {
      if (buf[i + 0] == 0x4e && buf[i + 1] == 0x75 && buf[i + 2] == 0x47
	  && buf[i + 3] == 0xfa && buf[i + 12] == 0x4e && buf[i + 13] == 0x75) {
	strcpy(pre, "FRED");	/* FRED */
	break;
      }
    }

  } else if (buf[0] == 0x60 && buf[1] == 0x00 &&
	     buf[4] == 0x60 && buf[5] == 0x00 &&
	     buf[8] == 0x60 && buf[9] == 0x00 &&
	     buf[12] == 0x48 && buf[13] == 0xe7) {
    strcpy(pre, "MA");		/*Music Assembler */

  } else if (buf[0] == 0x00 && buf[1] == 0x00 &&
	     buf[2] == 0x00 && buf[3] == 0x28 &&
	     (buf[7] >= 0x34 && buf[7] <= 0x64) &&
	     buf[0x20] == 0x21 && (buf[0x21] == 0x54 || buf[0x21] == 0x44)
	     && buf[0x22] == 0xff && buf[0x23] == 0xff) {
    strcpy(pre, "SA-P");	/*SonicArranger Packed */


  } else if (buf[0] == 0x4e && buf[1] == 0xfa &&
	     buf[4] == 0x4e && buf[5] == 0xfa &&
	     buf[8] == 0x4e && buf[9] == 0xfa) {
    t = ((buf[2] * 256) + buf[3]);
    if (t < bufsize - 9) {
      if (buf[2 + t] == 0x4b && buf[3 + t] == 0xfa &&
	  buf[6 + t] == 0x08 && buf[7 + t] == 0xad && buf[8 + t] == 0x00
	  && buf[9 + t] == 0x00) {
	strcpy(pre, "MON");	/*M.O.N */
      }
    }

  } else if (buf[0] == 0x02 && buf[1] == 0x39 &&
	     buf[2] == 0x00 && buf[3] == 0x01 &&
	     buf[8] == 0x66 && buf[9] == 0x02 &&
	     buf[10] == 0x4e && buf[11] == 0x75 &&
	     buf[12] == 0x78 && buf[13] == 0x00 &&
	     buf[14] == 0x18 && buf[15] == 0x39) {
    strcpy(pre, "MON_old");	/*M.O.N_old */

  } else if (buf[0] == 0x48 && buf[1] == 0xe7 && buf[2] == 0xf1
	     && buf[3] == 0xfe && buf[4] == 0x61 && buf[5] == 0x00) {
    t = ((buf[6] * 256) + buf[7]);
    if (t < (bufsize - 17)) {
      for (i = 0; i < 10; i = i + 2) {
	if (buf[6 + t + i] == 0x47 && buf[7 + t + i] == 0xfa) {
	  strcpy(pre, "DW");	/*Whittaker Type1... FIXME: incomplete */
	}
      }
    }

  } else if (buf[0] == 0x13 && buf[1] == 0xfc &&
	     buf[2] == 0x00 && buf[3] == 0x40 &&
	     buf[8] == 0x4e && buf[9] == 0x71 &&
	     buf[10] == 0x04 && buf[11] == 0x39 &&
	     buf[12] == 0x00 && buf[13] == 0x01 &&
	     buf[18] == 0x66 && buf[19] == 0xf4 &&
	     buf[20] == 0x4e && buf[21] == 0x75 &&
	     buf[22] == 0x48 && buf[23] == 0xe7 &&
	     buf[24] == 0xff && buf[25] == 0xfe) {
    strcpy(pre, "EX");		/*Fashion Tracker */

/* Magic ID */
  } else if (buf[0x3a] == 'S' && buf[0x3b] == 'I' && buf[0x3c] == 'D' &&
	     buf[0x3d] == 'M' && buf[0x3e] == 'O' && buf[0x3f] == 'N' &&
	     buf[0x40] == ' ' && buf[0x41] == 'I' && buf[0x42] == 'I') {
    strcpy(pre, "SID2");	/* SidMon II */

  } else if (buf[0x28] == 'R' && buf[0x29] == 'O' && buf[0x2a] == 'N' &&
	     buf[0x2b] == '_' && buf[0x2c] == 'K' && buf[0x2d] == 'L' &&
	     buf[0x2e] == 'A' && buf[0x2f] == 'R' && buf[0x30] == 'E' &&
	     buf[0x31] == 'N') {
    strcpy(pre, "CM");		/* Ron Klaren (CustomMade) */

  } else if (buf[0x3e] == 'A' && buf[0x3f] == 'C' && buf[0x40] == 'T'
	     && buf[0x41] == 'I' && buf[0x42] == 'O' && buf[0x43] == 'N'
	     && buf[0x44] == 'A' && buf[0x45] == 'M') {
    strcpy(pre, "AST");		/*Actionanamics */

  } else if (buf[26] == 'V' && buf[27] == '.' && buf[28] == '2') {
    strcpy(pre, "BP");		/* Soundmon V2 */

  } else if (buf[26] == 'V' && buf[27] == '.' && buf[28] == '3') {
    strcpy(pre, "BP3");		/* Soundmon V2.2 */

  } else if (buf[60] == 'S' && buf[61] == 'O' && buf[62] == 'N'
	     && buf[63] == 'G') {
    strcpy(pre, "SFX13");	/* Sfx 1.3-1.8 */

  } else if (buf[124] == 'S' && buf[125] == 'O' && buf[126] == '3'
	     && buf[127] == '1') {
    strcpy(pre, "SFX20");	/* Sfx 2.0 */

  } else if (buf[0x1a] == 'E' && buf[0x1b] == 'X' && buf[0x1c] == 'I'
	     && buf[0x1d] == 'T') {
    strcpy(pre, "AAM");		/*Audio Arts & Magic */
  } else if (buf[8] == 'E' && buf[9] == 'M' && buf[10] == 'O'
	     && buf[11] == 'D' && buf[12] == 'E' && buf[13] == 'M'
	     && buf[14] == 'I' && buf[15] == 'C') {
    strcpy(pre, "EMOD");	/* EMOD */

    /* generic ID Check at offset 0x24 */

  } else if (chk_id_offset(buf, bufsize, offset_0024_patterns, 0x24, pre)) {

    /* HIP7 ID Check at offset 0x04 */
  } else if (patterntest(buf, " **** Player by Jochen Hippel 1990 **** ",
			 0x04, 40, bufsize)) {
    strcpy(pre, "S7G");	/* HIP7 */

    /* Magic ID at Offset 0x00 */
  } else if (buf[0] == 'M' && buf[1] == 'M' && buf[2] == 'D') {
    if (buf[0x3] >= '0' && buf[0x3] < '3') {
      /*move.l mmd_songinfo(a0),a1 */
      int s = (buf[8] << 24) + (buf[9] << 16) + (buf[0xa] << 8) + buf[0xb];
      if (((int) buf[s + 767]) & (1 << 6)) {	/* btst #6, msng_flags(a1); */
	strcpy(pre, "OCTAMED");
       /*OCTAMED*/} else {
	strcpy(pre, "MED");
       /*MED*/}
    } else if (buf[0x3] != 'C') {
      strcpy(pre, "MMD3");	/* mmd3 and above */
    }

    /* all TFMX format tests here */
  } else if (tfmxtest(buf, bufsize, pre)) {
    /* is TFMX, nothing to do here ('pre' set in tfmxtest() */

  } else if (buf[0] == 'T' && buf[1] == 'H' && buf[2] == 'X') {
    if ((buf[3] == 0x00) || (buf[3] == 0x01)) {
      strcpy(pre, "AHX");	/* AHX */
    }

  } else if (buf[1] == 'M' && buf[2] == 'U' && buf[3] == 'G'
	     && buf[4] == 'I' && buf[5] == 'C' && buf[6] == 'I'
	     && buf[7] == 'A' && buf[8] == 'N') {
    if (buf[9] == '2') {
      strcpy(pre, "MUG2");	/* Digimugi2 */
    } else {
      strcpy(pre, "MUG");	/* Digimugi */
    }

  } else if (buf[0] == 'L' && buf[1] == 'M' && buf[2] == 'E' && buf[3] == 0x00) {
    strcpy(pre, "LME");		/* LegLess */

  } else if (buf[0] == 'P' && buf[1] == 'S' && buf[2] == 'A' && buf[3] == 0x00) {
    strcpy(pre, "PSA");		/* PSA */

  } else if ((buf[0] == 'S' && buf[1] == 'y' && buf[2] == 'n' && buf[3] == 't'
	      && buf[4] == 'h' && buf[6] == '.' && buf[8] == 0x00)
	     && (buf[5] > '1' && buf[5] < '4')) {
    strcpy(pre, "SYN");		/* Synthesis */

  } else if (buf[0xbc6] == '.' && buf[0xbc7] == 'F' && buf[0xbc8] == 'N'
	     && buf[0xbc9] == 'L') {
    strcpy(pre, "DM2");		/* Delta 2.0 */

  } else if (buf[0] == 'R' && buf[1] == 'J' && buf[2] == 'P') {

    if (buf[4] == 'S' && buf[5] == 'M' && buf[6] == 'O' && buf[7] == 'D') {
      strcpy(pre, "RJP");	/* Vectordean (Richard Joseph Player) */
    } else {
      strcpy(pre, "");		/* but don't play .ins files */
    }
  } else if (buf[0] == 'F' && buf[1] == 'O' && buf[2] == 'R' && buf[3] == 'M') {
    if (buf[8] == 'S' && buf[9] == 'M' && buf[10] == 'U' && buf[11] == 'S') {
      strcpy(pre, "SMUS");	/* Sonix */
    }
    // } else if (buf[0x00] == 0x00 && buf[0x01] == 0xfe &&
    //            buf[0x30] == 0x00 && buf[0x31] ==0x00 && buf[0x32] ==0x01 && buf[0x33] ==0x40 &&
    //          realfilesize > 332 ){
    //         }
    //         strcpy (pre, "SMUS");              /* Tiny Sonix*/

  } else if (tronictest(buf, bufsize)) {
    strcpy(pre, "TRONIC");	/* Tronic */

    /* generic ID Check at offset 0x00 */
  } else if (chk_id_offset(buf, bufsize, offset_0000_patterns, 0x00, pre)) {

    /*magic ids of some modpackers */
  } else if (buf[0x438] == 'P' && buf[0x439] == 'W' && buf[0x43a] == 'R'
	     && buf[0x43b] == 0x2e) {
    strcpy(pre, "PPK");		/*Polkapacker */

  } else if (buf[0x100] == 'S' && buf[0x101] == 'K' && buf[0x102] == 'Y'
	     && buf[0x103] == 'T') {
    strcpy(pre, "SKT");		/*Skytpacker */

  } else if ((buf[0x5b8] == 'I' && buf[0x5b9] == 'T' && buf[0x5ba] == '1'
	      && buf[0x5bb] == '0') || (buf[0x5b8] == 'M' && buf[0x5b9] == 'T'
					&& buf[0x5ba] == 'N'
					&& buf[0x5bb] == 0x00)) {
    strcpy(pre, "ICE");		/*Ice/Soundtracker 2.6 */

  } else if (buf[0x3b8] == 'K' && buf[0x3b9] == 'R' && buf[0x3ba] == 'I'
	     && buf[0x3bb] == 'S') {
    strcpy(pre, "KRIS");	/*Kristracker */

  } else if (buf[0] == 'X' && buf[1] == 'P' && buf[2] == 'K' && buf[3] == 'F'&&
	     read_be_u32(&buf[4]) + 8 == realfilesize &&
	     buf[8] == 'S' && buf[9] == 'Q' && buf[10] == 'S' && buf[11] == 'H') {
    fprintf(stderr, "uade: The file is SQSH packed. Please depack first.\n");
    strcpy(pre, "packed");

  } else if ((modtype = mod15check(buf, bufsize, realfilesize, path)) != 0) {
    for (t = 0; mod15types[t].str != NULL; t++) {
      if (modtype == mod15types[t].e) {
	strcpy(pre, mod15types[t].str);
	return;
      }
    }

    /* Custom file check */
  } else if (buf[0] == 0x00 && buf[1] == 0x00 && buf[2] == 0x03
	     && buf[3] == 0xf3) {
     /*CUSTOM*/ i = (buf[0x0b] * 4) + 0x1c;	/* beginning of first chunk */

    if (i < bufsize - 0x42) {

      t = 0;
      /* unfort. we can't always assume: moveq #-1,d0 rts before "delirium" */
      /* search 0x40 bytes from here, (enough?) */
      while ((buf[i + t + 0] != 'D' && buf[i + t + 1] != 'E'
	      && buf[i + t + 2] != 'L' && buf[i + t + 3] != 'I')
	     && (t < 0x40)) {
	t++;
      }

      if (t < 0x40) {
	/* longword after Delirium is rel. offset from first chunk 
	   where "hopefully" the delitags are */
	int s = (buf[i + t + 10] * 256) + buf[i + t + 11] + i;	/* 64K */
	if (s < bufsize - 0x33) {
	  for (i = 0; i < 0x30; i = i + 4) {
	    if (buf[i + s + 0] == 0x80 && buf[i + s + 1] == 0x00 &&
		buf[i + s + 2] == 0x44 && buf[i + s + 3] == 0x55) {
	      strcpy(pre, "CUST");	/* CUSTOM */
	      break;
	    }
	  }
	}
      }
    }

  } else if (buf[12] == 0x00) {
    int s = (buf[12] * 256 + buf[13] + 1) * 14;
    if (s < (bufsize - 91)) {
      if (buf[80 + s] == 'p' && buf[81 + s] == 'a' && buf[82 + s] == 't'
	  && buf[83 + s] == 't' && buf[87 + s] == 32 && buf[88 + s] == 'p'
	  && buf[89 + s] == 'a' && buf[90 + s] == 't' && buf[91 + s] == 't') {
	strcpy(pre, "PUMA");	/* Pumatracker */
      }
    }
  }
}


/* We are currently stupid and check only for a few magic IDs at the offsets
 * chk_id_offset returns 1 on success and sets the right prefix/extension
 * in pre
 * TODO: more and less easy check for the rest of the 52 trackerclones
 */
static int chk_id_offset(unsigned char *buf, int bufsize,
			 const char *patterns[], int offset, char *pre)
{
  int i;
  for (i = 0; patterns[i]; i = i + 2) {
    if (patterntest(buf, patterns[i], offset, strlen(patterns[i]), bufsize)) {
      /* match found */
      strcpy(pre, patterns[i + 1]);
      return 1;
    }
  }
  return 0;
}
