#include <metal_stdlib>

using namespace metal;

#include "ScopeShaderTypes.h"

struct RasterizerData {
    float4 position [[position]];
};

vertex RasterizerData
vertexShader(uint vertexID [[vertex_id]],
             constant ScopeVertex *vertices [[buffer(ScopeVertexInputIndexVertices)]],
             constant vector_uint2 *viewportSizePointer [[buffer(ScopeVertexInputIndexViewportSize)]]) {
    RasterizerData out;

    // Index into the array of positions to get the current vertex.
    // The positions are specified in pixel dimensions (i.e. a value of 100
    // is 100 pixels from the origin).
    float2 pixelSpacePosition = vertices[vertexID].position.xy;

    // Get the viewport size and cast to float.
    vector_float2 viewportSize = vector_float2(*viewportSizePointer);
    

    // To convert from positions in pixel space to positions in clip-space,
    //  divide the pixel coordinates by half the size of the viewport.
    out.position = vector_float4(0.0, 0.0, 0.0, 1.0);
    out.position.xy = pixelSpacePosition / (viewportSize / 2.0);

    return out;
}

// TODO: is it ok to pass a uniform minmax array of that large size?
fragment float4 fragmentShader(RasterizerData in [[stage_in]], constant FragParams &params [[buffer(0)]], constant float2 *minmax [[buffer(1)]]) {
    float index = floor(in.position.x/params.scale);
    float y = in.position.y/params.scale;

    int coffs = 0;
    float line = 0;
    for (int c = 0; c < params.channels; c++) {
        int minmax_index = coffs + index;
        float ymin = minmax[minmax_index].x;
        float ymax = minmax[minmax_index].y;
        line = max(line, step(ymin, y) * step(y, ymax));
        coffs += params.point_count;
    }

    // ensure the index is in range of the minmax array
    line *= step(index, params.point_count-1);
    line *= step(0, index);

    return params.color * line;
}

