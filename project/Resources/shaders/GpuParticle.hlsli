struct Particle {
	float3 translate;
	float3 scale;
	float lifeTime;
	float3 velocity;
	float currentTime;
	float4 color;
};

struct PerFrame {
	float time;
	float deltaTime;
};



static const uint kMaxParticles = 1048576 * 4;