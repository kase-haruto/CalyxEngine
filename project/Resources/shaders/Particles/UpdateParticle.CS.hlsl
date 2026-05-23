#include "GpuParticle.hlsli"

ConstantBuffer<PerFrame> gPerFrame : register(b0);
ConstantBuffer<EmitterData> gEmitter : register(b1);
RWStructuredBuffer<Particle> gParticles : register(u0);
RWStructuredBuffer<int> gFreeListIndex : register(u1);
RWStructuredBuffer<int> gFreeList : register(u2);

float4 ApplyBlend4(float4 currentValue, float4 moduleValue, uint blend) {
	if(blend == 1) return currentValue + moduleValue;
	if(blend == 2) return currentValue * moduleValue;
	return moduleValue;
}

[numthreads(1024, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID) {
	uint particleIndex = DTid.x;
	if(particleIndex < kMaxParticles) {
		if(gParticles[particleIndex].isAlive == 0) {
			return;
		}

		if(gEmitter.gravityEnabled != 0) {
			gParticles[particleIndex].velocity += gEmitter.gravity * gPerFrame.deltaTime;
		}
		gParticles[particleIndex].translate += gParticles[particleIndex].velocity * gPerFrame.deltaTime;
		gParticles[particleIndex].currentTime += gPerFrame.deltaTime;
		float alpha = 1.0f - (gParticles[particleIndex].currentTime / max(gParticles[particleIndex].lifeTime, 0.01f));
		gParticles[particleIndex].color.a = saturate(alpha);

		float lifeT = gParticles[particleIndex].currentTime / max(gParticles[particleIndex].lifeTime, 0.01f);
		if(gEmitter.overLifeClamp != 0) {
			lifeT = saturate(lifeT);
		}
		if(gEmitter.overLifeInvert != 0) {
			lifeT = 1.0f - lifeT;
		}

		if(gEmitter.sizeLifeEnabled != 0) {
			float sizeT = ApplyGpuEase(gEmitter.sizeLifeEase, saturate(lifeT));
			float sizeFactor = (gEmitter.sizeLifeGrowing != 0) ? sizeT : (1.0f - sizeT);
			gParticles[particleIndex].scale = gParticles[particleIndex].initialScale * sizeFactor;
		}

		if(gEmitter.overLifeEnabled != 0) {
			float overT = ApplyGpuEase(gEmitter.overLifeEase, lifeT);
			float4 moduleValue = lerp(gEmitter.overLifeStart, gEmitter.overLifeEnd, overT);
			if(gEmitter.overLifeTarget == 0) {
				float4 scaleValue = ApplyBlend4(float4(gParticles[particleIndex].scale, 1.0f), moduleValue, gEmitter.overLifeBlend);
				gParticles[particleIndex].scale = scaleValue.xyz;
			}
			else if(gEmitter.overLifeTarget == 4) {
				gParticles[particleIndex].color = ApplyBlend4(gParticles[particleIndex].color, moduleValue, gEmitter.overLifeBlend);
			}
			else if(gEmitter.overLifeTarget == 5) {
				float4 alphaValue = ApplyBlend4(float4(gParticles[particleIndex].color.a, 0.0f, 0.0f, 0.0f), moduleValue.xxxx, gEmitter.overLifeBlend);
				gParticles[particleIndex].color.a = alphaValue.x;
			}
		}

		if(gParticles[particleIndex].currentTime >= gParticles[particleIndex].lifeTime) {
			gParticles[particleIndex].scale = float3(0.0f, 0.0f, 0.0f);
			gParticles[particleIndex].color.a = 0.0f;
			gParticles[particleIndex].isAlive = 0;

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
