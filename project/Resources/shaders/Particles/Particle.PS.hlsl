#include "Particle.hlsli"

///////////////////////////////////////////////////////////////////////////////
//                            structs
///////////////////////////////////////////////////////////////////////////////
//マテリアル
struct Material {
	float4 color;
	float4x4 uvTransform;
	float4 noiseMaskParams; // x: enabled, y: scale, z: strength, w: threshold
	float4 noiseMaskUv;     // xy: offset, z: softness
};

///////////////////////////////////////////////////////////////////////////////
//                            出力
///////////////////////////////////////////////////////////////////////////////
struct PixelShaderOutput {
	float4 color : SV_TARGET0;
};

///////////////////////////////////////////////////////////////////////////////
//                            cbuffers
///////////////////////////////////////////////////////////////////////////////
ConstantBuffer<Material> gMaterial : register(b1);

///////////////////////////////////////////////////////////////////////////////
//                            tables
///////////////////////////////////////////////////////////////////////////////
Texture2D<float4> gTexture : register(t1);

///////////////////////////////////////////////////////////////////////////////
//                            samplers
///////////////////////////////////////////////////////////////////////////////
SamplerState gSampler : register(s0);

float ParticleNoiseHash21(float2 p) {
	p = frac(p * float2(123.34f, 456.21f));
	p += dot(p, p + 45.32f);
	return frac(p.x * p.y);
}

float ParticleValueNoise(float2 p) {
	float2 i = floor(p);
	float2 f = frac(p);
	f = f * f * (3.0f - 2.0f * f);
	float a = ParticleNoiseHash21(i);
	float b = ParticleNoiseHash21(i + float2(1.0f, 0.0f));
	float c = ParticleNoiseHash21(i + float2(0.0f, 1.0f));
	float d = ParticleNoiseHash21(i + float2(1.0f, 1.0f));
	return lerp(lerp(a, b, f.x), lerp(c, d, f.x), f.y);
}

///////////////////////////////////////////////////////////////////////////////
//                            dither
///////////////////////////////////////////////////////////////////////////////
static const float4x4 kBayerMatrix = float4x4(
	0.0 / 16.0, 8.0 / 16.0, 2.0 / 16.0, 10.0 / 16.0,
	12.0 / 16.0, 4.0 / 16.0, 14.0 / 16.0, 6.0 / 16.0,
	3.0 / 16.0, 11.0 / 16.0, 1.0 / 16.0, 9.0 / 16.0,
	15.0 / 16.0, 7.0 / 16.0, 13.0 / 16.0, 5.0 / 16.0
);

///////////////////////////////////////////////////////////////////////////////
//                            main
///////////////////////////////////////////////////////////////////////////////
PixelShaderOutput main(VertexShaderOutput input) {
	PixelShaderOutput output;

	// UV座標を変換
	float4 transformedUV = mul(float4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
	// テクスチャサンプル
	float4 texColor = gTexture.Sample(gSampler, transformedUV.xy);
	// 合成
	float4 baseColor = gMaterial.color * texColor * input.color;
	if(gMaterial.noiseMaskParams.x > 0.5f) {
		float2 noiseUv = transformedUV.xy * max(gMaterial.noiseMaskParams.y, 0.0001f)
			+ gMaterial.noiseMaskUv.xy;
		float noiseValue = ParticleValueNoise(noiseUv);
		float threshold = saturate(gMaterial.noiseMaskParams.w);
		float softness = max(gMaterial.noiseMaskUv.z, 0.0001f);
		float mask = smoothstep(threshold - softness, threshold + softness, noiseValue);
		baseColor.a *= lerp(1.0f, mask, saturate(gMaterial.noiseMaskParams.z));
	}
	// トーンマッピング
	float exposure = 1.0f;
	float3 toneMapped = baseColor.rgb * exposure / (baseColor.rgb * exposure + 1.0f);
	// ガンマ補正
	float3 gammaCorrected = pow(toneMapped, 1.0 / 2.2);

	output.color = float4(gammaCorrected, baseColor.a);

	// ---- ディザ抜き (Dithered Clipping) ----
	uint2 pixelPos = uint2(input.position.xy) % 4;
	float ditherThreshold = kBayerMatrix[pixelPos.y][pixelPos.x];

	// カメラ近傍フェード値(0.0〜1.0)に基づいてピクセルを破棄
	// 無効時は VS 側で常に 1.0 にしているため破棄されない
	if(input.fade <= ditherThreshold) {
		discard;
	}

	return output;
}
