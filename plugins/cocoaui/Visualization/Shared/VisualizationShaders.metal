#include <metal_stdlib>

using namespace metal;

#include "ShaderRendererTypes.h"
#include "ScopeShaderTypes.h"

struct RasterizerData {
    float4 position [[position]];
};

vertex RasterizerData
vertexShader(uint vertexID [[vertex_id]],
             constant ShaderRendererVertex *vertices [[buffer(ShaderRendererVertexInputIndexVertices)]],
             constant vector_uint2 *viewportSizePointer [[buffer(ShaderRendererVertexInputIndexViewportSize)]]) {
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

fragment float4 scopeFragmentShader(RasterizerData in [[stage_in]], constant ScopeFragParams &params [[buffer(0)]], constant float2 *minmax [[buffer(1)]]) {
    float index = floor(in.position.x/params.scale);
    float y = floor(in.position.y / params.scale);

    int coffs = 0;
    float line = 0;
    for (int c = 0; c < params.channels; c++) {
        int minmax_index = coffs + index;
        float ymin = minmax[minmax_index].x;
        float ymax = minmax[minmax_index].y;
        line += smoothstep(ymin - 1, ymin, y) * smoothstep(-ymax - 1, -ymax, -y);
//        line += step(ymin, y) * step(-ymax, -y);
        coffs += params.point_count;
    }

    line = clamp(line, 0.f, 1.f);

    // ensure the index is in range of the minmax array
    line *= step(index, params.point_count-1);
    line *= step(0, index);

    return params.color * line + params.backgroundColor * (1.0 - line);
}

