#include "BackgroundTypes.hlsli"

BackgroundVSOutput main(uint vertexId : SV_VertexID) {
    BackgroundVSOutput output;
    float2 position = float2((vertexId << 1) & 2, vertexId & 2);
    output.uv = position * 0.5f;
    output.position = float4(position * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 1.0f, 1.0f);
    return output;
}
