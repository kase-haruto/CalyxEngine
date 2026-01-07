#include "Object3D.hlsli"

///////////////////////////////////////////////////////////////////////////////
//                            structs
///////////////////////////////////////////////////////////////////////////////
// マテリアル
struct Material {
	float4 color;
	int enableLighting;
	float4x4 uvTransform;
	float shiniess;

	// enviromentMap
	bool isReflect;
	float enviromentCoefficient;
	float roughness; // 0.0（鏡のような反射）～ 1.0（完全にぼけた反射）
};

// ディレクショナルライト
struct DirectionalLight {
	float4 color;
	float3 direction;
	float intensity;
};

// ポイントライト
struct PointLight {
	float4 color;
	float3 position;
	float intensity;
	float radius;
	float decay;
};

///////////////////////////////////////////////////////////////////////////////
//                            cbuffers
///////////////////////////////////////////////////////////////////////////////
cbuffer MaterialConstants : register(b0) {
	Material gMaterial;
}

cbuffer DirectionalLightConstants : register(b2) {
	DirectionalLight gDirectionalLight;
}

cbuffer ShadowConstants : register(b3) {
	float4x4 gLightVP; // ワールド→ライトクリップ
	float gShadowBias; // 0.0005～0.003 くらいから調整
	float3 _shadowPad;
};

cbuffer PointLightConstants : register(b4) {
	PointLight gPointLight;
}

///////////////////////////////////////////////////////////////////////////////
//                            tables
///////////////////////////////////////////////////////////////////////////////
TextureCube<float4> gEnvironmentMap : register(t1);
Texture2D<float> gShadowMap : register(t2);
Texture2D<float4> gTexture : register(t0);

///////////////////////////////////////////////////////////////////////////////
//                            samplers
///////////////////////////////////////////////////////////////////////////////
SamplerState gSampler : register(s0);

///////////////////////////////////////////////////////////////////////////////
//                            出力
///////////////////////////////////////////////////////////////////////////////
struct PixelShaderOutput {
	float4 color : SV_TARGET0;
};

///////////////////////////////////////////////////////////////////////////////
//                    関数: トーンマッピング + ガンマ補正
///////////////////////////////////////////////////////////////////////////////
float3 ApplyToneMappingAndGamma(float3 color, float exposure) {
	float3 toneMapped = color * exposure / (color * exposure + 1.0f);
	return pow(toneMapped, 1.0 / 2.2);
}

///////////////////////////////////////////////////////////////////////////////
//                    関数: ディレクショナルライト
///////////////////////////////////////////////////////////////////////////////
void ComputeDirectionalLight(
	float3 normal,
	float3 toEye,
	float3 albedo,
	out float3 diffuse,
	out float3 specular
	) {
	diffuse = float3(0.0f, 0.0f, 0.0f);
	specular = float3(0.0f, 0.0f, 0.0f);

	float3 L = -gDirectionalLight.direction; // 表面→光源
	float NdotL = saturate(dot(normal, L));

	if(gMaterial.enableLighting == 0) {
		// Half-Lambert
		float halfLambert = pow(NdotL * 0.5f + 0.5f, 2.0f);
		diffuse = albedo * gDirectionalLight.color.rgb * halfLambert * gDirectionalLight.intensity;

		float3 H = normalize(L + toEye);
		float NdotH = saturate(dot(normal, H));
		specular = gDirectionalLight.color.rgb * pow(NdotH, gMaterial.shiniess) * gDirectionalLight.intensity;
	}
	else if(gMaterial.enableLighting == 1) {
		// Lambert
		diffuse = albedo * gDirectionalLight.color.rgb * NdotL * gDirectionalLight.intensity;

		float3 H = normalize(L + toEye);
		float NdotH = saturate(dot(normal, H));
		specular = gDirectionalLight.color.rgb * pow(NdotH, gMaterial.shiniess) * gDirectionalLight.intensity;
	}
}

///////////////////////////////////////////////////////////////////////////////
//                    関数: ポイントライト
///////////////////////////////////////////////////////////////////////////////
void ComputePointLight(
	float3 normal,
	float3 toEye,
	float3 worldPos,
	float3 albedo, // ← 呼び出し側から “material×texture” を渡す
	out float3 diffuse,
	out float3 specular
	) {
	diffuse = float3(0.0f, 0.0f, 0.0f);
	specular = float3(0.0f, 0.0f, 0.0f);

	float3 lightDir = normalize(worldPos - gPointLight.position);
	float distance = length(gPointLight.position - worldPos);
	float attenuation = pow(saturate(1.0f - distance / gPointLight.radius), gPointLight.decay);

	float NdotL = saturate(dot(normal, -lightDir)); // lightDir は表面→光源の負
	diffuse = albedo * gPointLight.color.rgb * NdotL * gPointLight.intensity * attenuation;

	float3 halfVec = normalize(-lightDir + toEye);
	float NdotH = saturate(dot(normal, halfVec));
	specular = gPointLight.color.rgb * pow(NdotH, gMaterial.shiniess) * gPointLight.intensity * attenuation;
}

///////////////////////////////////////////////////////////////////////////////
//                              
///////////////////////////////////////////////////////////////////////////////
float ComputeShadow1Tap(float3 worldPos) {
	float4 lp = mul(float4(worldPos, 1.0f), gLightVP);
	float3 ndc = lp.xyz / lp.w;

	float2 uv;
	uv.x = ndc.x * 0.5f + 0.5f;
	uv.y = -ndc.y * 0.5f + 0.5f;

	float receiverDepth = ndc.z;
	float shadowDepth = gShadowMap.SampleLevel(gSampler, uv, 0).r;

	return (receiverDepth > shadowDepth + gShadowBias)
        ? 0.0f
        : 1.0f;
}


///////////////////////////////////////////////////////////////////////////////
//                              main
///////////////////////////////////////////////////////////////////////////////
PixelShaderOutput main(VertexShaderOutput input) {
	PixelShaderOutput output;

	//================= UV 変換 & テクスチャ取得 =================
	float4 transformedUV = mul(float4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
	float4 textureColor = gTexture.Sample(gSampler, transformedUV.xy);

	// アルベド：テクスチャ × マテリアル色
	float3 albedo = gMaterial.color.rgb * textureColor.rgb;
	float alpha = gMaterial.color.a * textureColor.a;

	//================= アンリット =================
	// enableLighting == 4 を “Unlit Color”（ライティング無視でそのままの色）
	if(gMaterial.enableLighting == 4) {
		if(alpha <= 0.01f)
			discard;
		// そのままの色を出す（トーンマップ/ガンマも無視したい要望に合わせて素通し）
		output.color = float4(albedo, alpha);
		return output;
	}

	float3 normal = normalize(input.normal);
	float3 toEye = normalize(cameraPosition - input.worldPosition);

	//================= ライト計算 =================
	float3 directionalDiffuse, directionalSpecular;
	ComputeDirectionalLight(normal, toEye, albedo, directionalDiffuse, directionalSpecular);

	float3 pointDiffuse, pointSpecular;
	ComputePointLight(normal, toEye, input.worldPosition, albedo, pointDiffuse, pointSpecular);

	//================= 照明合成 =================
	float3 litColor = directionalDiffuse + directionalSpecular + pointDiffuse + pointSpecular;
	
	// ================= Shadow (1tap) =================
	float shadow = ComputeShadow1Tap(input.worldPosition);

	// まずは “影は暗くする” だけ（真っ黒だと強すぎるので 0.2 を残す）
	litColor *= lerp(0.2f, 1.0f, shadow);

	//================= 環境マップ =================
	if(gMaterial.isReflect) {
		float3 viewDir = normalize(input.worldPosition - cameraPosition);
		float3 reflectDir = reflect(viewDir, normal);

		const float maxMipLevel = 7.0f;
		float mipLevel = saturate(gMaterial.roughness) * maxMipLevel;

		float3 envColor = gEnvironmentMap.SampleLevel(gSampler, reflectDir, mipLevel).rgb;
		litColor += envColor * gMaterial.enviromentCoefficient;
	}

	//================= トーンマッピング + ガンマ補正 =================
	float3 finalColor = ApplyToneMappingAndGamma(litColor, 1.0f);

	//================= アルファ計算 =================
	if(alpha <= 0.01f)
		discard;

	//================= 出力 =================


	output.color = float4(finalColor, alpha);
	return output;
}