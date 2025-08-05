#include "GpuParticle.hlsli"

///////////////////////////////////////////////////////////////////////////////
//                            structs
///////////////////////////////////////////////////////////////////////////////

RWStructuredBuffer<Particle> gParticles : register(u0);
RWStructuredBuffer<int> gFreeListIndex : register(u1);
RWStructuredBuffer<int> gFreeList : register(u2);

///////////////////////////////////////////////////////////////////////////////
//                            main
///////////////////////////////////////////////////////////////////////////////
[numthreads(256, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID) {
	uint id = DTid.x;

	if(id == 0) {
		gFreeListIndex[0] = kMaxParticles - 1;
	}

	if(id < kMaxParticles) {
		gParticles[id] = (Particle)0;
		gFreeList[id] = id;
		gParticles[id].scale = float3(0.5f, 0.5f, 0.5f);
		gParticles[id].color = float4(1.0f, 1.0f, 1.0f, 0.0f);
		gParticles[id].lifeTime = 2.0f;
		gParticles[id].velocity = float3(0.0f, 0.0f, 0.0f);
	}
}
