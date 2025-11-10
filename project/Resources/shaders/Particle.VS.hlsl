#include "Particle.hlsli"

///////////////////////////////////////////////////////////////////////////////
//                            enums
///////////////////////////////////////////////////////////////////////////////
enum BillboardMode {
	Billboard_None	= 0,
	Billboard_Full	= 1,
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
	float  rotation; // Z軸スピン角
};

struct Camera {
	float4x4 view;
	float4x4 projection;
	float4x4 viewProjection;
	float3	 cameraPosition;
	float3	 camRight;
	float3	 camUp;
	float3	 camForward;
};

struct BillboardParm {
	uint   gBillboardMode;
	float3 pad;
};

///////////////////////////////////////////////////////////////////////////////
//                            cbuffers
///////////////////////////////////////////////////////////////////////////////
ConstantBuffer<Camera>		  gCamera : register(b0);
ConstantBuffer<BillboardParm> gBillboardParm : register(b2);

///////////////////////////////////////////////////////////////////////////////
//                            tables
///////////////////////////////////////////////////////////////////////////////
StructuredBuffer<ParticleData> gParticle : register(t0);

///////////////////////////////////////////////////////////////////////////////
//                            main
///////////////////////////////////////////////////////////////////////////////
VertexShaderOutput main(uint vertexID : SV_VertexID,
						uint instanceID : SV_InstanceID) {
	ParticleData p = gParticle[instanceID];

	// 四隅
	float2 corner;
	float2 texcoord;
	switch(vertexID) {
	case 0:
		corner	 = float2(-0.5, -0.5);
		texcoord = float2(0, 1);
		break;
	case 1:
		corner	 = float2(-0.5, 0.5);
		texcoord = float2(0, 0);
		break;
	case 2:
		corner	 = float2(0.5, -0.5);
		texcoord = float2(1, 1);
		break;
	case 3:
		corner	 = float2(0.5, 0.5);
		texcoord = float2(1, 0);
		break;
	}

	// ==========================================================
	//  ビルボード軸決定
	// ==========================================================
	float3 right;
	float3 up;

	if(gBillboardParm.gBillboardMode == Billboard_Full) {
		right = gCamera.camRight;
		up	  = gCamera.camUp;
	} else if(gBillboardParm.gBillboardMode == Billboard_AxisY) {
		float3 camDir = normalize(gCamera.cameraPosition - p.position);
		camDir.y	  = 0.0f;
		camDir		  = normalize(camDir);
		right		  = normalize(cross(float3(0, 1, 0), camDir));
		up			  = float3(0, 1, 0);
	} else { // None
		right = float3(1, 0, 0);
		up	  = float3(0, 1, 0);
	}

	// ==========================================================
	//  Z軸スピン回転（ローカル平面上で right/up を回転）
	// ==========================================================
	float  s			= sin(p.rotation);
	float  c			= cos(p.rotation);
	float3 rotatedRight = right * c + up * s;
	float3 rotatedUp	= up * c - right * s;

	// ==========================================================
	//  頂点オフセット計算
	// ==========================================================
	float2 offset = corner * p.scale.xy;

	float3 worldPos = p.position + rotatedRight * offset.x + rotatedUp * offset.y;

	// ==========================================================
	//  出力
	// ==========================================================
	VertexShaderOutput o;
	o.position = mul(float4(worldPos, 1.0f), gCamera.viewProjection);
	o.texcoord = texcoord;
	o.color	   = p.color;
	return o;
}
