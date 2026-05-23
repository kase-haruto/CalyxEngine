#include"Fragment.hlsli"

float4 main(VSOutput input) : SV_TARGET{
    float4 baseColor = input.color;

    // トーンマッピング
    float exposure = 1.0f;
    float3 toneMapped = baseColor.rgb * exposure / (baseColor.rgb * exposure + 1.0f);

    // ガンマ補正
    float3 gammaCorrected = pow(toneMapped, 1.0 / 2.2);

    return float4(gammaCorrected, baseColor.a);
}