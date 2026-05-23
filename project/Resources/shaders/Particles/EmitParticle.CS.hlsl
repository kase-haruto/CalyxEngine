#include "GpuParticle.hlsli"
#include "../Utility/Function/Random.hlsli"



ConstantBuffer<EmitterData> gEmitter : register(b0);
ConstantBuffer<PerFrame> gPerFrame : register(b1);

RWStructuredBuffer<Particle> gParticles : register(u0);
RWStructuredBuffer<int> gFreeListIndex : register(u1);
RWStructuredBuffer<int> gFreeList : register(u2);

float3 RandomUnitVector(inout RandomGenerator gen) {
	float3 v;
	v = gen.Generate3d(); // [-1, 1]
	if (length(v) < 0.0001f) {
		v = gen.Generate3d();
	}
	if (length(v) > 1.0f) {
		v = gen.Generate3d();
	}
	return normalize(v);
}

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

float3 RandomInSphere(RandomGenerator generator) {
	float3 dir = RandomOnSphere(generator);
	float r = pow(generator.Generate1d(), 1.0f / 3.0f);
	return dir * r;
}

float3 RandomInBox(RandomGenerator generator) {
	return generator.Generate3d() * 2.0f - 1.0f;
}

float3 RandomInCircle(RandomGenerator generator) {
	float theta = generator.Generate1d() * 6.28318530718f;
	float r = sqrt(generator.Generate1d());
	return float3(cos(theta) * r, 0.0f, sin(theta) * r);
}

float3 RandomInCone(RandomGenerator generator, float angleDeg) {
	float height = generator.Generate1d();
	float angleRad = radians(clamp(angleDeg, 0.0f, 89.0f));
	float maxRadius = height * tan(angleRad);
	float theta = generator.Generate1d() * 6.28318530718f;
	float r = sqrt(generator.Generate1d()) * maxRadius;
	return float3(cos(theta) * r, height, sin(theta) * r);
}

float3 GenerateSpawnOffset(RandomGenerator generator) {
	if(gEmitter.shape == 0) {
		return float3(0.0f, 0.0f, 0.0f);
	}
	if(gEmitter.shape == 2) {
		return RandomInCone(generator, gEmitter.angle) * gEmitter.radius;
	}
	if(gEmitter.shape == 3) {
		return RandomInCircle(generator) * gEmitter.radius;
	}
	if(gEmitter.shape == 4) {
		return RandomInBox(generator) * gEmitter.shapeSize;
	}
	return RandomInSphere(generator) * gEmitter.radius;
}

[numthreads(1024, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID) {
	uint globalIndex = DTid.x;

	if(gEmitter.emit == 0)
		return;

	if(globalIndex >= gEmitter.count)
		return;

	RandomGenerator generator;
	generator.InitSeed(DTid, gPerFrame.time);

	int freeListIndex;
	InterlockedAdd(gFreeListIndex[0], -1, freeListIndex);

	if(0 <= freeListIndex && freeListIndex < kMaxParticles) {
		Particle p;
		p.scale = gEmitter.scale;
		p.initialScale = gEmitter.scale;
		
		p.translate = gEmitter.translate + GenerateSpawnOffset(generator);

		p.color = gEmitter.color;
		p.color.rgb *= (generator.Generate3d()+1)*0.5;
		p.color.a = gEmitter.color.a;
		p.lifeTime = max(gEmitter.lifeTime, 0.01f);
		p.currentTime = 0.0f;
		p.isAlive = 1;

		p.velocity = gEmitter.velocity;
        
		uint particleIndex = gFreeList[freeListIndex];
		gParticles[particleIndex] = p;
	}
	else {
		InterlockedAdd(gFreeListIndex[0], 1);
	}
}
