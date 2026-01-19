#include "EnemyShootingAgent.h"
#include "../Enemy.h"

// game shooting details
#include <Game/Battle/Shooting/Details/AimProvider.h>
#include <Game/Battle/Shooting/Details/FireScheduler.h>
#include <Game/Battle/Shooting/ShootingController/EnemyShootingControllerSink.h>

EnemyShootingAgent::EnemyShootingAgent() = default;
EnemyShootingAgent::~EnemyShootingAgent()  = default;

void EnemyShootingAgent::Initialize(Enemy* owner) {
	owner_ = owner;
	param_.LoadParams();
}

void EnemyShootingAgent::ShowGui() {
	param_.ShowGui();
}

void EnemyShootingAgent::SetController(std::unique_ptr<EnemyShootingController> ctrl) {
	controller_ = std::move(ctrl);
}

void EnemyShootingAgent::SetTarget(const WorldTransform* tf) {
	targetTf_ = tf;
}

void EnemyShootingAgent::Update(float dt) {
	if(!owner_) return;

	BuildEmitterIfReady();

	// 下流コントローラ更新
	if(controller_) {
		controller_->SetGameplayEngaged(gameplayEngaged_);
		controller_->Update(dt);
	}

	// ゲームプレイ中のみ弾を出す
	if(!gameplayEngaged_ || !emitter_) return;

	if(auto* pat = emitter_->Pattern()) {
		pat->Advance(dt);
	}

	BulletEmitterContext cxt{};
	cxt.origin    = owner_->GetCenterPos();
	cxt.targetPos = targetTf_
						? targetTf_->GetWorldPosition()
						: owner_->GetWorldPosition();

	emitter_->Update(dt, cxt);
}

void EnemyShootingAgent::BuildEmitterIfReady() {
	if(emitter_) return;
	if(!controller_) return;
	if(!targetTf_) return;

	auto sink = std::make_unique<EnemyShootingControllerSink>(controller_.get());
	auto aim  = std::make_unique<AimAtTarget>();

	 FireScheduler sched;
	sched.shotsPerSec = param_.shotsPerSec;
	sched.useBurst    = false;

	BulletEmitterConfig cfg;
	cfg.tag = "enemy_homing";

	emitter_ = std::make_unique<BulletEmitter>(
		cfg, std::move(sink), std::move(aim), nullptr, sched);

	EnsurePatternBound();
}

void EnemyShootingAgent::EnsurePatternBound() {
	if(!emitter_) return;

	if(!pattern_ || patternKind_ != lastPatternKind_) {
		pattern_         = CreatePattern(patternKind_);
		lastPatternKind_ = patternKind_;
	}

	emitter_->SetPattern(pattern_.get());
}

/////////////////////////////////////////////////////////////////////////////////////////
//		ShootingParam
/////////////////////////////////////////////////////////////////////////////////////////
EnemyShootingAgent::ShootingParam::ShootingParam() {
	AddField("shotsPerSec", shotsPerSec).Category("Basic").Range(0.1f, 10.0f);
}

CalyxEngine::ParamPath EnemyShootingAgent::ShootingParam::GetParamPath() const {
	return {CalyxEngine::ParamDomain::Game, "EnemyShooting", "Battle/Shooting"};
}