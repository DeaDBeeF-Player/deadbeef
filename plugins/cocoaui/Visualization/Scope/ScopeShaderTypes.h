#ifndef ScopeShaderTypes_h
#define ScopeShaderTypes_h

#include <simd/simd.h>

// Buffer index values shared between shader and C code to ensure Metal shader buffer inputs
// match Metal API buffer set calls.
typedef enum ScopeVertexInputIndex
{
    ScopeVertexInputIndexVertices     = 0,
    ScopeVertexInputIndexViewportSize = 1,
} ScopeVertexInputIndex;

//  This structure defines the layout of vertices sent to the vertex
//  shader. This header is shared between the .metal shader and C code, to guarantee that
//  the layout of the vertex array in the C code matches the layout that the .metal
//  vertex shader expects.
typedef struct
{
    vector_float2 position;
    vector_float4 color;
} ScopeVertex;

struct FragParams
{
    vector_float4 color;
    vector_float4 backgroundColor;
    vector_uint2 size;
    float scale;
    int point_count;
    int channels;
};

#endif /* ScopeShaderTypes_h */
