#include "GpuParticle.hlsli"

///////////////////////////////////////////////////////////////////////////////
//                            structs
///////////////////////////////////////////////////////////////////////////////

static const uint kMaxParticles = 1024;

///////////////////////////////////////////////////////////////////////////////
//                            main
///////////////////////////////////////////////////////////////////////////////
RWStructuredBuffer<Particle> gParticles : register(u0);
RWStructuredBuffer<int> gFreeCounter : register(u1);

[numthreads(1024, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID) {
	uint id = DTid.x;

	if(id == 0) {
		gFreeCounter[0] = 0; // カウンター初期化
	}

	if(id < kMaxParticles) {
		gParticles[id] = (Particle)0;
		gParticles[id].scale = float3(0.5f, 0.5f, 0.5f);
		gParticles[id].color = float4(1.0f, 1.0f, 1.0f, 0.0f);
	}
}
