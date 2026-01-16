#pragma once
/* ========================================================================
/*		include space
/* ===================================================================== */
// engine
#include <Engine/Application/System/EngineMode.h>
#include <Engine/Scene/Context/SceneContext.h>
// externals
#include <externals/imgui/imgui.h>
// c++
#include <memory>

namespace CalyxEditor {

	/*-----------------------------------------------------------------------------------------
	 * PlaySession
	 * - プレイセッション管理クラス
	 * - ランタイムの実行・停止・再実行・ワンステップ実行を制御
	 *---------------------------------------------------------------------------------------*/
	class PlaySession {
	public:
		void Initialize(SceneContext* editorContext);
		void Update();
		void RenderToolbar();

		//--------- session -----------------------------------------------------
		void Enter();		//< 実行
		void Restart();		//< リスタート
		void Exit();		//< 終了
		void TogglePause(); //< 停止とグル
		void StepOnce();	//< ワンステップ更新

		// 終了Request
		bool ExitRequested() const;
		// 終了クリーンナップ
		void FinalizeExitCleanup();
		// 再接続
		void RebuildRuntimeFromEditor(SceneContext* newEditorCtx);
		// SceneManager からの接続API
		void BindEditorContext(SceneContext* ctx);

		//--------- accessor -----------------------------------------------------
		bool		  IsRuntime() const;
		SceneContext* GetContext() const;
		uint64_t	  RuntimeGeneration() const { return runtimeGen_; }

	private:
		SceneContext*				  editorContext_ = nullptr;
		std::unique_ptr<SceneContext> runtimeContext_;
		EngineMode					  mode_			 = EngineMode::Editor;
		bool						  exitRequested_ = false;
		uint64_t					  runtimeGen_	 = 0;

		struct IconData {
			ImTextureID tex	 = nullptr;
			ImVec2		size = ImVec2(30, 30);
		};
		IconData iconPlay_;
		IconData iconPause_;
		IconData iconStep_;
		IconData iconStop_;
		IconData iconRestart_;
		void	 LoadIcons();
	};
}

