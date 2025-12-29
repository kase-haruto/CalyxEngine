#pragma once

////////////////////////////////////////////////////////////
//  include
////////////////////////////////////////////////////////////
/* engine */
#include <Engine/Renderer/Sprite/Sprite.h>
#include <Engine/scene/Base/BaseScene.h>

/* game */
#include <Game/Runtime/Engagement/EnemyEngagementService.h>

/* c++ */
#include <memory>

// fwd
class NumbersSprite;

/*---------------------------------------------------------
 *  クリアシーンクラス
 *  - クリア画面のシーン
 *--------------------------------------------------------*/
class ClearScene final
	: public BaseScene {
public:
	//================================================*/
	//  public methods
	//================================================*/
	/** \brief コンストラクタ*/
	ClearScene();
	~ClearScene() override;
	/**
	 * \brief シーン遷移ペイロード設定
	 */
	void Initialize() override;
	/**
	 * \brief 更新処理
	 * \param dt
	 */
	void Update(float dt) override;
	/**
	 * \brief 描画処理
	 */
	void Draw(ID3D12GraphicsCommandList*, class PipelineService*, RenderTargetType) override;
	/**
	 * \brief 終了処理
	 */
	void CleanUp() override;
	/**
	 * \brief アセットロード
	 */
	void LoadAssets() override;
	/**
	 * \brief シーン遷移ペイロード設定
	 * \param payload
	 */
	void SetPayload(const SceneTransitionPayload& payload);

private:
	//================================================*/
	//  private members
	//================================================*/

	/* objects ======================================================*/
	std::unique_ptr<Sprite>		   buttonSprite_	  = nullptr;
	std::unique_ptr<NumbersSprite> scoreSprite_		  = nullptr;
	std::unique_ptr<Sprite>		   clearSprite_		  = nullptr;
	std::unique_ptr<Sprite>		   resultScoreSprite_ = nullptr;
	int32_t						   finalScore_;

	/* runtime services =============================================*/
	bool  blinkState	= true; // 表示/非表示
	float blinkTimer	= 0.0f; // 経過時間
	float blinkInterval = 0.5f; // 反転間隔(秒)
};