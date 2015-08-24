#if defined(OPLTYPE_IS_OPL2)
#define ADLIBEMU(name)			adlib_OPL2_##name
#elif defined(OPLTYPE_IS_OPL3)
#define ADLIBEMU(name)			adlib_OPL3_##name
#endif

typedef void (*ADL_UPDATEHANDLER)(void *param);

void* ADLIBEMU(init)(UINT32 clock, UINT32 samplerate,
					 ADL_UPDATEHANDLER UpdateHandler, void* param);
void ADLIBEMU(stop)(void *chip);
void ADLIBEMU(reset)(void *chip);

void ADLIBEMU(writeIO)(void *chip, UINT32 addr, UINT8 val);
void ADLIBEMU(getsample)(void *chip, INT32 ** sndptr, INT32 numsamples);

UINT32 ADLIBEMU(reg_read)(void *chip, UINT32 port);
void ADLIBEMU(write_index)(void *chip, UINT32 port, UINT8 val);

void ADLIBEMU(set_mute_mask)(void *chip, UINT32 MuteMask);
