#pragma once

struct FireTick { int triggers = 0; };

class FireScheduler {
public:
	FireTick Tick(float dt);
	
public:
	float shotsPerSec = 8.0f;

	bool  useBurst = false;
	int   burstsPerTrigger = 5;
	float burstIntervalSec = 0.08f;

	float accum = 0.0f;
	int burstsLeft = 0;
	float burstTimer = 0.0f;

};