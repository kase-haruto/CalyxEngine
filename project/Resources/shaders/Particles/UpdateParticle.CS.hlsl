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

float3 PerlinFade(float3 t) {
	return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

uint PerlinHash(int3 cell) {
	uint3 p = asuint(cell);
	uint h = p.x * 374761393u + p.y * 668265263u + p.z * 2246822519u;
	h = (h ^ (h >> 13u)) * 1274126177u;
	return h ^ (h >> 16u);
}

float PerlinGradientDot(int3 cell,float3 delta) {
	uint h = PerlinHash(cell) & 15u;
	float u = h < 8u ? delta.x : delta.y;
	float v = h < 4u ? delta.y : ((h == 12u || h == 14u) ? delta.x : delta.z);
	return ((h & 1u) == 0u ? u : -u) + ((h & 2u) == 0u ? v : -v);
}

float PerlinNoise3D(float3 coordinate) {
	int3 cell = (int3)floor(coordinate);
	float3 local = coordinate - (float3)cell;
	float3 fade = PerlinFade(local);
	float n000 = PerlinGradientDot(cell + int3(0,0,0),local - float3(0,0,0));
	float n100 = PerlinGradientDot(cell + int3(1,0,0),local - float3(1,0,0));
	float n010 = PerlinGradientDot(cell + int3(0,1,0),local - float3(0,1,0));
	float n110 = PerlinGradientDot(cell + int3(1,1,0),local - float3(1,1,0));
	float n001 = PerlinGradientDot(cell + int3(0,0,1),local - float3(0,0,1));
	float n101 = PerlinGradientDot(cell + int3(1,0,1),local - float3(1,0,1));
	float n011 = PerlinGradientDot(cell + int3(0,1,1),local - float3(0,1,1));
	float n111 = PerlinGradientDot(cell + int3(1,1,1),local - float3(1,1,1));
	float4 nx = lerp(float4(n000,n010,n001,n011),float4(n100,n110,n101,n111),fade.x);
	float2 nxy = lerp(nx.xz,nx.yw,fade.y);
	return lerp(nxy.x,nxy.y,fade.z);
}

float PerlinFbm(float3 coordinate) {
	float value = 0.0f;
	float weight = 1.0f;
	float frequency = max(gEmitter.curlNoiseFrequency,0.0001f);
	float weightSum = 0.0f;
	uint octaves = clamp(gEmitter.curlNoiseOctaves,1u,4u);
	for(uint octave = 0; octave < octaves; ++octave) {
		value += PerlinNoise3D(coordinate * frequency) * weight;
		weightSum += weight;
		weight *= gEmitter.curlNoiseRoughness;
		frequency *= max(gEmitter.curlNoiseLacunarity,1.0f);
	}
	return value / max(weightSum,0.0001f);
}

float3 PerlinVectorPotential(float3 coordinate) {
	return float3(
		PerlinFbm(coordinate + float3(19.1f,33.4f,47.2f)),
		PerlinFbm(coordinate + float3(74.2f,12.8f,29.6f)),
		PerlinFbm(coordinate + float3(41.7f,91.3f,15.5f)));
}

float3 PerlinCurlNoise(float3 coordinate) {
	float epsilon = 0.01f / max(gEmitter.curlNoiseFrequency,0.0001f);
	float3 dx = float3(epsilon,0.0f,0.0f);
	float3 dy = float3(0.0f,epsilon,0.0f);
	float3 dz = float3(0.0f,0.0f,epsilon);
	float3 potentialXp = PerlinVectorPotential(coordinate + dx);
	float3 potentialXm = PerlinVectorPotential(coordinate - dx);
	float3 potentialYp = PerlinVectorPotential(coordinate + dy);
	float3 potentialYm = PerlinVectorPotential(coordinate - dy);
	float3 potentialZp = PerlinVectorPotential(coordinate + dz);
	float3 potentialZm = PerlinVectorPotential(coordinate - dz);
	float inverseDoubleEpsilon = 0.5f / epsilon;
	return float3(
		(potentialYp.z - potentialYm.z) - (potentialZp.y - potentialZm.y),
		(potentialZp.x - potentialZm.x) - (potentialXp.z - potentialXm.z),
		(potentialXp.y - potentialXm.y) - (potentialYp.x - potentialYm.x)) * inverseDoubleEpsilon;
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
		if(gEmitter.curlNoiseEnabled != 0) {
			float3 noiseCoordinate = gParticles[particleIndex].translate + gEmitter.curlNoiseOffset
				+ gEmitter.curlNoiseScrollSpeed * gPerFrame.time;
			float3 curlVelocity = PerlinCurlNoise(noiseCoordinate) * gEmitter.curlNoiseAmplitude;
			gParticles[particleIndex].translate += curlVelocity * gPerFrame.deltaTime;
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
