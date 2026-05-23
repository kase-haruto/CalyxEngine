#pragma once
#include <vector>
#include <memory>
#include "Engine/Foundation/Math/Vector3.h"
#include <Game/DemoShockwave/Shockwave.h>

class Stage;

class ShockwaveManager {
public:
	static ShockwaveManager* GetInstance();

	/// <summary> プールの初期化 </summary>
	/// <param name="poolSize"> 最大数 </param>
	void Initialize(int poolSize);

	/// <summary> 衝撃波を発生させる </summary>
	/// <param name="pos"> 位置 </param>
	/// <param name="scaleMultiplier"> 拡大の倍率 </param>
	void Emit(const CalyxEngine::Vector3& pos, float scaleMultiplier = 1.0f);

	/// <summary> 全てを非アクティブ </summary>
	void Clear();

	void CheckTakeDamageForStage();

	void SetStage(Stage* pStage) { stage_ = pStage; }

private:
	ShockwaveManager() = default;
	~ShockwaveManager() = default;
	ShockwaveManager(const ShockwaveManager&) = delete;
	ShockwaveManager& operator=(const ShockwaveManager&) = delete;

private:
	std::vector<std::shared_ptr<Shockwave>> pool_;
	Stage*									stage_;
};
