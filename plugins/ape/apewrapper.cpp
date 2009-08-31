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

int
ape_decompress_get_info_int (void *d, int id) {
    IAPEDecompress *dec = (IAPEDecompress *)d;
    return dec->GetInfo ((APE_DECOMPRESS_FIELDS)id);
}

int
ape_decompress_get_info_data (void *d, int id, void *ptr) {
    IAPEDecompress *dec = (IAPEDecompress *)d;
    return dec->GetInfo ((APE_DECOMPRESS_FIELDS)id, (intptr_t)ptr);
}

int
ape_decompress_get_info_data_sized (void *d, int id, void *ptr, int size) {
    IAPEDecompress *dec = (IAPEDecompress *)d;
    return dec->GetInfo ((APE_DECOMPRESS_FIELDS)id, (intptr_t)ptr, size);
}

int
ape_decompress_getdata (void *d, char *buffer, int nblocks) {
    IAPEDecompress *dec = (IAPEDecompress *)d;
    int retr;
    int res = dec->GetData (buffer, nblocks, &retr);
    return retr;
}

int
ape_decompress_seek (void *d, int nblockoffs) {
    IAPEDecompress *dec = (IAPEDecompress *)d;
    return dec->Seek (nblockoffs);
}
