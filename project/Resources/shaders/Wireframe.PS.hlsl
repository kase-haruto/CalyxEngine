#include "Object3D.hlsli"

struct Material {
	float4	 color;
	int		 enableLighting;
	float4x4 uvTransform;
	float	 shiniess;

	bool  isReflect;
	float environmentCoefficient;
	float roughness;
};

cbuffer MaterialConstants : register(b0) { Material gMaterial; }

struct GSOutput {
	float4 position : SV_POSITION;
	float2 texcoord : TEXCOORD0;
	float3 normal   : NORMAL0;
	float3 worldPos : POSITION0;
};

float4 main(GSOutput input) : SV_TARGET {
	// 選択用ワイヤーフレームなので、マテリアルの色（オレンジ）をそのまま出す
	return float4(1.0f, 0.5f, 0.0f, 1.0f);
}