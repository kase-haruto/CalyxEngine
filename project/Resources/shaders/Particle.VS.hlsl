#include"Particle.hlsli"

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
};

struct Camera {
	float4x4 view;
	float4x4 projection;
	float4x4 viewProjection;
	float3 cameraPosition;
	float3 camRight; // ViewMatrixのX列
	float3 camUp; // ViewMatrixのY列
};

///////////////////////////////////////////////////////////////////////////////
//                            cbuffers
///////////////////////////////////////////////////////////////////////////////
ConstantBuffer<Camera> gCamera : register(b0);

///////////////////////////////////////////////////////////////////////////////
//                            tables
///////////////////////////////////////////////////////////////////////////////
StructuredBuffer<ParticleData> gParticle : register(t0);

///////////////////////////////////////////////////////////////////////////////
//                            main (shader内でビルボード処理
///////////////////////////////////////////////////////////////////////////////
VertexShaderOutput main(uint vertexID : SV_VertexID,
                        uint instanceID : SV_InstanceID) {
	ParticleData p = gParticle[instanceID];

    // 頂点の位置（TRIANGLESTRIP順: 左下, 左上, 右下, 右上）
	float2 corner;
	float2 texcoord;
	switch (vertexID) {
		case 0:
			corner = float2(-0.5f, -0.5f);
			texcoord = float2(0.0f, 1.0f);
			break;
		case 1:
			corner = float2(-0.5f, 0.5f);
			texcoord = float2(0.0f, 0.0f);
			break;
		case 2:
			corner = float2(0.5f, -0.5f);
			texcoord = float2(1.0f, 1.0f);
			break;
		case 3:
			corner = float2(0.5f, 0.5f);
			texcoord = float2(1.0f, 0.0f);
			break;
	}

	float2 offset = float2(corner.x * p.scale.x, corner.y * p.scale.y);

	float3 worldPos = p.position
                + gCamera.camRight * offset.x
                + gCamera.camUp * offset.y;

	VertexShaderOutput o;
	o.position = mul(float4(worldPos, 1.0f), gCamera.viewProjection);
	o.texcoord = texcoord;
	o.color = p.color;
	return o;
}
