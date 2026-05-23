#include "Effect.hlsli"
/////////////////////////////////////////////////////////////////////////
//                      world/view/projection matrix
/////////////////////////////////////////////////////////////////////////
Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

/////////////////////////////////////////////////////////////////////////
//                      main
/////////////////////////////////////////////////////////////////////////
float4 main(VSOutput input) : SV_TARGET{
    float4 texColor = gTexture.Sample(gSampler, input.texcoord);

    float4 baseColor = float4(texColor.rgb * input.color.rgb, texColor.a * input.color.a);

    // トーンマッピング
    float exposure = 1.0f;
    float3 toneMapped = baseColor.rgb * exposure / (baseColor.rgb * exposure + 1.0f);

    // ガンマ補正
    float3 gammaCorrected = pow(toneMapped, 1.0 / 2.2);

    return float4(gammaCorrected, baseColor.a);
}