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

    // environmentMap
    bool isReflect;
    float environmentCoefficient;
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

cbuffer RaytracingShadowParamConstants : register(b5) {
	float gShadowRayEps		= 0.01f;
	float gBaseAngularRadius = 0.05f;
	float gPenumbraStart		= 0.05f;
	float gPenumbraScale		= 1.0f;
	float gMinShadow			= 0.1f;
};

///////////////////////////////////////////////////////////////////////////////
//                            tables
///////////////////////////////////////////////////////////////////////////////
TextureCube<float4> gEnvironmentMap : register(t1);
RaytracingAccelerationStructure gRtScene : register(t3);
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
    diffuse  = 0.0f;
    specular = 0.0f;

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
    float3 albedo, 
    out float3 diffuse,
    out float3 specular
) {
    diffuse  = 0.0f;
    specular = 0.0f;

    float3 lightDir = normalize(worldPos - gPointLight.position);
    float distance  = length(gPointLight.position - worldPos);
    float attenuation = pow(saturate(1.0f - distance / gPointLight.radius), gPointLight.decay);

    float NdotL = saturate(dot(normal, -lightDir)); // lightDir は表面→光源の負
    diffuse = albedo * gPointLight.color.rgb * NdotL * gPointLight.intensity * attenuation;

    float3 halfVec = normalize(-lightDir + toEye);
    float NdotH = saturate(dot(normal, halfVec));
    specular = gPointLight.color.rgb * pow(NdotH, gMaterial.shiniess) * gPointLight.intensity * attenuation;
}

///////////////////////////////////////////////////////////////////////////////
//                    小物: ハッシュ / 回転
///////////////////////////////////////////////////////////////////////////////
float Hash11(float p) {
    p = frac(p * 0.1031f);
    p *= p + 33.33f;
    p *= p + p;
    return frac(p);
}

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
//    小物: Orthonormal Basis（L を軸にした接平面基底）
///////////////////////////////////////////////////////////////////////////////
void BuildOrthonormalBasis(float3 n, out float3 t, out float3 b) {
    // n: 正規化済み想定
    float3 up = (abs(n.z) < 0.999f) ? float3(0,0,1) : float3(0,1,0);
    t = normalize(cross(up, n));
    b = cross(n, t);
}

///////////////////////////////////////////////////////////////////////////////
//    ソフトシャドウ（距離依存）: Inline Raytracing
//    - 1本レイで blocker 距離 t を取得
//    - t が大きいほど jitter 半径を増やして “ぼけ” を強める
///////////////////////////////////////////////////////////////////////////////
static const int   kShadowSamples = 16;

// Poisson disk（固定サンプル）
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

bool TraceOcclusion(float3 origin, float3 dir, float tMin, float tMax, out float hitT) {
    RayQuery<RAY_FLAG_CULL_NON_OPAQUE |
             RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES |
             RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH> q;

    RayDesc ray;
    ray.Origin    = origin;
    ray.Direction = dir;
    ray.TMin      = tMin;
    ray.TMax      = tMax;

    q.TraceRayInline(gRtScene, RAY_FLAG_NONE, 0xFF, ray);
    q.Proceed();

    if (q.CommittedStatus() == COMMITTED_TRIANGLE_HIT) {
        hitT = q.CommittedRayT();
        return true;
    }

    hitT = 0.0f;
    return false;
}

float ComputeDirectionalSoftShadow_RT(float3 worldPos, float3 normal, float3 L) {

    // ---- 調整パラメータ（DirectionalLight.shadowParam から取得） ----
    // 自己交差対策（ワールドスケールに合わせて調整）
	const float kRayEps = 0.01f;

	const float kBaseAngularRadius = 0.5f;
	const float kPenumbraStart     = 0.5f;
	const float kPenumbraScale     = 1.0f;
	// 影の濃さ（小さいほど暗い。0.0fで完全な黒）
	const float kMinShadow         = 0.05f;

    float3 origin = worldPos + normal * kRayEps;

    // ---- まず blocker を1本で調べる ----
    float tBlocker = 0.0f;
    bool blocked = TraceOcclusion(origin, L, 0.0f, 1000.0f, tBlocker);
    if (!blocked) {
        return 1.0f;
    }

    // ---- 距離依存で "ぼけ半径" を決める ----
    float s = saturate((tBlocker - kPenumbraStart) / max(kPenumbraScale, 1e-5f));
    float angularRadius = kBaseAngularRadius * (1.0f + s * 2.0f); // 最小1.0x、最大3.0x

    // 方向Lに直交する平面の基底
    float3 T, B;
    BuildOrthonormalBasis(L, T, B);

    // パターン回転（バンディング軽減）
    float rnd = Hash21(worldPos.xz * 17.0f + worldPos.yy * 3.0f);
    float ang = rnd * 6.2831853f;

    // ---- 複数サンプルで平均（ソフトシャドウ） ----
    float occluded = 0.0f;

    [loop]
    for (int i = 0; i < kShadowSamples; ++i) {
        float2 d = Rotate2D(kPoisson16[i], ang) * angularRadius;
        float3 dirJ = normalize(L + T * d.x + B * d.y);

        float hitT;
        bool hit = TraceOcclusion(origin, dirJ, 0.0f, 1000.0f, hitT);
        occluded += hit ? 1.0f : 0.0f;
    }

    float shadow = 1.0f - (occluded / (float)kShadowSamples);

    // 真っ黒回避
    shadow = lerp(kMinShadow, 1.0f, shadow);

    return shadow;
}

///////////////////////////////////////////////////////////////////////////////
//                              main
///////////////////////////////////////////////////////////////////////////////
PixelShaderOutput main(VertexShaderOutput input) {
    PixelShaderOutput output;

    //================= UV 変換 & テクスチャ取得 =================
    float4 transformedUV = mul(float4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
    float4 textureColor  = gTexture.Sample(gSampler, transformedUV.xy);

    // アルベド：テクスチャ × マテリアル色
    float3 albedo = gMaterial.color.rgb * textureColor.rgb;
    float  alpha  = gMaterial.color.a   * textureColor.a;

    //================= アンリット =================
    if(gMaterial.enableLighting == 4) {
        if(alpha <= 0.01f) discard;
        output.color = float4(albedo, alpha);
        return output;
    }

    float3 normal = normalize(input.normal);
    float3 toEye  = normalize(cameraPosition - input.worldPosition);

    //================= ライト計算 =================
    float3 directionalDiffuse, directionalSpecular;
    ComputeDirectionalLight(normal, toEye, albedo, directionalDiffuse, directionalSpecular);

    float3 pointDiffuse, pointSpecular;
    ComputePointLight(normal, toEye, input.worldPosition, albedo, pointDiffuse, pointSpecular);

    //================= ソフトシャドウ（Directional: Inline Raytracing） =================
    float3 L = normalize(-gDirectionalLight.direction); // 表面→光源
    float shadow = ComputeDirectionalSoftShadow_RT(input.worldPosition, normal, L);

    // 影は Directional Light にのみ適用
    directionalDiffuse  *= shadow;
    directionalSpecular *= shadow;

    //================= 照明合成 =================
    float3 litColor = directionalDiffuse + directionalSpecular + pointDiffuse + pointSpecular;

    // 疑似環境光
    float3 ambient = albedo * 0.07f; 
    litColor += ambient;

    //================= 環境マップ =================
    if(gMaterial.isReflect) {
        float3 viewDir    = normalize(input.worldPosition - cameraPosition);
        float3 reflectDir = reflect(viewDir, normal);

        const float maxMipLevel = 7.0f;
        float mipLevel = saturate(gMaterial.roughness) * maxMipLevel;

        float3 envColor = gEnvironmentMap.SampleLevel(gSampler, reflectDir, mipLevel).rgb;
        litColor += envColor * gMaterial.environmentCoefficient;
    }

    //================= トーンマッピング + ガンマ補正 =================
    float3 finalColor = ApplyToneMappingAndGamma(litColor, 1.0f);

    //================= アルファ =================
    if(alpha <= 0.01f) discard;

    //================= 出力 =================
    output.color = float4(finalColor, alpha);
    return output;
}
