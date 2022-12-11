/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2022 Oleksiy Yakovenko and other contributors

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
*/

#ifndef SpectrumShaderTypes_h
#define SpectrumShaderTypes_h

#include <simd/simd.h>

struct SpectrumFragParams
{
    vector_float4 backgroundColor;
    vector_float4 peakColor;
    vector_float4 barColor;
    vector_float4 lineColor;
    float barWidth;
    vector_uint2 size;
    int barCount;
    int gridLineCount;
    float backingScaleFactor;
    int discreteFrequencies;
};

struct SpectrumFragBar {
    float barX;
    float peakY;
    float barHeight;
};

#endif /* SpectrumShaderTypes_h */
