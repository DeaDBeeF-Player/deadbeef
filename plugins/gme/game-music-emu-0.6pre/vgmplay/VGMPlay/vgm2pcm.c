#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include "chips/mamedef.h"
#include "stdbool.h"
#include "VGMPlay.h"

#define SAMPLESIZE sizeof(WAVE_16BS)

INLINE int fputBE16(UINT16 Value, FILE* hFile)
{
    int RetVal;
    int ResVal;

    RetVal = fputc((Value & 0xFF00) >> 8, hFile);
    RetVal = fputc((Value & 0x00FF) >> 0, hFile);
    ResVal = (RetVal != EOF) ? 0x02 : 0x00;
    return ResVal;
}

int main(int argc, char *argv[]) {
    UINT8 result;
    WAVE_16BS *sampleBuffer;
    UINT32 bufferedLength;
    FILE *outputFile;
    void *vgmp;
    VGM_PLAYER *p;

    if (argc < 3) {
        fputs("usage: vgm2pcm vgm_file pcm_file\n", stderr);
        return 1;
    }

    vgmp = VGMPlay_Init();
    VGMPlay_Init2(vgmp);

    if (!OpenVGMFile(vgmp, argv[1])) {
        fprintf(stderr, "vgm2pcm: error: failed to open vgm_file (%s)\n", argv[1]);
        return 1;
    }

    outputFile = fopen(argv[2], "wb");
    if (outputFile == NULL) {
        fprintf(stderr, "vgm2pcm: error: failed to open pcm_file (%s)\n", argv[2]);
        return 1;
    }

    PlayVGM(vgmp);

    p = (VGM_PLAYER *) vgmp;
    
    sampleBuffer = (WAVE_16BS*)malloc(SAMPLESIZE * p->SampleRate);
    if (sampleBuffer == NULL) {
        fprintf(stderr, "vgm2pcm: error: failed to allocate %u bytes of memory\n", SAMPLESIZE * p->SampleRate);
        return 1;
    }

    while (!p->EndPlay) {
        UINT32 bufferSize = p->SampleRate;
        bufferedLength = FillBuffer(vgmp, sampleBuffer, bufferSize);
        if (bufferedLength) {
            UINT32 numberOfSamples;
            UINT32 currentSample;
            const UINT16* sampleData;

            sampleData = (UINT16*)sampleBuffer;
            numberOfSamples = SAMPLESIZE * bufferedLength / 0x02;
            for (currentSample = 0x00; currentSample < numberOfSamples; currentSample++) {
                fputBE16(sampleData[currentSample], outputFile);
            }
        }
    }

    StopVGM(vgmp);

    CloseVGMFile(vgmp);

    VGMPlay_Deinit(vgmp);

    return 0;
}
