#include "FireScheduler.h"
FireTick FireScheduler::Tick(float dt){
	FireTick ft{};
	if (useBurst){
		accum += dt*shotsPerSec;
		if (accum>=1.0f){ accum-=1.0f; burstsLeft=burstsPerTrigger; burstTimer=0.0f; }
		if (burstsLeft>0){
			burstTimer -= dt;
			if (burstTimer<=0.0f){ --burstsLeft; ft.triggers++; burstTimer = burstIntervalSec; }
		}
	}else{
		accum += dt*shotsPerSec;
		while (accum>=1.0f){ accum-=1.0f; ft.triggers++; }
	}
	return ft;
}
