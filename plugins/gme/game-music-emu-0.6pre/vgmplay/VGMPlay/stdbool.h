// custom stdbool.h to for 1-byte bool types

#ifndef _CSTM_STDBOOL_H_
#define	_CSTM_STDBOOL_H_	

#ifndef __cplusplus	// C++ already has the bool-type

#define	false	0x00
#define	true	0x01

// the MS VC++ 6 compiler uses a one-byte-type (unsigned char, to be exact), so I'll reproduce this here
typedef	unsigned char	bool;

#endif // !__cplusplus

#endif // !_CSTM_STDBOOL_H_
