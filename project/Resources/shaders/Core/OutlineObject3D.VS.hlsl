#include "Object3D.hlsli"

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
	uint mode;
	float3 pad;
};

cbuffer OutlineConstants : register(b2) {
	float4 outlineColor;
	float outlineThickness;
}

StructuredBuffer<TransformationMat> gTransMat : register(t0);
StructuredBuffer<BillboardParms> gBillboard : register(t1);

static const float3 WORLD_UP = float3(0, 1, 0);

VertexShaderOutput main(VertexShaderInput input, uint instancedId : SV_InstanceID) {
	VertexShaderOutput o;
	const TransformationMat tm = gTransMat[instancedId];
	const BillboardParms bp = gBillboard[instancedId];

	float3 worldPos3;
	float3 nWorld;

	if(bp.mode == 0) {
		float4 worldPos = mul(input.position, tm.World);
		worldPos3 = worldPos.xyz;
		nWorld = normalize(mul(input.normal, (float3x3)tm.WorldInverseTranspose));
	} else {
		const float3 centerWS = tm.World[3].xyz;
		const float3x3 M = (float3x3)tm.World;
		const float3 basisX = mul(float3(1, 0, 0), M);
		const float3 basisY = mul(float3(0, 1, 0), M);
		const float3 basisZ = mul(float3(0, 0, 1), M);
		float Sx = max(length(basisX), 1e-6);
		float Sy = max(length(basisY), 1e-6);
		float Sz = max(length(basisZ), 1e-6);

		float3 R, U, F;
		if(bp.mode == 1) {
			F = normalize(cameraPosition - centerWS);
			float3 Uref = normalize(camUp);
			R = normalize(cross(F, Uref));
			if(length(R) < 1e-4) {
				Uref = float3(0, 1, 0);
				R = normalize(cross(F, Uref));
			}
			U = normalize(cross(R, F));
		} else {
			float3 toCam = cameraPosition - centerWS;
			toCam.y = 0.0;
			F = (length(toCam) > 1e-4) ? normalize(toCam) : float3(0, 0, 1);
			U = WORLD_UP;
			R = normalize(cross(F, U));
			if(length(R) < 1e-4) R = float3(1, 0, 0);
			U = normalize(cross(R, F));
		}

		if(determinant((float3x3)tm.World) < 0.0f) {
			R = -R;
		}

		const float3 local = input.position.xyz;
		worldPos3 = centerWS + local.x * Sx * R + local.y * Sy * U + local.z * Sz * F;
		const float3 nLocal = input.normal;
		nWorld = normalize(nLocal.x * (R / Sx) + nLocal.y * (U / Sy) + nLocal.z * (F / Sz));
	}

	worldPos3 += nWorld * outlineThickness;
	o.position = mul(float4(worldPos3, 1), ViewProjection);
	o.worldPosition = worldPos3;
	o.texcoord = input.texcoord;
	o.normal = nWorld;
	return o;
}
