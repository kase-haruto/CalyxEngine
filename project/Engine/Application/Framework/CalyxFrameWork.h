#pragma once
/* ========================================================================
/* include space
/* ===================================================================== */
// engine
#include <Engine/Application/Platform/WinApp.h>
#include <Engine/Application/System/CalyxCore.h>
#include <Engine/Application/System/PlaySession.h>
#include <Engine/Application/UI/EngineUI/Core/EngineUICore.h>
#include <Engine/Editor/Collection/EditorCollection.h>
#include <Engine/Graphics/Core/GraphicsSystem.h>
#include <Engine/Scene/System/SceneManager.h>

#include <Engine/PostProcess/Collection/PostProcessCollection.h>
#include <Engine/PostProcess/Graph/PostEffectGraph.h>

// engine
#include <Engine/Foundation/HotReload/LivePP/LivePPService.h>

// c++
#include <Windows.h>
namespace CalyxEngine {

	/*-----------------------------------------------------------------------------------------
	 * CalyxFrameWork
	 * - エンジンのメインフレームワーククラス
	 * - アプリケーションの初期化、更新、描画、終了処理を統括
	 *---------------------------------------------------------------------------------------*/
	class CalyxFrameWork {
	public:
		CalyxFrameWork()  = default;
		~CalyxFrameWork() = default;

		void Initialize(HINSTANCE hInstance);
		void BeginUpdate();
		bool Update();
		void EndUpdate();
		void Run();
		void Render();
		void Finalize();

	private:
		// system
		std::unique_ptr<CalyxCore>		system_;
		std::unique_ptr<GraphicsSystem> graphicsSystem_;

		// ui
		std::unique_ptr<CalyxEngine::EngineUICore> engineUICore_;

		// scene
		std::unique_ptr<CalyxEngine::SceneManager>	   sceneManager_;
		std::unique_ptr<CalyxEngine::EditorCollection> editorCollection_;
		std::unique_ptr<CalyxEngine::PlaySession>	   playSession_;

		// ホットリロード
		LivePPService livePPService_;

		// ポストエフェクトの適用と管理
		PostEffectGraph*	   postEffectGraph_;
		PostProcessCollection* postProcessCollection_;
	};
} // namespace CalyxEngine