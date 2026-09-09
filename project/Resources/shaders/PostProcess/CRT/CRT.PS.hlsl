#include "../Copy/CopyImage.hlsli"

///////////////////////////////////////////////////////////////////////////////
//                            structs
///////////////////////////////////////////////////////////////////////////////
struct CRTParam {
	float2 screenSize;
	float  time;
	float  distortionStrength;
};

///////////////////////////////////////////////////////////////////////////////
//                            cbuffers
///////////////////////////////////////////////////////////////////////////////
cbuffer CRTParam : register(b0) {
	CRTParam gCRTParam;
}

///////////////////////////////////////////////////////////////////////////////
//                            tables
///////////////////////////////////////////////////////////////////////////////
Texture2D<float4> gTexture : register(t0);

///////////////////////////////////////////////////////////////////////////////
//                            samplers
///////////////////////////////////////////////////////////////////////////////
SamplerState gSampler : register(s0);

///////////////////////////////////////////////////////////////////////////////
//                            functions
///////////////////////////////////////////////////////////////////////////////

/**
 * @brief CRT風の樽型歪み
 */
float2 ApplyBarrelDistortion(float2 uv) {
	float2 coord = uv * 2.0f - 1.0f;

	float r2 = dot(coord, coord);

	float distortion =
		1.0f
		+ gCRTParam.distortionStrength * r2
		+ gCRTParam.distortionStrength * 0.2f * r2 * r2;

	coord *= distortion;

	return coord * 0.5f + 0.5f;
}

/**
 * @brief UVが画面内かどうかを滑らかにマスクする
 */
float ScreenMask(float2 uv) {
	float2 edgeDistance = min(uv, 1.0f - uv);

	float edge = min(edgeDistance.x, edgeDistance.y);

	// 境界を少しぼかす
	return smoothstep(0.0f, 0.008f, edge);
}

/**
 * @brief CRT風のビネット
 */
float Vignette(float2 uv) {
	float2 coord = uv * 2.0f - 1.0f;

	float vignette = 1.0f - dot(coord, coord) * 0.18f;

	return saturate(vignette);
}

/**
 * @brief CRT風スキャンライン
 */
float Scanline(float2 uv) {
	float y = uv.y * gCRTParam.screenSize.y;

	float scanline =
		sin(y * 3.14159265f);

	scanline = scanline * 0.5f + 0.5f;

	return lerp(
		0.88f,
		1.0f,
		scanline
	);
}

///////////////////////////////////////////////////////////////////////////////
//                            main
///////////////////////////////////////////////////////////////////////////////
float4 main(VertexShaderOutput input) : SV_TARGET {
	//----------------------------------------------------------------------
	// Barrel Distortion
	//----------------------------------------------------------------------

	float2 distortedUV =
		ApplyBarrelDistortion(input.texcoord);

	//----------------------------------------------------------------------
	// Screen Mask
	//----------------------------------------------------------------------

	float screenMask =
		ScreenMask(distortedUV);

	//----------------------------------------------------------------------
	// Texture Sampling
	//----------------------------------------------------------------------

	// 範囲外アクセスによる端の引き伸ばしを防ぐため、
	// サンプリング時だけUVを0～1に制限する。
	float2 sampleUV =
		saturate(distortedUV);

	float3 color =
		gTexture.Sample(gSampler, sampleUV).rgb;

	//----------------------------------------------------------------------
	// Scanline
	//----------------------------------------------------------------------

	float scanline =
		Scanline(distortedUV);

	color *= scanline;

	//----------------------------------------------------------------------
	// Vignette
	//----------------------------------------------------------------------

	float vignette =
		Vignette(distortedUV);

	color *= vignette;

	//----------------------------------------------------------------------
	// Screen Border
	//----------------------------------------------------------------------

	color *= screenMask;

	return float4(color, 1.0f);
}