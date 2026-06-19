#pragma once
/* ========================================================================
/* include space
/* ===================================================================== */
/* game */

/* engine */
#include <Engine/scene/Base/BaseScene.h>
#include <Engine/Renderer/Sprite/Sprite.h>

/* ========================================================================
/* DemoScene
/* ===================================================================== */
class DemoScene final :
	public BaseScene{
public:
	//===================================================================*/
	//			public methods
	//===================================================================*/
	DemoScene();
	~DemoScene() override = default;

	/**
	 * \brief シーン初期化
	 */
	void Initialize()override;
	/**
	 * \brief シーン更新
	 * @param dt デルタタイム
	 */
	void Update(float dt)override;
	/**
	 * \brief シーン描画
	 * @param cmdLst
	 * @param psoService
	 */
	void Draw(ID3D12GraphicsCommandList* cmdLst, class PipelineService* psoService, IRenderTarget* )override;
	/**
	 * \brief シーン終了処理
	 */
	void CleanUp()override;
	void LoadAssets()override;
};
