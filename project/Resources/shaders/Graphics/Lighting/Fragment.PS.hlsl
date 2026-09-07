#include"Fragment.hlsli"

float4 main(VSOutput input) : SV_TARGET{
    float4 baseColor = input.color;

    // トーンマッピング
    float3 linearColor = max(baseColor.rgb, 0.0f);
    float luminance = dot(linearColor, float3(0.2126f, 0.7152f, 0.0722f));
    float3 toneMapped = linearColor / (1.0f + luminance);
    return float4(pow(toneMapped, 1.0f / 2.2f), baseColor.a);
}
