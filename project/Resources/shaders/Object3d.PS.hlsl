#include "Object3D.hlsli"

///////////////////////////////////////////////////////////////////////////////
//                            structs
///////////////////////////////////////////////////////////////////////////////
struct Material {
    float4 color;
    int enableLighting;
    float4x4 uvTransform;
    float shiniess;

    bool isReflect;
    float environmentCoefficient;
    float roughness;
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

cbuffer PointLightConstants : register(b4) { PointLight gPointLight; }

cbuffer RaytracingShadowParamConstants : register(b5) {
    float gShadowRayEps;
    float gBaseAngularRadius;
    float gPenumbraStart;
    float gPenumbraScale;
    float gMinShadow;
};

///////////////////////////////////////////////////////////////////////////////
//                            tables
///////////////////////////////////////////////////////////////////////////////
TextureCube<float4> gEnvironmentMap : register(t1);
Texture2D<float>    gShadowMap      : register(t2);
RaytracingAccelerationStructure gRtScene : register(t3);
Texture2D<float4>   gTexture        : register(t0);

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

    float3 L = -gDirectionalLight.direction;
    
    // Half-Lambert用にsaturateしていない生のcos値を取得
    float rawNdotL = dot(normal, L);
    // 従来のランバート用に0-1にクランプした値も用意
    float NdotL = saturate(rawNdotL);

    if(gMaterial.enableLighting == 0) {
        // Half-Lambert: -1.0～1.0 の範囲を 0.0～1.0 に変換して二乗
        // saturateされた値を使うと、背面が 0 => 0.25 (明るいまま) になってしまうため、生のcos値を使う
        float halfLambert = pow(rawNdotL * 0.5f + 0.5f, 2.0f);
        diffuse = albedo * gDirectionalLight.color.rgb * halfLambert * gDirectionalLight.intensity;

        float3 H = normalize(L + toEye);
        float NdotH = saturate(dot(normal, H));
        specular = gDirectionalLight.color.rgb * pow(NdotH, gMaterial.shiniess) * gDirectionalLight.intensity;
    }
    else if(gMaterial.enableLighting == 1) {
        // 通常のランバート
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

    float NdotL = saturate(dot(normal, -lightDir));
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
//    Orthonormal Basis
///////////////////////////////////////////////////////////////////////////////
void BuildOrthonormalBasis(float3 n, out float3 t, out float3 b) {
    float3 up = (abs(n.z) < 0.999f) ? float3(0,0,1) : float3(0,1,0);
    t = normalize(cross(up, n));
    b = cross(n, t);
}

///////////////////////////////////////////////////////////////////////////////
//    ソフトシャドウ（距離依存）: Inline Raytracing
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

// 遮蔽物までの「正確な距離」を取得する (Closest Hit)
// ペナンブラの計算には「一番近い遮蔽物」までの距離が必要なため、ACCEPT_FIRST_HIT は使わない
bool FindClosestBlocker(float3 origin, float3 dir, float tMax, out float hitT) {
    RayDesc ray;
    ray.Origin    = origin;
    ray.Direction = dir;
    ray.TMin      = 0.0f;
    ray.TMax      = tMax;

    // First Hit フラグを削除 -> Closest Hit (最も近い交点) を探す
    RayQuery<
        RAY_FLAG_CULL_NON_OPAQUE |
        RAY_FLAG_CULL_BACK_FACING_TRIANGLES |
        RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES
    > q;

    q.TraceRayInline(
        gRtScene,
        RAY_FLAG_CULL_NON_OPAQUE |
        RAY_FLAG_CULL_BACK_FACING_TRIANGLES |
        RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES,
        0xFF,
        ray
    );

    q.Proceed();

    if (q.CommittedStatus() == COMMITTED_TRIANGLE_HIT) {
        hitT = q.CommittedRayT();
        return true;
    }

    hitT = -1.0f;
    return false;
}

//  単純な遮蔽判定を行う (Any Hit / Accept First Hit)
// ポアソンサンプリングなどの「見えているか否か」だけ知りたい場合はこちらが高速
bool CheckVisibility(float3 origin, float3 dir, float tMax) {
    RayDesc ray;
    ray.Origin    = origin;
    ray.Direction = dir;
    ray.TMin      = 0.0f;
    ray.TMax      = tMax;

    // 遮蔽されているかだけわかれば良いので First Hit で即終了する
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

    // ヒットしたらtrue
    return (q.CommittedStatus() == COMMITTED_TRIANGLE_HIT);
}

// サンプル数を距離で可変
int SelectShadowSampleCount(float tBlocker) {
    
	float dist = tBlocker - gPenumbraStart;
	float scale = max(gPenumbraScale, 1e-5f);

	// まだボケ始め (Scaleの20%未満) -> 4サンプル
	if (dist < scale * 0.2f) return 4;

	// ある程度ボケている (Scaleの80%未満) -> 8サンプル
	if (dist < scale * 0.8f) return 8;

	// 最大までボケている -> 16サンプル
	return 16;
}

float ComputeDirectionalSoftShadow_RT(float3 worldPos, float3 normal, float3 L) {

    // -----------------------------
    //  自己交差対策（cbuffer param 使用）
    // -----------------------------
    float3 origin = worldPos + normal * gShadowRayEps;

    // -----------------------------
    //  距離計測 (Closest Hit)
    // -----------------------------
    float tBlocker = 0.0f;
    
    // ここで FindClosestBlocker を使う（First Hitで終了させない）
    bool blocked = FindClosestBlocker(origin, L, 1000.0f, tBlocker);

    if (!blocked) {
        return 1.0f;
    }

    // -----------------------------
    //  blocker が極端に近い = ほぼ完全影
    // -----------------------------
    if (tBlocker < (gPenumbraStart * 0.5f)) {
        return gMinShadow;
    }

    // -----------------------------
    // 距離依存でぼけ量（角半径）を決定
    // -----------------------------
    float s = saturate((tBlocker - gPenumbraStart) / max(gPenumbraScale, 1e-5f));
    float angularRadius = gBaseAngularRadius * (1.0f + s * 2.0f); // 1x〜3x

    // 角半径が小さすぎるなら、多重サンプル不要（真っ暗領域の高速化）
    // 「濃い影ほど軽い」を実現するための安全弁
    if (angularRadius < (gBaseAngularRadius * 0.35f)) {
        return gMinShadow;
    }

    // -----------------------------
    //  適応サンプル数
    // -----------------------------
    int sampleCount = SelectShadowSampleCount(tBlocker);

    float3 T, B;
    BuildOrthonormalBasis(L, T, B);

    // 画面揺れがない固定ノイズ（worldPosベース）
    float rnd = Hash21(worldPos.xz * 17.0f + worldPos.yy * 3.0f);
    float ang = rnd * 6.2831853f;

    // -----------------------------
    //  Poisson サンプル
    // -----------------------------
    float occluded = 0.0f;

    [loop]
    for (int i = 0; i < sampleCount; ++i) {
        float2 d = Rotate2D(kPoisson16[i], ang) * angularRadius;
        float3 dirJ = normalize(L + T * d.x + B * d.y);

		// 判定だけなので早いものを使う
    	bool hit = CheckVisibility(origin, dirJ, 1000.0f);
        occluded += hit ? 1.0f : 0.0f;

        //全部ヒットしたら結果は確定
        if (occluded >= (float)sampleCount) {
            break;
        }
    }

    float shadow = 1.0f - (occluded / (float)sampleCount);

    // 真っ黒回避
    shadow = lerp(gMinShadow, 1.0f, shadow);

    return shadow;
}

///////////////////////////////////////////////////////////////////////////////
//                              main
///////////////////////////////////////////////////////////////////////////////
PixelShaderOutput main(VertexShaderOutput input) {
    PixelShaderOutput output;

    float4 transformedUV = mul(float4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
    float4 textureColor  = gTexture.Sample(gSampler, transformedUV.xy);

    float3 albedo = gMaterial.color.rgb * textureColor.rgb;
    float  alpha  = gMaterial.color.a   * textureColor.a;

    if(gMaterial.enableLighting == 4) {
        if(alpha <= 0.01f) discard;
        output.color = float4(albedo, alpha);
        return output;
    }

    float3 normal = normalize(input.normal);
    float3 toEye  = normalize(cameraPosition - input.worldPosition);

    float3 directionalDiffuse, directionalSpecular;
    ComputeDirectionalLight(normal, toEye, albedo, directionalDiffuse, directionalSpecular);

    float3 pointDiffuse, pointSpecular;
    ComputePointLight(normal, toEye, input.worldPosition, albedo, pointDiffuse, pointSpecular);

    //================= ソフトシャドウ（Directional: Inline Raytracing） =================
    float3 L = normalize(-gDirectionalLight.direction);

    float shadow = 1.0f;
    if (dot(normal, L) > 0.0f) {
        shadow = ComputeDirectionalSoftShadow_RT(input.worldPosition, normal, L);
    }

    directionalDiffuse  *= shadow;
    directionalSpecular *= shadow;

    float3 litColor = directionalDiffuse + directionalSpecular + pointDiffuse + pointSpecular;

    float3 ambient = albedo * 0.07f;
    litColor += ambient;

    if(gMaterial.isReflect) {
        float3 viewDir    = normalize(input.worldPosition - cameraPosition);
        float3 reflectDir = reflect(viewDir, normal);

        const float maxMipLevel = 7.0f;
        float mipLevel = saturate(gMaterial.roughness) * maxMipLevel;

        float3 envColor = gEnvironmentMap.SampleLevel(gSampler, reflectDir, mipLevel).rgb;
        litColor += envColor * gMaterial.environmentCoefficient;
    }

    float3 finalColor = ApplyToneMappingAndGamma(litColor, 1.0f);

    if(alpha <= 0.01f) discard;

    output.color = float4(finalColor, alpha);
    return output;
}