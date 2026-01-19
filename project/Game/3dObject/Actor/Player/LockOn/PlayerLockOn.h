#pragma once
#include <memory>
#include <vector>

// engine
#include <Engine/Renderer/Sprite/Sprite.h>

// game
#include "Data/Game/Config/Player/PlayerLockOnConfig.h"
#include "Game/3dObject/Actor/Player/Context/PlayerContext.h"

#include <Game/3dObject/Actor/Enemy/Enemy.h>

// forward
class Player;
class Camera3d;

/*-----------------------------------------------------------------------------------------
 * PlayerLockOn class
 * - プレイヤーのロックオン機能を管理するクラス
 * - ロックオン対象の管理とUIマーカー制御を担当する
 *---------------------------------------------------------------------------------------*/
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
	void SetEnemyList(const std::vector<std::shared_ptr<Enemy>>& list);
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
	/**
	 * \brief debug GUI 表示
	 */
	void ShowGui();
	/**
	 * \brief 設定の保存
	 */
	void SaveConfig();
	/**
	 * \brief 設定の読み込み
	 */
	void LoadConfig();

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
	std::vector<std::shared_ptr<Enemy>> targets_;				//< 敵リスト
	std::vector<std::shared_ptr<Enemy>> lockedOnTargets_;	//< ロックオン中の敵リスト

	std::vector<std::unique_ptr<Sprite>> markerPool_;		//< ロックオンマーカー再利用プール
	std::vector<std::unique_ptr<Sprite>> lockOnSprites_;	//< ロックオンマーカー表示中リスト

	PlayerLockOnConfig config_;

};
