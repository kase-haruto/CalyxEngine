#include "Object3D.hlsli"

///////////////////////////////////////////////////////////////////////////////
//                            structs
///////////////////////////////////////////////////////////////////////////////
struct VertexShaderInput {
	float4 position : POSITION0;
	float2 texcoord : TEXCOORD0;
	float3 normal : NORMAL0;
};

struct TransformationMat {
	float4x4 World;
	float4x4 WorldInverseTranspose;
};

struct BillboardParms {
	uint mode; // 0=None, 1=Full, 2=AxisY
	float3 pad; // 16B アライン用
};

///////////////////////////////////////////////////////////////////////////////
//                            tables
///////////////////////////////////////////////////////////////////////////////
StructuredBuffer<TransformationMat> gTransMat : register(t0);
StructuredBuffer<BillboardParms> gBillboard : register(t1);

static const float3 WORLD_UP = float3(0, 1, 0);

///////////////////////////////////////////////////////////////////////////////
//                            main
///////////////////////////////////////////////////////////////////////////////
VertexShaderOutput main(VertexShaderInput input, uint instancedId : SV_InstanceID) {
	VertexShaderOutput o;
	const TransformationMat tm = gTransMat[instancedId];
	const BillboardParms bp = gBillboard[instancedId];

    // === 通常描画（ビルボード無効） ===
	if(bp.mode == 0) {
		float4 worldPos = mul(input.position, tm.World);
		o.position = mul(worldPos, ViewProjection);
		o.worldPosition = worldPos.xyz;
		o.texcoord = input.texcoord;
		o.normal = normalize(mul(input.normal, (float3x3)tm.WorldInverseTranspose));
		return o;
	}

    // === ここから 3Dメッシュ用ビルボード ===
    // 中心（平行移動）
	const float3 centerWS = tm.World[3].xyz;

    // 上3x3（回転×スケール）
	const float3x3 M = (float3x3)tm.World;

    // 非一様スケールを抽出（軸長）
	const float3 basisX = mul(float3(1, 0, 0), M);
	const float3 basisY = mul(float3(0, 1, 0), M);
	const float3 basisZ = mul(float3(0, 0, 1), M);
	float Sx = max(length(basisX), 1e-6);
	float Sy = max(length(basisY), 1e-6);
	float Sz = max(length(basisZ), 1e-6);

    // ビルボード基底（R:右=X, U:上=Y, F:前=Z）
	float3 R, U, F;

	if(bp.mode == 1) {
    // --- フル正対（LH）---
		F = normalize(cameraPosition - centerWS); // Z
		float3 Uref = normalize(camUp);

    // LH: R = F × Up
		R = normalize(cross(F, Uref));

		if(length(R) < 1e-4) {
			Uref = float3(0, 1, 0);
			R = normalize(cross(F, Uref));
			if(length(R) < 1e-4) {
				Uref = float3(0, 0, 1);
				R = normalize(cross(F, Uref));
			}
		}

    // LH: U = R × F
		U = normalize(cross(R, F));
	}
	else // bp.mode == 2 : AxisY
	{
		float3 toCam = cameraPosition - centerWS;
		toCam.y = 0.0;
		F = (length(toCam) > 1e-4) ? normalize(toCam) : float3(0, 0, 1); // 前方(Z)
		U = WORLD_UP; // 上(Y)

    // LH: 右ベクトル = 前 × 上
		R = normalize(cross(F, U));
		if(length(R) < 1e-4)
			R = float3(1, 0, 0);

    // LH: 上ベクトル再直交化
		U = normalize(cross(R, F));
	}


    // ---- パリティ（巻き方向）補正：負スケール対応 ----
	const float detW = determinant((float3x3)tm.World);
	if(detW < 0.0f) {
        // いずれか1軸を反転（ここでは X を反転）
		R = -R;
	}

    // 位置： center + (local.x*Sx)*R + (local.y*Sy)*U + (local.z*Sz)*F
	const float3 local = input.position.xyz;
	const float3 worldPos3 =
        centerWS + local.x * Sx * R
                 + local.y * Sy * U
                 + local.z * Sz * F;

    // 法線： n' = normalize( n.x*(R/Sx) + n.y*(U/Sy) + n.z*(F/Sz) )
	const float3 nLocal = input.normal;
	const float3 nWorld = normalize(nLocal.x * (R / Sx)
                                  + nLocal.y * (U / Sy)
                                  + nLocal.z * (F / Sz));

    // 出力
	o.position = mul(float4(worldPos3, 1), ViewProjection);
	o.worldPosition = worldPos3;
	o.texcoord = input.texcoord;
	o.normal = nWorld;
	return o;
}
