#include "GpuParticle.hlsli"

ConstantBuffer<PerFrame> gPerFrame : register(b0);
RWStructuredBuffer<Particle> gParticles : register(u0);
RWStructuredBuffer<int> gFreeListIndex : register(u1);
RWStructuredBuffer<int> gFreeList : register(u2);

[numthreads(1024, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID) {
	uint particleIndex = DTid.x;
	if(particleIndex < kMaxParticles) {
		if(gParticles[particleIndex].color.a != 0) {
			gParticles[particleIndex].translate += gParticles[particleIndex].velocity * gPerFrame.deltaTime;
			gParticles[particleIndex].currentTime += gPerFrame.deltaTime;
			float alpha = 1.0f - (gParticles[particleIndex].currentTime / gParticles[particleIndex].lifeTime);
			gParticles[particleIndex].color.a = saturate(alpha);
		}
		else if(gParticles[particleIndex].color.a == 0) {
			gParticles[particleIndex].scale = float3(0.0f, 0.0f, 0.0f);

			int freeListIndex;
			InterlockedAdd(gFreeListIndex[0], 1, freeListIndex);

			if(freeListIndex < kMaxParticles) {
				// 正しく死んだparticleIndexを格納
				gFreeList[freeListIndex] = particleIndex;
			}
			else {
				InterlockedAdd(gFreeListIndex[0], -1);
			}
		}
	}
}