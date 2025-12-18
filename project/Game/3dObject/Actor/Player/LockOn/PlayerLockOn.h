#pragma once
#include <memory>
#include <vector>

// engine
#include <Engine/Renderer/Sprite/Sprite.h>

// game
#include "Game/3dObject/Actor/Player/Context/PlayerContext.h"

#include <Game/3dObject/Actor/Enemy/Enemy.h>

// forward
class Player;
class Camera3d;

/**
 * \brief プレイヤーのロックオン機能を管理
 */
class PlayerLockOn {
public:
	PlayerLockOn();
	~PlayerLockOn();

	/**
	 * 初期化
	 * @param owner / Player 所有者
	 */
	void Initialize(const PlayerActionContext& ctx);
	/**
	 * 更新
	 * @param dt / デルタタイム
	 */
	void Update(float dt);
	/**
	 * \brief ロックオン要求
	 */
	void RequestLockOn();
	/**
	 * \brief ロックオン解除要求
	 */
	void RequestLockOnClear();
	/**
	 * \brief 敵リストのセット
	 * \param list 敵リスト
	 */
	void SetEnemyList(const std::list<std::shared_ptr<Enemy>>& list);
	/**
	 * \brief ロックオン中の敵リストを取得
	 * \return 敵リスト
	 */
	const std::vector<std::shared_ptr<Enemy>>& GetLockedTargets() const;
	/**
	 * \brief ロックオンマーカーのスプライトリストを取得
	 * \return スプライトリスト
	 */
	std::vector<Sprite*> GetSprites() const;

private:
	/**
	 * \brief 自動ロックオン更新
	 * \param dt デルタタイム
	 */
	void UpdateAutoLockOn(float dt);
	/**
	 * \brief 死んだ敵のロックオン解除
	 */
	void PurgeDeadLockedTargets();
	/**
	 * \brief ロックオンマーカーを取得
	 * \return スプライトポインタ
	 */
	std::unique_ptr<Sprite> AcquireMarker();
	/**
	 * \brief ロックオンマーカーを再利用プールに戻す
	 * \param s スプライトポインタ
	 */
	void RecycleMarker(std::unique_ptr<Sprite> s);
	/**
	 * \brief ロックオンマーカーの初期確保
	 * \param n 確保数
	 */
	void PrewarmMarkers(size_t n);

private:
	PlayerActionContext ctx_;
	std::list<std::shared_ptr<Enemy>> targets_;				//< 敵リスト
	std::vector<std::shared_ptr<Enemy>> lockedOnTargets_;	//< ロックオン中の敵リスト

	std::vector<std::unique_ptr<Sprite>> markerPool_;		//< ロックオンマーカー再利用プール
	std::vector<std::unique_ptr<Sprite>> lockOnSprites_;	//< ロックオンマーカー表示中リスト

	size_t maxLockOn_ = 5;					//< 最大ロックオン数

	float lockOnRadiusPx_        = 60.0f;	//< ロックオン表示半径(px)
	float lockOnAcquireRadiusPx_ = 60.0f;	//< ロックオン獲得半径(px)
	float lockOnReleaseRadiusPx_ = 150.0f;	//< ロックオン解除半径(px)
	float lockOnRefreshInterval_ = 0.15f;	//< ロックオン判定間隔（秒）
	float lockOnRefreshTimer_    = 0.0f;	//< ロックオン判定タイマー

};
