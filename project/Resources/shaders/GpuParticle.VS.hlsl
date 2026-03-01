#include "Particle.hlsli"
#include "GpuParticle.hlsli"

struct VSIn {
	float4 pos : POSITION0; // テンプレ用（クォッド）
	float2 uv : TEXCOORD0;
};

struct Camera {
	float4x4 view;
	float4x4 projection;
	float4x4 viewProjection;
	float3 cameraPosition;
	float3 camRight;
	float3 camUp;
};

ConstantBuffer<Camera> gCamera : register(b0);
StructuredBuffer<Particle> gParticles : register(t0);

VertexShaderOutput main(uint vid : SV_VertexID, uint iid : SV_InstanceID) {
	Particle p = gParticles[iid];

	// ── 無効パーティクルは即クリップ ───────────────
	//if (p.currentTime < 0.0f) {
	//	VertexShaderOutput o;
	//	o.position = float4(0, 0, 0, 0);
	//	o.texcoord = 0;
	//	o.color = 0;
	//	return o;
	//}

	// ── Billboard コーナー計算 (左下,左上,右下,右上) ──
	float2 corner, uv;
	switch(vid) {
		case 0:
			corner = float2(-0.5, -0.5);
			uv = float2(0, 1);
			break;
		case 1:
			corner = float2(-0.5, 0.5);
			uv = float2(0, 0);
			break;
		case 2:
			corner = float2(0.5, -0.5);
			uv = float2(1, 1);
			break;
		default:
			corner = float2(0.5, 0.5);
			uv = float2(1, 0);
			break;
	}

	float2 offset = corner * p.scale.xy;

	float3 worldPos =
		  p.translate
		+ gCamera.camRight * offset.x
		+ gCamera.camUp * offset.y;

	// ── 出力 ──────────────────────────────────────
	VertexShaderOutput o;
	o.position = mul(float4(worldPos, 1.0f), gCamera.viewProjection);
	o.texcoord = uv;

	// フェードアウト（寿命に応じ α 減衰）
	float lifeFade = saturate(1.0f - p.currentTime / p.lifeTime);
	o.color = float4(p.color.rgb, p.color.a * lifeFade);
	o.fade = 1.0f; // GPU パーティクルはカメラ距離ディザなし

	return o;
}