#include "../BackgroundTypes.hlsli"
#include "../Common/NoiseCommon.hlsli"

cbuffer StarParameters : register(b1) {
    float gDensity;
    float gMinSize;
    float gMaxSize;
    float gStarBrightness;
    float gTwinkleSpeed;
    float gTwinkleStrength;
    float gBloomIntensity;
    float gStarParallaxStrength;
    float gStarTime;
    float3 gStarPadding;
};

float3 StarLayer(float3 ray, float scale, float seed, out float bloom) {
    float3 p = ray * scale + gCameraPosition * gStarParallaxStrength * 0.002f;
    float3 cell = floor(p);
    float3 local = frac(p) - 0.5f;
    float3 random = Hash33(cell + seed);
    float densityPatch = 0.55f + 0.65f * ValueNoise(cell * 0.075f + seed);
    float exists = step(1.0f - saturate(gDensity * densityPatch), random.x);
    float3 starPosition = (random - 0.5f) * 0.72f;
    float distanceToStar = length(local - starPosition);
    // pは既にセル座標なので、星のサイズもセル座標単位のまま扱う。
    float size = lerp(gMinSize, gMaxSize, random.y);
    float shape = exp2(-distanceToStar * distanceToStar / max(size * size, 1e-7f) * 2.6f) * exists;
    float phase = random.z * 6.2831853f;
    float speed = lerp(0.55f, 1.65f, Hash31(cell.yzx + seed));
    float hot = smoothstep(0.91f, 0.995f, Hash31(cell.zxy + seed * 3.1f));
    // 星の位置は固定し、位相と速度を星ごとに変える。希少な高輝度星だけ明滅を強くする。
    float twinkleAmplitude = gTwinkleStrength * lerp(0.45f, 1.0f, hot);
    float twinkle = max(0.2f, 1.0f + sin(gStarTime * gTwinkleSpeed * speed + phase) * twinkleAmplitude);
    float3 cool = float3(0.62f, 0.82f, 1.0f);
    float3 warm = float3(1.0f, 0.68f, 0.86f);
    float3 color = lerp(cool, warm, random.z) * shape * twinkle * gStarBrightness * lerp(0.55f, 2.4f, hot);
    bloom = shape * hot * twinkle * gBloomIntensity;
    return color;
}

BackgroundOutput main(BackgroundVSOutput input) {
    float3 ray = MakeBackgroundViewRay(input.uv);
    float bloomA, bloomB;
    float3 color = StarLayer(ray, 82.0f, 11.0f, bloomA);
    color += StarLayer(ray, 137.0f, 37.0f, bloomB) * 0.65f;
    BackgroundOutput output;
    output.color = float4(color, 1.0f);
    output.bloomMask = float4(color * (bloomA + bloomB), 1.0f);
    return output;
}
