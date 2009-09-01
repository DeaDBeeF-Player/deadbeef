/*
  apedec - Monkey's Audio Decoder plugin for DeaDBeeF player
  http://deadbeef.sourceforge.net

  Copyright (C) 2009 Alexey Yakovenko

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  Note: DeaDBeeF player itself uses different license
*/
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
