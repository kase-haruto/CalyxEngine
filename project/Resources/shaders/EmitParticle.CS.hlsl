#include "GpuParticle.hlsli"

struct EmitterData {
	float3 translate;
	float radius;
	uint count;
	float frequency;
	float frequencyTime;
	uint emit;
};

float3 rand3dTo3d(float3 p) {
	p = float3(dot(p, float3(127.1, 311.7, 74.7)),
               dot(p, float3(269.5, 183.3, 246.1)),
               dot(p, float3(113.5, 271.9, 124.6)));
	return frac(sin(p) * 43758.5453);
}

float rand3dTo1d(float3 p) {
	return frac(sin(dot(p, float3(12.9898, 78.233, 37.719))) * 43758.5453);
}

class RandomGenerator {
	float3 seed;
	float3 Generate3d() {
		seed = rand3dTo3d(seed);
		return seed;
	}
	float Generate1d() {
		float result = rand3dTo1d(seed);
		seed.x = result;
		return result;
	}
};

ConstantBuffer<EmitterData> gEmitter : register(b0);
ConstantBuffer<PerFrame> gPerFrame : register(b1);

RWStructuredBuffer<Particle> gParticles : register(u0);
RWStructuredBuffer<int> gFreeListIndex : register(u1);
RWStructuredBuffer<int> gFreeList : register(u2);

// 球面上のランダムな位置を得る関数
float3 RandomOnSphere(RandomGenerator generator) {
	float u = generator.Generate1d() * 2.0f - 1.0f;
	float theta = generator.Generate1d() * 6.28318530718f; // 2π
	float sqrtOneMinusUSquared = sqrt(1.0f - u * u);
	return float3(
		sqrtOneMinusUSquared * cos(theta),
		sqrtOneMinusUSquared * sin(theta),
		u
	);
}

[numthreads(1024, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID) {
	uint globalIndex = DTid.x;

	if(gEmitter.emit == 0)
		return;

	if(globalIndex >= gEmitter.count)
		return;

	RandomGenerator generator;
	generator.seed = (DTid + gPerFrame.time) * gPerFrame.time;

	int freeListIndex;
	InterlockedAdd(gFreeListIndex[0], -1, freeListIndex);

	if(0 <= freeListIndex && freeListIndex < kMaxParticles) {
		Particle p;
		p.scale = float3(0.1f, 0.1f, 0.1f);
		
		// 球面上のランダムな位置に設定
		float3 sphereDir = RandomOnSphere(generator);
		p.translate = gEmitter.translate + sphereDir * gEmitter.radius;

		p.color.rgb = generator.Generate3d();
		p.color.a = 1.0f;
		p.lifeTime = 3.0;
		p.currentTime = 0.0f;

		// 上方向に移動する速度を設定
		p.velocity = generator.Generate3d();
        
		uint particleIndex = gFreeList[freeListIndex];
		gParticles[particleIndex] = p;
	}
	else {
		InterlockedAdd(gFreeListIndex[0], 1);
	}
}
