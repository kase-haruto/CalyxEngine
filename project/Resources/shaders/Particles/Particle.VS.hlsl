#include "Particle.hlsli"

///////////////////////////////////////////////////////////////////////////////
//                            enums
///////////////////////////////////////////////////////////////////////////////
enum BillboardMode {
	Billboard_None = 0,
	Billboard_Full = 1,
	Billboard_AxisY = 2
};

///////////////////////////////////////////////////////////////////////////////
//                            structs
///////////////////////////////////////////////////////////////////////////////
struct VertexShaderInput {
	float4 position : POSITION0;
	float2 texcoord : TEXCOORD0;
	float3 normal : NORMAL0;
};

struct ParticleData {
	float3 position;
	float3 scale;
	float4 color;
	float3 rotation; // XYZ spin angles
	float3 alignDirection;
	uint alignToDirection;
};

float3 RotateAroundAxis(float3 v, float3 axis, float angle) {
	float s = sin(angle);
	float c = cos(angle);
	return v * c + cross(axis, v) * s + axis * dot(axis, v) * (1.0f - c);
}

struct Camera {
	float4x4 view;
	float4x4 projection;
	float4x4 viewProjection;
	float3 cameraPosition;
	float3 camRight;
	float3 camUp;
	float3 camForward;
};

struct BillboardParm {
	uint gBillboardMode;
	float3 gBillboardPad;
};

struct FadeParm {
	float fadeNear;
	float fadeFar;
	uint enabled;
	float fadePad;
};

///////////////////////////////////////////////////////////////////////////////
//                            cbuffers
///////////////////////////////////////////////////////////////////////////////
ConstantBuffer<Camera> gCamera : register(b0);
ConstantBuffer<BillboardParm> gBillboardParm : register(b2);
ConstantBuffer<FadeParm> gFadeParm : register(b3);

///////////////////////////////////////////////////////////////////////////////
//                            tables
///////////////////////////////////////////////////////////////////////////////
StructuredBuffer<ParticleData> gParticle : register(t0);

///////////////////////////////////////////////////////////////////////////////
//                            main
///////////////////////////////////////////////////////////////////////////////
VertexShaderOutput main(VertexShaderInput input,
						uint instanceID : SV_InstanceID) {
	ParticleData p = gParticle[instanceID];

	// ==========================================================
	//  ビルボード軸決定
	// ==========================================================
	float3 right;
	float3 up;
	float3 forward; // 3Dメッシュ対応のためForwardも必要

	if(gBillboardParm.gBillboardMode == Billboard_Full) {
		right = gCamera.camRight;
		up = gCamera.camUp;
		forward = gCamera.camForward;
	}
	else if(gBillboardParm.gBillboardMode == Billboard_AxisY) {
		float3 camDir = normalize(gCamera.cameraPosition - p.position);
		camDir.y = 0.0f;
		// カメラ方向(水平)
		if(length(camDir) < 0.001f)
			camDir = float3(0, 0, 1);
		else
			camDir = normalize(camDir);

		up = float3(0, 1, 0);
		right = normalize(cross(up, camDir));
		forward = cross(right, up); // 垂直なForwardを計算
	}
	else { // None
		right = float3(1, 0, 0);
		up = float3(0, 1, 0);
		forward = float3(0, 0, 1);
	}

	float3 rotatedRight = right;
	float3 rotatedUp = up;
	float3 rotatedForward = forward;

	if(p.alignToDirection != 0) {
		float3 dir = normalize(p.alignDirection);
		float2 projectedDir = float2(dot(dir, right), dot(dir, up));
		if(length(projectedDir) > 0.001f) {
			projectedDir = normalize(projectedDir);
			rotatedUp = right * projectedDir.x + up * projectedDir.y;
			rotatedRight = normalize(cross(rotatedUp, forward));
			rotatedForward = normalize(cross(rotatedRight, rotatedUp));
		}
	}

	rotatedUp = normalize(RotateAroundAxis(rotatedUp, rotatedRight, p.rotation.x));
	rotatedForward = normalize(RotateAroundAxis(rotatedForward, rotatedRight, p.rotation.x));

	rotatedRight = normalize(RotateAroundAxis(rotatedRight, rotatedUp, p.rotation.y));
	rotatedForward = normalize(RotateAroundAxis(rotatedForward, rotatedUp, p.rotation.y));

	rotatedRight = normalize(RotateAroundAxis(rotatedRight, rotatedForward, p.rotation.z));
	rotatedUp = normalize(RotateAroundAxis(rotatedUp, rotatedForward, p.rotation.z));

	// ==========================================================
	//  頂点計算
	// ==========================================================
	// inputのローカル座標にスケールを適用
	// (input.position.xyz はモデルの頂点データ)
	float3 offset = input.position.xyz * p.scale;

	// パーティクルの中心位置 + 回転した基底ベクトルに沿ったオフセット
	float3 worldPos = p.position
					+ rotatedRight * offset.x
					+ rotatedUp * offset.y
					+ rotatedForward * offset.z;

	// ==========================================================
	//  出力
	// ==========================================================
	VertexShaderOutput o;
	o.position = mul(float4(worldPos, 1.0f), gCamera.viewProjection);
	o.texcoord = input.texcoord; // 入力のUVをそのまま使用

	// ---- カメラ距離によるフェード ----
	float dist = length(worldPos - gCamera.cameraPosition);
	float cameraFade = 1.0f;
	if(gFadeParm.enabled != 0 && gFadeParm.fadeFar > gFadeParm.fadeNear) {
		cameraFade = saturate((dist - gFadeParm.fadeNear) / (gFadeParm.fadeFar - gFadeParm.fadeNear));
	}

	o.color = p.color;
	o.fade = cameraFade;
	return o;
}
