#pragma once
/* ========================================================================
/* include space
/* ===================================================================== */
// engine
#include <Engine/Scene/Base/IScene.h>
#include <Engine/Scene/Context/SceneContext.h>
#include <Engine/Extensions/SkyBox/SkyBox.h>
#include <Engine/Scene/Transitioner/SceneTransitionRequestor.h>

#include <Engine/Renderer/Sprite/SpriteRenderer.h>
#include <Engine/Renderer/Model/ModelRenderer.h>
#include <Engine/Renderer/Outline/OutlineRenderer.h>
#include <Engine/Graphics/Shadow/ShadowMap/ShadowMapSystem.h>

// c++
#include "Engine/Graphics/RenderTarget/Interface/IRenderTarget.h"
#include "Engine/Graphics/Shadow/ShadowMap/GpuResource/SceneDepthResource.h"

#include <string>

/*-----------------------------------------------------------------------------------------
 * BaseScene
 * - シーン基底クラス
 * - シーンの初期化、更新、描画、アセット管理のインターフェースを提供
 *---------------------------------------------------------------------------------------*/
class BaseScene :
	public IScene{
public:
	//===================================================================*/
	//			public methods
	//===================================================================*/
	/**
	 * \brief コンストラクタ
	 */
	BaseScene();
	/**
	 * \brief デストラクタ
	 */
	~BaseScene() override = default;

	/**
	 * \brief 初期化
	 */
	virtual void Initialize()override;
	/**
	 * \brief 更新処理
	 * \param dt デルタタイム
	 */
	virtual void Update([[maybe_unused]] float dt )override{}
	/**
	 * \brief 後更新処理
	 * \param cmdList コマンドリスト
	 * \param psoService PSOサービス
	 */
	virtual void PostUpdate(ID3D12GraphicsCommandList* cmdList,
							class PipelineService* psoService)override;
	/**
	 * \brief 描画処理
	 * \param cmdList コマンドリスト
	 * \param psoService PSOサービス
	 * \param rt レンダリングターゲット
	 */
	virtual void Draw(ID3D12GraphicsCommandList* cmdList,
					  class PipelineService* psoService,
					  IRenderTarget* rt)override;
	/**
	 * \brief スプライトのみ描画
	 * \param cmdList コマンドリスト
	 * \param psoService PSOサービス
	 */
	void DrawSpritesOnly(ID3D12GraphicsCommandList* cmdList,
					class PipelineService* psoService)override;
	/**
	 * \brief 終了処理
	 */
	void CleanUp()override{};
	/**
	 * \brief アセット読み込み
	 */
	virtual void LoadAssets()override{}

public:
	/**
	 * \brief シーン名を設定
	 * \param name シーン名
	 */
	void SetSceneName(const std::string& name){ sceneName_ = name; }
	/**
	 * \brief シーン名を取得
	 * \return シーン名
	 */
	const std::string& GetSceneName() const { return sceneName_; }
	/**
	 * \brief コンテキストを注入
	 * \param ctx シーンコンテキスト
	 */
	void InjectContext(SceneContext* ctx) override { sceneContext_ = ctx; }
	/**
	 * \brief シーンコンテキストを取得
	 * \return シーンコンテキスト
	 */
	SceneContext* GetSceneContext() const { return sceneContext_; }
	/**
	 * \brief 遷移リクエスタを設定
	 * \param requestor 遷移リクエスタ
	 */
	void SetTransitionRequestor(CalyxEngine::ISceneTransitionRequestor* requestor)override{
		transitionRequestor_ = requestor;
	}

	/**
	 * \brief 退場時処理
	 */
	virtual void OnExit()override{}
	/**
	 * \brief 入場時処理
	 */
	virtual void OnEnter()override{}
	/**
	 * \brief ペイロード受取
	 * \param payload ペイロード
	 */
	virtual void OnPayload(std::unique_ptr<CalyxEngine::IScenePayload> payload) {
		(void)payload;
	}

	ModelRenderer* GetModelRenderer() const { return modelRenderer_.get(); }
protected:
	//===================================================================*/
	//			protected member variables
	//===================================================================*/
	SceneContext* sceneContext_ = nullptr; //< シーンコンテキスト
	std::shared_ptr<SkyBox> skyBox_ = nullptr; //< スカイボックス
	std::string sceneName_ = "Scene"; //< シーン名

	//===================================================================*/
	//			renderers
	//===================================================================*/
	std::unique_ptr<SpriteRenderer> spriteRenderer_ = nullptr; //< スプライトレンダラ
	std::unique_ptr<ModelRenderer> modelRenderer_ = nullptr; //< モデルレンダラ
	std::unique_ptr<OutlineRenderer> outlineRenderer_ = nullptr; //< アウトラインレンダラ
	std::unique_ptr<CalyxEngine::ShadowMapSystem> shadowMapSystem_ = nullptr; //< シャドウマップシステム

	CalyxEngine::ISceneTransitionRequestor* transitionRequestor_ = nullptr; //< 遷移リクエスタ
};
