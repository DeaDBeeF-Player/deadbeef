#ifndef ScopeShaderTypes_h
#define ScopeShaderTypes_h

#include <simd/simd.h>

struct ScopeFragParams
{
    vector_float4 color;
    vector_float4 backgroundColor;
    vector_uint2 size;
    float scale;
    int point_count;
    int channels;
};

#endif /* ScopeShaderTypes_h */
