struct Particle {
	float3 translate;
	float3 scale;
	float lifeTime;
	float3 velocity;
	float currentTime;
	float4 color;
	uint isAlive;
	float3 initialScale;
};

struct PerFrame {
	float time;
	float deltaTime;
};

struct EmitterData {
	float3 translate;
	float radius;
	uint count;
	float frequency;
	float frequencyTime;
	uint emit;
	float3 scale;
	float lifeTime;
	float3 velocity;
	float angle;
	float4 color;
	float3 shapeSize;
	uint shape;
	float4 rotation;
	float3 gravity;
	uint gravityEnabled;
	float4 overLifeStart;
	float4 overLifeEnd;
	uint overLifeTarget;
	uint overLifeBlend;
	uint overLifeEase;
	uint overLifeEnabled;
	uint overLifeClamp;
	uint overLifeInvert;
	uint sizeLifeEnabled;
	uint sizeLifeGrowing;
	uint sizeLifeEase;
	float _pad1;
};

float ApplyGpuEase(uint ease, float t) {
	t = saturate(t);
	if(ease == 1) return t * t;
	if(ease == 2) return 1.0f - (1.0f - t) * (1.0f - t);
	if(ease == 3) return (t < 0.5f) ? 2.0f * t * t : 1.0f - pow(-2.0f * t + 2.0f, 2.0f) * 0.5f;
	if(ease == 4) return t * t * t;
	if(ease == 5) return 1.0f - pow(1.0f - t, 3.0f);
	if(ease == 6) return (t < 0.5f) ? 4.0f * t * t * t : 1.0f - pow(-2.0f * t + 2.0f, 3.0f) * 0.5f;
	return t;
}



static const uint kMaxParticles = 262144;
