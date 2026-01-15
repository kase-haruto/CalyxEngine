#pragma once
#include <memory>
#include <vector>

// engine
#include "Data/Game/Config/Player/PlayerDangerSceneConfig.h"
#include "Game/3dObject/Actor/Bullet/Container/BulletContainer.h"
#include "Game/3dObject/Actor/Player/Context/PlayerContext.h"

#include <Engine/Foundation/Math/Vector3.h>
#include <Engine/Renderer/Sprite/Sprite.h>

class Player;
class PlayerDodge;
class EnemyDirectory;
class EnemyBulletContainer;
class BaseBullet;

/**
 * \brief プレイヤー危険察知クラス
 */
class PlayerDangerSense {
public:
	//=====================================================================*/
	// Public Methods
	//=====================================================================*/
	PlayerDangerSense();
	~PlayerDangerSense();

	/** \brief 初期化
	 * \param ctx コンテキスト
	 * \param cfg 設定
	 */
	void Initialize(const PlayerStateContext& ctx);
	/** \brief 更新
	 * \param dt デルタタイム
	 */
	void Update(float dt);
	/** \brief 敵ディレクトリ設定
	 * \param dir 敵ディレクトリ
	 */
	void SetEnemyDirectory(EnemyDirectory* dir) { dir_ = dir; }
	/**
	 * \brief 危険UIスプライト取得
	 * \return スプライトポインタ
	 */
	Sprite* GetUiSprite() const { return cue_.get(); }
	// accessor
	const DangerSenseConfig& GetConfig() const { return cfg_; }
	void                     SetConfig(const DangerSenseConfig& c) { cfg_ = c; }
	/** \brief 弾コンテナ追加
	 * \param container 弾コンテナポインタ
	 */
	void AddBulletContainer(const BulletContainer* container);
	/**
	 * \brief デバッグGUI表示
	 */
	void ShowGui();
	/**
	 * \brief パラメータセーブ
	 */
	void SaveParam();
	/**
	 * \brief パラメータロード
	 */
	void LoadParam();

private:
	//=====================================================================*/
	// Private Methods
	//=====================================================================*/
	/** \brief 近距離に危険があるか計算
	 * \param outPlayerPos プレイヤー位置出力先
	 * \return 危険があるか
	 */
	bool ComputeDangerNearby(CalyxMath::Vector3& outPlayerPos) const;
	/** \brief 危険結果適用
	 * \param danger 危険があるか
	 * \param playerPos プレイヤー位置
	 */
	void ApplyDangerResult(bool danger,const CalyxMath::Vector3& playerPos);

private:
	//=====================================================================*/
	// Private Variables
	//=====================================================================*/
	PlayerStateContext ctx_;
	EnemyDirectory*                     dir_   = nullptr;
	std::vector<const BulletContainer*> bulletContainers_;
	float                               dangerHold_ = 0.0f;

	DangerSenseConfig cfg_{};

	std::unique_ptr<Sprite> cue_;

	// スキャン間引き
	int frameCounter_ = 0;

	bool lastDanger_ = false;
};