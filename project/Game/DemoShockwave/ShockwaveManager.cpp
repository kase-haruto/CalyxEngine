#include "ShockwaveManager.h"
#include "Engine/Scene/Utility/SceneUtility.h"
#include "Game\Stage\Stage.h"

ShockwaveManager* ShockwaveManager::GetInstance() {
	static ShockwaveManager instance;
	return &instance;
}

void ShockwaveManager::Initialize(int poolSize) {
	// 既にあれば一度削除する
	for (auto& sw : pool_) {
		if (sw) sw->Destroy();
	}
	pool_.clear();
	pool_.reserve(poolSize);// メモリ確保
	for (int i = 0; i < poolSize; ++i) {
		auto sw = SceneAPI::Instantiate<Shockwave>("Torus.obj", "Shockwave");
		sw->Initialize();
		sw->Deactivate();
		sw->SetTexture("Textures/shockwave/shockwave.png");
		sw->SetCastShadow(false);
		sw->SetTransient(true);
		pool_.push_back(sw);
	}
}

void ShockwaveManager::Emit(const CalyxEngine::Vector3& pos, float scaleMultiplier) {
	// 非アクティブを探して再利用する
	for (auto& sw : pool_) {
		if (!sw->IsActive()) {
			sw->Activate(pos, scaleMultiplier);
			return;
		}
	}
}

void ShockwaveManager::Clear() {
	for (auto& sw : pool_) {
		sw->Deactivate();
	}
}

void ShockwaveManager::CheckTakeDamageForStage() {
	if(!stage_) return;
	for(auto& sw : pool_) {
		if(!sw->IsTakeDamageForStage()) continue;
		stage_->TakeDamage(1);
	}
}
