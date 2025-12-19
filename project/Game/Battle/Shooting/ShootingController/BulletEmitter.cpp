#include "BulletEmitter.h"

void BulletEmitter::Update(float dt, const BulletEmitterContext& ctx){
	if (!sink_ || !aim_ || !pattern_) return;
	FireTick tick = scheduler_.Tick(dt);
	if (tick.triggers <= 0) return;

	AimContext a {ctx.origin, ctx.selfForward, ctx.targetPos, ctx.targetVel};
	CalyxMath::Vector3 base = aim_->GetForwardN(a);
	base = (base.LengthSquared() > 1e-8f) ? base.Normalize() : CalyxMath::Vector3 {0,0,1};

	PatternInput pin {base}; 
	PatternOutput pout;
	for (int i = 0; i < tick.triggers; ++i){
		pout.dirsN.clear();
		pattern_->Generate(pin, pout);
		if (pout.dirsN.empty()) continue;
		ShootBatch batch;
		batch.shots.reserve(pout.dirsN.size());
		for (const auto& d : pout.dirsN){
			batch.shots.push_back({ctx.origin, d, cfg_.shotSpeed, cfg_.tag});
		}
		sink_->Submit(batch);
	}
}

void BulletEmitter::SetPattern(IShootPattern* p){
	pattern_ = p;
	ownedPattern_.reset();
}