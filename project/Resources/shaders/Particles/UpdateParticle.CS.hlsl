#include "GpuParticle.hlsli"

ConstantBuffer<PerFrame> gPerFrame : register(b0);
ConstantBuffer<EmitterData> gEmitter : register(b1);
RWStructuredBuffer<Particle> gParticles : register(u0);
RWStructuredBuffer<int> gFreeListIndex : register(u1);
RWStructuredBuffer<int> gFreeList : register(u2);
Texture2D<float4> gNoiseMotionTexture : register(t0);
SamplerState gNoiseMotionSampler : register(s0);

float4 ApplyBlend4(float4 currentValue, float4 moduleValue, uint blend) {
	if(blend == 1) return currentValue + moduleValue;
	if(blend == 2) return currentValue * moduleValue;
	return moduleValue;
}

void SpawnUnitTrailSample(Particle source,float3 position) {
	int freeListIndex;
	InterlockedAdd(gFreeListIndex[0],-1,freeListIndex);
	if(0 <= freeListIndex && freeListIndex < kMaxParticles) {
		Particle trail = source;
		trail.translate = position;
		trail.scale = source.scale * gEmitter.unitTrailScale;
		trail.initialScale = trail.scale;
		trail.velocity = 0.0f;
		trail.lifeTime = max(gEmitter.unitTrailLifetime,0.01f);
		trail.currentTime = 0.0f;
		trail.isAlive = 1;
		trail.trailSource = 0;
		trail.trailRemainder = 0.0f;
		uint trailIndex = gFreeList[freeListIndex];
		gParticles[trailIndex] = trail;
	} else {
		InterlockedAdd(gFreeListIndex[0],1);
	}
}

[numthreads(1024, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID) {
	uint particleIndex = DTid.x;
	if(particleIndex < kMaxParticles) {
		if(gParticles[particleIndex].isAlive == 0) {
			return;
		}

		if(gParticles[particleIndex].trailSource != 0) {
			float3 previousPosition = gParticles[particleIndex].translate;
			if(gEmitter.gravityEnabled != 0) {
				gParticles[particleIndex].velocity += gEmitter.gravity * gPerFrame.deltaTime;
			}
			gParticles[particleIndex].translate += gParticles[particleIndex].velocity * gPerFrame.deltaTime;
			if(gEmitter.noiseMotionEnabled != 0) {
				float3 noisePosition = gParticles[particleIndex].translate * gEmitter.noiseMotionFrequency;
				float2 noiseScroll = gEmitter.noiseMotionScrollSpeed * gPerFrame.time;
				float noiseX = gNoiseMotionTexture.SampleLevel(gNoiseMotionSampler,noisePosition.yz + noiseScroll,0.0f).r;
				float noiseY = gNoiseMotionTexture.SampleLevel(gNoiseMotionSampler,noisePosition.zx + noiseScroll,0.0f).g;
				float noiseZ = gNoiseMotionTexture.SampleLevel(gNoiseMotionSampler,noisePosition.xy + noiseScroll,0.0f).b;
				float3 noiseDirection = float3(noiseX,noiseY,noiseZ) * 2.0f - 1.0f;
				gParticles[particleIndex].translate += noiseDirection * gEmitter.noiseMotionStrength * gPerFrame.deltaTime;
			}

			if(gEmitter.unitTrailEnabled != 0) {
				float spacing = max(gEmitter.unitTrailSpacing,0.001f);
				float distanceMoved = length(gParticles[particleIndex].translate - previousPosition);
				float accumulated = gParticles[particleIndex].trailRemainder + distanceMoved;
				uint requestedCount = (uint)(accumulated / spacing);
				uint sampleCount = min(requestedCount,min(gEmitter.unitTrailMaxSamplesPerFrame,32u));
				float firstDistance = spacing - gParticles[particleIndex].trailRemainder
					+ float(requestedCount - sampleCount) * spacing;
				for(uint sampleIndex = 0; sampleIndex < sampleCount; ++sampleIndex) {
					float sampleDistance = firstDistance + float(sampleIndex) * spacing;
					float t = distanceMoved > 0.000001f ? saturate(sampleDistance / distanceMoved) : 0.0f;
					SpawnUnitTrailSample(gParticles[particleIndex],lerp(previousPosition,gParticles[particleIndex].translate,t));
				}
				gParticles[particleIndex].trailRemainder = fmod(accumulated,spacing);
			} else {
				gParticles[particleIndex].trailRemainder = 0.0f;
			}
		}
		gParticles[particleIndex].currentTime += gPerFrame.deltaTime;
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

			int previousFreeListIndex;
			InterlockedAdd(gFreeListIndex[0],1,previousFreeListIndex);
			int returnedFreeListIndex = previousFreeListIndex + 1;

			if(0 <= returnedFreeListIndex && returnedFreeListIndex < kMaxParticles) {
				// 正しく死んだparticleIndexを格納
				gFreeList[returnedFreeListIndex] = particleIndex;
			}
			else {
				InterlockedAdd(gFreeListIndex[0], -1);
			}
		}
	}
}
