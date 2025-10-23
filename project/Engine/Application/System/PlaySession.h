#pragma once

// engine
#include <Engine/Application/System/EngineMode.h>
#include <Engine/Scene/Context/SceneContext.h>
// externals
#include <externals/imgui/imgui.h>
// c++
#include <memory>

/// <summary>
/// ランタイムの再生・停止・リセットなど
/// </summary>
class PlaySession {
public:
	//===================================================================*/
	//				public methods
	//===================================================================*/
	/// <summary>
	/// 初期化
	/// </summary>
	/// <param name="Context"></param>
	void Initialize(SceneContext* editorContext);

	/// <summary>
	/// 再生
	/// </summary>
	void Enter();

	/// <summary>
	/// 再再生
	/// </summary>
	void Restart();

	/// <summary>
	/// 終了
	/// </summary>
	void Exit();

	/// <summary>
	/// 終了Request
	/// </summary>
	/// <returns></returns>
	bool ExitRequested() const;

	/// <summary>
	/// 終了処理
	/// </summary>
	void FinalizeExitCleanup();

	/// <summary>
	/// 停止のトグル
	/// </summary>
	void TogglePause();

	/// <summary>
	/// 更新
	/// </summary>
	/// <param name="newEditorCtx"></param>
	void Update();

	/// <summary>
	/// エディタからランタイムの再実行
	/// </summary>
	/// <returns></returns>
	void RebuildRuntimeFromEditor(SceneContext* newEditorCtx);

	/// <summary>
	/// ツールバー描画
	/// </summary>
	void RenderToolbar();

	// SceneManager からの接続API
	void BindEditorContext(SceneContext* ctx);
	bool IsRuntime() const;

	// accessor
	uint64_t RuntimeGeneration() const { return runtimeGen_; }
	SceneContext* GetContext() const;

private:
	//===================================================================*/
	//				provate methods
	//===================================================================*/
	/// <summary>
	/// 一フレーム更新
	/// </summary>
	void StepOnce();

private:
	//===================================================================*/
	//				public methods
	//===================================================================*/
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