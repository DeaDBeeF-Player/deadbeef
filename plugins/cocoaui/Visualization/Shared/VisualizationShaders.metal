#include <metal_stdlib>

using namespace metal;

#include "ShaderRendererTypes.h"
#include "ScopeShaderTypes.h"
#include "SpectrumShaderTypes.h"

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

// sdBox credit: https://iquilezles.org/articles/distfunctions2d/
float sdBox(float2 p, float2 b) {
    float2 d = abs(p)-b;
    return length(max(d,0.0)) + min(max(d.x,d.y),0.0);
}

float drawBar(
    float x, float y, float barX,
    float barY, float barWidth, float barHeight
) {
    float2 p(x - barX - barWidth/2, y - barY - barHeight/2);
    float2 b(barWidth/2, barHeight/2);
    float d = sdBox(p, b);
    return smoothstep(1, -1, d);
}

float4 drawSpectrumBar(float x, float y, int barIndex, float barWidth, float4 out, constant SpectrumFragParams &params, constant SpectrumFragBar *barData) {
    // fetch bar data
    float xpos = barData[barIndex].barX;
    float peak_ypos = barData[barIndex].peakY;
    float bar_height = barData[barIndex].barHeight;

    float line = 0;

    // bars
    line = drawBar(x, y, xpos, params.size.y - bar_height, barWidth, bar_height);
    out = params.barColor * line + out * (1-line);

    // peaks
    line = drawBar(x, y, xpos, params.size.y - peak_ypos - params.backingScaleFactor/2, barWidth, params.backingScaleFactor);
    out = params.peakColor * line + out * (1-line);

    return out;
}

fragment float4
spectrumFragmentShader(
                       RasterizerData in [[stage_in]],
                       constant SpectrumFragParams &params [[buffer(0)]],
                       constant SpectrumFragBar *barData [[buffer(1)]],
                       constant int *barIndexTable [[buffer(2)]]
                       )
{
    float x = in.position.x;
    float y = in.position.y;

    float4 out = params.backgroundColor;

    // grid
    float lineAlpha = params.lineColor.w;
    for (int i = 0; i < params.gridLineCount; i++) {
        float lineY = params.size.y * (float)i / (float)params.gridLineCount;

        // dotted line with 001 pattern
        float mask = saturate((float)(((int)(x / params.backingScaleFactor)) % 3) - 1);
        float line = mask * drawBar(x, y, 0, lineY, params.size.x, params.backingScaleFactor);

        out.xyz = params.lineColor.xyz * line * lineAlpha + out.xyz * (1 - line * lineAlpha);
    }

    if (params.barCount == 0) {
        return out;
    }

    int barIndex;
    float barWidth;
    if (!params.discreteFrequencies) {
        // octave bands
        barIndex = x / (float)params.size.x * params.barCount;
        barIndex = clamp(barIndex, 0, params.barCount-1);
        barWidth = params.barWidth;
    }
    else {
        barIndex = barIndexTable[(int)x];
        if (barIndex == -1) {
            return out;
        }
        barWidth = params.backingScaleFactor;
    }

    out = drawSpectrumBar(x, y, barIndex, barWidth, out, params, barData);

    return out;
}
