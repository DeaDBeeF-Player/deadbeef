#include <mac/All.h>
#include <mac/GlobalFunctions.h>
#include <mac/MACLib.h>
#include <mac/CharacterHelper.h>
#include <mac/APETag.h>
#include "apewrapper.h"

void *
ape_decompress_create (const char *fname) {
    int ret;
    CSmartPtr<wchar_t> str;
    str.Assign (GetUTF16FromUTF8 ((const str_utf8 *)fname), TRUE);
    IAPEDecompress *dec = CreateIAPEDecompress(str, &ret);
    return dec;
}

void
ape_decompress_destroy (void *d) {
    IAPEDecompress *dec = (IAPEDecompress *)d;
    delete dec;
}


// decode process:
// 1. get input format:
//    WAVEFORMATEX wfe;
//    GetInfo(APE_INFO_WAVEFORMATEX, (intptr_t)&wfe)
// 2. get wav header
//    int size = GetInfo(APE_INFO_WAV_HEADER_BYTES)
//    char buf[size];
//    GetInfo (APE_INFO_WAV_HEADER_DATA, (intptr_t)buf, size);
// 3. allocate space for readbuffer
//    int bufsize = GetInfo(APE_INFO_BLOCK_ALIGN) * BLOCKS_PER_DECODE;
//    char readbuf[bufsize];
// 4. get total number of blocks
//    int blocksleft = GetInfo(APE_DECOMPRESS_TOTAL_BLOCKS);
// 5. decompress
//    while (blocksleft > 0) {
//      int ndecoded;
//      GetData (readbuf, BLOCKS_PER_DECODE, &ndecoded);
//      nblocksleft -= ndecoded;
//    }
// 6. terminate output
//    if (GetInfo(APE_INFO_WAV_TERMINATING_BYTES) > 0) {
//      GetInfo(APE_INFO_WAV_TERMINATING_DATA, (intptr_t)readbuf, GetInfo(APE_INFO_WAV_TERMINATING_BYTES));
//    }
int
ape_decompress_info_int (void *d, int id) {
    IAPEDecompress *dec = (IAPEDecompress *)d;
    return dec->GetInfo ((APE_DECOMPRESS_FIELDS)id);
}

int
ape_decompress_getdata (void *d, char *buffer, int nblocks, int *retr) {
    IAPEDecompress *dec = (IAPEDecompress *)d;
    return dec->GetData (buffer, nblocks, retr);
}

int
ape_decompress_seek (void *d, int nblockoffs) {
    IAPEDecompress *dec = (IAPEDecompress *)d;
    return dec->Seek (nblockoffs);
}
