#pragma
/* ========================================================================
/* include space
/* ===================================================================== */
// engine
#include <Engine/Application/Platform/WinApp.h>
#include <Engine/Application/System/PlaySession.h>
#include <Engine/Application/System/System.h>
#include <Engine/Application/UI/EngineUI/Core/EngineUICore.h>
#include <Engine/Editor/Collection/EditorCollection.h>
#include <Engine/Graphics/Core/GraphicsSystem.h>
#include <Engine/Scene/System/SceneManager.h>

#include <Engine/PostProcess/Collection/PostProcessCollection.h>
#include <Engine/PostProcess/Graph/PostEffectGraph.h>

// c++
#include <Windows.h>

class EngineController {
public:
	EngineController();
	~EngineController();

	// 初期化
	void Initialize(HINSTANCE hInstance);

	// 更新
	bool Update();		//< 本更新
	void BeginUpdate(); //< 更新前処理
	void EndUpdate();	//< 更新後処理

	void Render();	 //< 描画
	void Run();		 //< ランタイム更新
	void Finalize(); //< 終了処理

private:
	std::unique_ptr<System>			system_;
	std::unique_ptr<GraphicsSystem> graphicsSystem_;

	// ui
	std::unique_ptr<EngineUICore> engineUICore_;

	// scene
	std::unique_ptr<SceneManager>	  sceneManager_;
	std::unique_ptr<EditorCollection> editorCollection_;

	std::unique_ptr<PlaySession> playSession_;

	// ポストエフェクトの適用と管理
	PostEffectGraph*	   postEffectGraph_;
	PostProcessCollection* postProcessCollection_;
};
