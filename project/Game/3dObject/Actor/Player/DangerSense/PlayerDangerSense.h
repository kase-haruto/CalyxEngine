#pragma once
/* =========================================================================
   PlayerDangerSense - 近距離の敵弾がある間だけ回避UIを出し、
   その間に押せばジャスト成立にする軽量コンポーネント
   ========================================================================= */
#include <memory>
#include <optional>
#include <vector>

// engine
#include <Engine/Foundation/Math/Vector3.h>
#include <Engine/Renderer/Sprite/Sprite.h>

class Player;
class PlayerDodge;
class EnemyDirectory;
class BulletContainer;
class BaseBullet;

struct DangerSenseConfig {
	float playerInflate = 0.5f;
	float margin = 3.0f;
	float maxCheckDistance = 60.0f;
	int throttleFrames = 1;

	// UI
	std::string uiTex = "Textures/white1x1.png";
	Vector2 uiSize = { 64.0f, 64.0f };
};

class PlayerDangerSense {
public:
	PlayerDangerSense();
	~PlayerDangerSense();

	void Initialize(Player* owner, PlayerDodge* dodge, const DangerSenseConfig& cfg = {});
	void Update(float dt);

	// 敵一覧の供給（各敵が個別に BulletContainer を所有している構成に対応）
	void SetEnemyDirectory(EnemyDirectory* dir) { dir_ = dir; }

	// UIスプライト（外部の一括描画や GetAllSprites への合流用）
	Sprite* GetUiSprite() const { return cue_.get(); }

	// 設定の出し入れ（ImGui等で調整したい場合）
	const DangerSenseConfig& GetConfig() const { return cfg_; }
	void SetConfig(const DangerSenseConfig& c) { cfg_ = c; }

private:
	// 近距離に弾があるかを判定して返す（true = 警告）
	bool ComputeDangerNearby(Vector3& outPlayerPos) const;

	// UI更新と PlayerDodge へのフラグ連携
	void ApplyDangerResult(bool danger, const Vector3& playerPos);

private:
	Player* owner_ = nullptr;
	PlayerDodge* dodge_ = nullptr;
	EnemyDirectory* dir_ = nullptr;

	DangerSenseConfig cfg_{};

	std::unique_ptr<Sprite> cue_;

	// スキャン間引き
	int frameCounter_ = 0;

	bool lastDanger_ = false;
};
