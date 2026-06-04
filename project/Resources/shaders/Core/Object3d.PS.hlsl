#include "Object3D.hlsli"

///////////////////////////////////////////////////////////////////////////////
//                            structs
///////////////////////////////////////////////////////////////////////////////
struct Material {
    float4 color;
    int enableLighting;
    float3 pad;
    float4x4 uvTransform;
    float shiniess;

    int isReflect;
    float environmentCoefficient;
    float roughness;
    float4 toonHighlightColor;
    float4 toonBaseColor;
    float4 toonMidShadowColor;
    float4 toonShadowColor;
    float toonBaseStep;
    float toonBaseFeather;
    float toonShadeStep;
    float toonShadeFeather;
    float toonSpecularThreshold;
    float toonSpecularSoftness;
    float toonSpecularIntensity;
    float pad3;
    float4 emissiveColor;
    float emissiveIntensity;
    int useNormalMap;
    float normalMapStrength;
    int normalMapFlipY;
};

struct DirectionalLight {
    float4 color;
    float3 direction;
    float intensity;
};

struct PointLight {
    float4 color;
    float3 position;
    float intensity;
    float radius;
    float decay;
    float2 pad;
};

struct Object3dVertexOutput {
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
    float3 worldPosition : POSITION0;
    float4 tangent : TANGENT0;
};

///////////////////////////////////////////////////////////////////////////////
//                            cbuffers
///////////////////////////////////////////////////////////////////////////////
cbuffer MaterialConstants : register(b0) { Material gMaterial; }
cbuffer DirectionalLightConstants : register(b2) { DirectionalLight gDirectionalLight; }

cbuffer ShadowConstants : register(b3) {
    float4x4 gLightVP;
    float gShadowBias;
    float3 _shadowPad;
};

cbuffer PointLightConstants : register(b4) {
    uint gPointLightCount;
    uint gPointLightShadowsEnabled;
    uint gMaxPointShadowLights;
    float gPointShadowContributionThreshold;
    PointLight gPointLights[16];
}

// NOTE:
// gPenumbraStart / gPenumbraScale は「距離ベース」をやめるために未使用にします。
cbuffer RaytracingShadowParamConstants : register(b5) {
    float gShadowRayEps;
    float gBaseAngularRadius; // 角半径（ソフトさ）: 距離で変えない
    float gMinShadow;
	bool gIsSoft;
};

///////////////////////////////////////////////////////////////////////////////
//                            tables
///////////////////////////////////////////////////////////////////////////////
TextureCube<float4> gEnvironmentMap : register(t1);
Texture2D<float>    gShadowMap      : register(t2); // unused
RaytracingAccelerationStructure gRtScene : register(t3);
Texture2D<float4>   gTexture        : register(t0);
Texture2D<float4>   gNormalMap      : register(t4);

//////////////////////////////////////////////////////////////////////////j/////
//                            samplers
///////////////////////////////////////////////////////////////////////////////
SamplerState gSampler : register(s0);

///////////////////////////////////////////////////////////////////////////////
//                            出力
///////////////////////////////////////////////////////////////////////////////
struct PixelShaderOutput {
    float4 color     : SV_TARGET0;
    float4 bloomMask : SV_TARGET1; // Emissive bloom mask (MRT RT1)
};

///////////////////////////////////////////////////////////////////////////////
//                    関数: トーンマッピング + ガンマ補正
///////////////////////////////////////////////////////////////////////////////
float3 ApplyToneMappingAndGamma(float3 color, float exposure) {
    float3 toneMapped = color * exposure / (color * exposure + 1.0f);
    return pow(toneMapped, 1.0 / 2.2);
}

bool CheckVisibility(float3 origin, float3 dir, float tMax);
float ComputePointHardShadow_RT(float3 worldPos, float3 normal, float3 lightPos, float lightDistance);

float3 ApplyNormalMap(Object3dVertexOutput input, float2 uv, float3 vertexNormal) {
    if(gMaterial.useNormalMap == 0 || gMaterial.normalMapStrength <= 0.0f) {
        return vertexNormal;
    }

    float3 tangent = normalize(input.tangent.xyz);
    tangent = normalize(tangent - vertexNormal * dot(tangent, vertexNormal));
    float3 bitangent = normalize(cross(vertexNormal, tangent) * input.tangent.w);

    float3 normalTexel = gNormalMap.Sample(gSampler, uv).xyz;
    if(all(normalTexel > 0.99f)) {
        return vertexNormal;
    }

    float grayscaleDelta = max(abs(normalTexel.r - normalTexel.g), abs(normalTexel.g - normalTexel.b));
    if(grayscaleDelta < 0.03f) {
        float height = normalTexel.r;
        float heightDx = ddx(height);
        float heightDy = ddy(height);
        float2 uvDx = ddx(uv);
        float2 uvDy = ddy(uv);
        float texelScale = max(max(length(uvDx), length(uvDy)), 0.0001f);
        float2 heightSlope = float2(heightDx, heightDy) / texelScale;
        float3 bumpNormal = normalize(float3(-heightSlope.x, -heightSlope.y, 1.0f));
        bumpNormal.xy *= max(gMaterial.normalMapStrength, 0.0f);
        bumpNormal = normalize(bumpNormal);
        float3x3 bumpTbn = float3x3(tangent, bitangent, vertexNormal);
        return normalize(mul(bumpNormal, bumpTbn));
    }

    float3 sampledNormal = normalTexel * 2.0f - 1.0f;
    if(gMaterial.normalMapFlipY != 0) {
        sampledNormal.y *= -1.0f;
    }
    sampledNormal.xy *= max(gMaterial.normalMapStrength, 0.0f);
    sampledNormal = normalize(sampledNormal);

    float3x3 tbn = float3x3(tangent, bitangent, vertexNormal);
    return normalize(mul(sampledNormal, tbn));
}

///////////////////////////////////////////////////////////////////////////////
//                    ディザマップ
///////////////////////////////////////////////////////////////////////////////
static const float4x4 kBayerMatrix = float4x4(
	0.0 / 16.0, 8.0 / 16.0, 2.0 / 16.0, 10.0 / 16.0,
	12.0 / 16.0, 4.0 / 16.0, 14.0 / 16.0, 6.0 / 16.0,
	3.0 / 16.0, 11.0 / 16.0, 1.0 / 16.0, 9.0 / 16.0,
	15.0 / 16.0, 7.0 / 16.0, 13.0 / 16.0, 5.0 / 16.0
);

#include "StandardLighting.hlsli"
#include "ToonLighting.hlsli"

///////////////////////////////////////////////////////////////////////////////
//                    小物: ハッシュ / 回転
///////////////////////////////////////////////////////////////////////////////
float Hash21(float2 p) {
    float3 p3 = frac(float3(p.xyx) * 0.1031f);
    p3 += dot(p3, p3.yzx + 33.33f);
    return frac((p3.x + p3.y) * p3.z);
}

float2 Rotate2D(float2 v, float a) {
    float s, c;
    sincos(a, s, c);
    return float2(c*v.x - s*v.y, s*v.x + c*v.y);
}

///////////////////////////////////////////////////////////////////////////////
//    Orthonormal Basis
///////////////////////////////////////////////////////////////////////////////
void BuildOrthonormalBasis(float3 n, out float3 t, out float3 b) {
    float3 up = (abs(n.z) < 0.999f) ? float3(0,0,1) : float3(0,1,0);
    t = normalize(cross(up, n));
    b = cross(n, t);
}

///////////////////////////////////////////////////////////////////////////////
//    Poisson
///////////////////////////////////////////////////////////////////////////////
static const float2 kPoisson16[16] = {
    float2(-0.326f, -0.406f),
    float2(-0.840f, -0.074f),
    float2(-0.696f,  0.457f),
    float2(-0.203f,  0.621f),
    float2( 0.962f, -0.195f),
    float2( 0.473f, -0.480f),
    float2( 0.519f,  0.767f),
    float2( 0.185f, -0.893f),
    float2(-0.433f,  0.043f),
    float2( 0.267f,  0.271f),
    float2(-0.123f, -0.754f),
    float2( 0.735f,  0.128f),
    float2( 0.078f,  0.912f),
    float2(-0.917f,  0.381f),
    float2( 0.621f, -0.727f),
    float2(-0.589f, -0.201f)
};

///////////////////////////////////////////////////////////////////////////////
//  AnyHit（遮蔽の有無だけ）: Fast
///////////////////////////////////////////////////////////////////////////////
bool CheckVisibility(float3 origin, float3 dir, float tMax) {
    RayDesc ray;
    ray.Origin    = origin;
    ray.Direction = dir;
    ray.TMin      = 0.0f;
    ray.TMax      = tMax;

    RayQuery<
        RAY_FLAG_CULL_NON_OPAQUE |
        RAY_FLAG_CULL_BACK_FACING_TRIANGLES |
        RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES |
        RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH
    > q;

    q.TraceRayInline(
        gRtScene,
        RAY_FLAG_CULL_NON_OPAQUE |
        RAY_FLAG_CULL_BACK_FACING_TRIANGLES |
        RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES |
        RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH,
        0xFF,
        ray
    );

    q.Proceed();
    return (q.CommittedStatus() == COMMITTED_TRIANGLE_HIT);
}

///////////////////////////////////////////////////////////////////////////////
//    ハードシャドウ（RayQuery 1本）
///////////////////////////////////////////////////////////////////////////////
float ComputeDirectionalHardShadow_RT(float3 worldPos, float3 normal, float3 L) {
    float3 origin = worldPos + normal * gShadowRayEps;
    const float tMax = 1000.0f;

    bool hit = CheckVisibility(origin, L, tMax);
    return hit ? gMinShadow : 1.0f;
}

float ComputePointHardShadow_RT(float3 worldPos, float3 normal, float3 lightPos, float lightDistance) {
    float3 origin = worldPos + normal * gShadowRayEps;
    float3 toLight = lightPos - origin;
    float tMax = min(max(length(toLight) - gShadowRayEps, 0.0f), lightDistance);
    if(tMax <= 0.0f) {
        return 1.0f;
    }

    float3 L = normalize(toLight);
    bool hit = CheckVisibility(origin, L, tMax);
    return hit ? gMinShadow : 1.0f;
}

///////////////////////////////////////////////////////////////////////////////
//    ソフトシャドウ（距離ベース廃止 + 屋内で重い問題の対策入り）
//    - 角半径は gBaseAngularRadius 固定（スケール不変）
//    - 距離(tBlocker)は一切使わない
//    - 見た目維持しつつ「屋内でのレイ爆発」を抑える
//
//    対策:
//      1) center ray をサンプルに含めて無駄撃ちを1本減らす
//      2) ndotl と angularRadius でサンプル数を固定段階的に落とす
//      3) かなり暗いことが確定したら早期終了（暗いほど軽い）
//      4) 角半径が小さい時はそもそも少サンプル（硬い影は高周波が少ない）
//
///////////////////////////////////////////////////////////////////////////////
float ComputeDirectionalSoftShadow_RT(float3 worldPos, float3 normal, float3 L) {

    // ソフトシャドウ無効ならレイを飛ばさない
    if (!gIsSoft) {
        return 1.0f;
    }

    // 自己交差対策
    float3 origin = worldPos + normal * gShadowRayEps;

    const float tMax = 1000.0f;

    // まず中心1本
    bool centerHit = CheckVisibility(origin, L, tMax);
    if (!centerHit) {
        return 1.0f; // 完全に光
    }

    // 距離で変化させない：角半径固定
    float angularRadius = max(gBaseAngularRadius, 0.0f);

    // ---- サンプル数を「決定論的」に落とす（屋内対策） ----
    // 角半径が小さいほど少サンプルで十分
    int sampleCount = 16;
    if (angularRadius < 0.010f) sampleCount = 4;
    else if (angularRadius < 0.025f) sampleCount = 8;

    // さらに、斜め入射（NdotL が小さい）では影境界の寄与が小さくなるので減らす
    float ndotl = saturate(dot(normal, L));
    if (ndotl < 0.35f) sampleCount = min(sampleCount, 4);
    else if (ndotl < 0.60f) sampleCount = min(sampleCount, 8);

    // sampleCount には「中心レイも含める」設計にする
    // つまり Poisson 追加分は sampleCount-1 本
    int extraCount = max(sampleCount - 1, 0);

    float3 T, B;
    BuildOrthonormalBasis(L, T, B);

    // 画面揺れがない固定ノイズ（ワールド位置に固定）
    float rnd = Hash21(worldPos.xz * 17.0f + worldPos.yy * 3.0f);
    float ang = rnd * 6.2831853f;

    // centerHit 分を先に加算
    float occluded = 1.0f;

    // 暗いことが確定したら打ち切る閾値（0.0～1.0）
    // 屋内だとここが効いて激的に軽くなる
    const float kDarkEarlyOut = 0.75f;

    [loop]
    for (int i = 0; i < extraCount; ++i) {
        // Poisson16 を使い回す（extraCount は最大15なのでOK）
        float2 d    = Rotate2D(kPoisson16[i], ang) * angularRadius;
        float3 dirJ = normalize(L + T * d.x + B * d.y);

        bool hit = CheckVisibility(origin, dirJ, tMax);
        occluded += hit ? 1.0f : 0.0f;

        // 「かなり暗い」が確定したら早期終了（暗いほど軽い）
        if (occluded >= (float)sampleCount * kDarkEarlyOut) {
            return gMinShadow; // ここで確定させてレイを止める
        }
    }

    float shadow = 1.0f - (occluded / (float)sampleCount);

    // 真っ黒回避（= 接触暗さの下限）
    shadow = lerp(gMinShadow, 1.0f, shadow);

    return shadow;
}

///////////////////////////////////////////////////////////////////////////////
//                              main
///////////////////////////////////////////////////////////////////////////////
PixelShaderOutput main(Object3dVertexOutput input) {
    PixelShaderOutput output;

    float4 transformedUV = mul(float4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
    float4 textureColor  = gTexture.Sample(gSampler, transformedUV.xy);

    float3 albedo = gMaterial.color.rgb * textureColor.rgb;
    float  alpha  = gMaterial.color.a   * textureColor.a;

    if(gMaterial.enableLighting == 4) {
        if(alpha <= 0.01f) discard;
        float3 emissive = gMaterial.emissiveColor.rgb * max(gMaterial.emissiveIntensity, 0.0f);
        output.color     = float4(ApplyToneMappingAndGamma(albedo, 1.0f) + emissive, alpha);
        output.bloomMask = float4(emissive, 1.0f);
        return output;
    }

    float3 normal = ApplyNormalMap(input, transformedUV.xy, normalize(input.normal));
    float3 toEye  = normalize(cameraPosition - input.worldPosition);

    float3 directionalDiffuse = 0.0f;
    float3 directionalSpecular = 0.0f;
    float3 pointDiffuse = 0.0f;
    float3 pointSpecular = 0.0f;

    if (gMaterial.enableLighting == 2) {
        // Toon
        ComputeToonDirectionalLight(normal, toEye, albedo, directionalDiffuse, directionalSpecular);
        ComputeToonPointLight(normal, toEye, input.worldPosition, albedo, pointDiffuse, pointSpecular);
    } else {
        // Standard (Lambert / Half-Lambert)
        ComputeStandardDirectionalLight(normal, toEye, albedo, directionalDiffuse, directionalSpecular);
        ComputeStandardPointLight(normal, toEye, input.worldPosition, albedo, pointDiffuse, pointSpecular);
    }

    //================= ソフトシャドウ（Directional: Inline Raytracing） =================
    float3 L = normalize(-gDirectionalLight.direction);

    float shadow = 1.0f;
    if (dot(normal, L) > 0.0f) {
        shadow = gIsSoft
            ? ComputeDirectionalSoftShadow_RT(input.worldPosition, normal, L)
            : ComputeDirectionalHardShadow_RT(input.worldPosition, normal, L);
    }

    directionalDiffuse  *= shadow;
    directionalSpecular *= shadow;

    float3 litColor = directionalDiffuse + directionalSpecular + pointDiffuse + pointSpecular;

    // AOではない（定数アンビエント）
    float3 ambient = albedo * 0.07f;
    litColor += ambient;

    if(gMaterial.isReflect != 0) {
        float3 viewDir    = normalize(input.worldPosition - cameraPosition);
        float3 reflectDir = reflect(viewDir, normal);

        const float maxMipLevel = 7.0f;
        float roughness = saturate(gMaterial.roughness);
        float mipLevel = roughness * maxMipLevel;
        float reflectionWeight = gMaterial.environmentCoefficient * pow(1.0f - roughness, 2.0f);

        float3 envColor = gEnvironmentMap.SampleLevel(gSampler, reflectDir, mipLevel).rgb;
        litColor += envColor * reflectionWeight;
    }

    float3 finalColor = ApplyToneMappingAndGamma(litColor, 1.0f);

    // トーンマッピング後にEmissiveを加算し、閾値越えのHDR値として出力させる
    finalColor += gMaterial.emissiveColor.rgb * max(gMaterial.emissiveIntensity, 0.0f);

	// ---- ディザ抜き (Dithered Clipping) ----
	uint2 pixelPos = uint2(input.position.xy) % 4;
	float ditherThreshold = kBayerMatrix[pixelPos.y][pixelPos.x];

	// カメラからの距離を計算（Object3d は input.fade がないためここで計算）
	float dist = length(input.worldPosition - cameraPosition);
	float fadeNear = 2.5f; // 透け始めの距離（必要に応じて調整）
	float fadeFar = 10.0f;  // 完全に不透明になる距離（必要に応じて調整）
	float fade = saturate((dist - fadeNear) / (fadeFar - fadeNear));

	// カメラ近傍フェード値(0.0〜1.0)に基づいてピクセルを破棄
	if(cameraDitherEnabled != 0 && objectDitherEnabled != 0 && fade <= ditherThreshold) {
		discard;
	}

    if(alpha <= 0.01f) discard;

    float3 emissiveOut = gMaterial.emissiveColor.rgb * max(gMaterial.emissiveIntensity, 0.0f);
    output.color     = float4(finalColor, alpha);
    output.bloomMask = float4(emissiveOut, 1.0f);
    return output;
}
