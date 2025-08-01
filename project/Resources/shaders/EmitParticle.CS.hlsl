#include "GpuParticle.hlsli"

struct EmitterData {
	float3 translate;
	float radius;
	uint count;
	float frequency;
	float frequencyTime;
	uint emit;
};

struct PerFrame {
	float time;
	float deltaTime;
};

static const uint kMaxParticles = 1024;

RWStructuredBuffer<Particle> gParticles : register(u0);
RWStructuredBuffer<int> gFreeCounter : register(u1);
ConstantBuffer<EmitterData> gEmitter : register(b0);
ConstantBuffer<PerFrame> gPerFrame : register(b1);

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

[numthreads(1, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID) {
	if(gEmitter.emit == 0)
		return;

	RandomGenerator generator;
	generator.seed = (DTid + gPerFrame.time) * gPerFrame.time;

	for(uint countIndex = 0; countIndex < gEmitter.count; ++countIndex) {
		int particleIndex;
		InterlockedAdd(gFreeCounter[0], 1, particleIndex);

		if(particleIndex < kMaxParticles) {
			Particle p;
			p.scale = generator.Generate3d();
			p.translate = generator.Generate3d();
			p.color.rgb = generator.Generate3d();
			p.color.a = 1.0f;
			p.lifeTime = 1.0f;
			p.currentTime = 0.0f;
			p.velocity = float3(0, 1, 0); // 適宜
			gParticles[particleIndex] = p;
		}
	}
}
