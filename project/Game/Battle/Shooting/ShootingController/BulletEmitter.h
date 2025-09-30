#pragma once
#include <memory>
#include <string>
#include <Engine/Foundation/Math/Vector3.h>
#include "Game/Battle/Shooting/Details/FireScheduler.h"
#include "Game/Battle/Shooting/Details/AimProvider.h"
#include "Game/Battle/Shooting/Pattern/IShootPattern.h"
#include <Game/Battle/Shooting/Details/IShootSink.h>

struct BulletEmitterConfig{
    float shotSpeed = 0.0f;   // 不要なら0
    std::string tag;          // 任意の識別
};
struct BulletEmitterContext{
    Vector3 origin{};
    Vector3 selfForward{0,0,1};
    Vector3 targetPos{};
    Vector3 targetVel{};
};

class BulletEmitter{
public:
    BulletEmitter(BulletEmitterConfig cfg,
                  std::unique_ptr<IShootSink> sink,
                  std::unique_ptr<IAimProvider> aim,
                  std::unique_ptr<IShootPattern> pattern,
                  FireScheduler scheduler)
    : cfg_(cfg), sink_(std::move(sink)), aim_(std::move(aim)),
      pattern_(std::move(pattern)), scheduler_(scheduler) {}

    void Update(float dt, const BulletEmitterContext& ctx){
        if (!sink_||!aim_||!pattern_) return;
        FireTick tick = scheduler_.Tick(dt);
        if (tick.triggers<=0) return;

        AimContext a{ctx.origin, ctx.selfForward, ctx.targetPos, ctx.targetVel};
        Vector3 base = aim_->GetForwardN(a);
        base = (base.LengthSquared()>1e-8f)? base.Normalize() : Vector3{0,0,1};

        PatternInput pin{base}; PatternOutput pout;
        for(int i=0;i<tick.triggers;++i){
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

    FireScheduler& Scheduler(){ return scheduler_; }
    IShootPattern* Pattern(){ return pattern_.get(); }
    IAimProvider* Aim(){ return aim_.get(); }

private:
    BulletEmitterConfig cfg_;
    std::unique_ptr<IShootSink>   sink_;
    std::unique_ptr<IAimProvider> aim_;
    std::unique_ptr<IShootPattern>     pattern_;
    FireScheduler                 scheduler_;
};