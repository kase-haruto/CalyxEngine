#pragma once

/* ========================================================================
/*	include space
/* ===================================================================== */
// engine
#include <Engine/Application/UI/GUI/ImGuiManager.h>
#include <Engine/Graphics/Device/DxCore.h>
#include <Engine/Graphics/Pipeline/Manager/PipelineStateManager.h>
#include <Engine/Application/UI/EngineUI/Core/EngineUICore.h>

// postprocess
#include <Engine/PostProcess/Collection/PostProcessCollection.h>

//リークチェック
#include <Engine/Foundation/Utility/LeakChecker/LeakChecker.h>

/* c++ */
#include<stdint.h>

class System{
public:
	//===================================================================*/
	//                    public functions
	//===================================================================*/
	System();
	~System() = default;

	/// <summary>
	/// 初期化処理
	/// </summary>
	void Initialize(HINSTANCE hInstance, int32_t clientWidth, int32_t clientHeight,const std::string _windowTitle);
	
	/// <summary>
	/// エディタ初期化
	/// </summary>
	void InitializeEditor();

	/// <summary>
	/// 終了処理
	/// </summary>
	void Finalize();

	/// <summary>
	/// 更新前処理
	/// </summary>
	void BeginFrame();

	/// <summary>
	/// 更新後処理
	/// </summary>
	void EndFrame();

	/// <summary>
	/// エディタ更新
	/// </summary>
	void EditorUpdate();

	/// <summary>
	/// message
	/// </summary>
	/// <returns></returns>
	int ProcessMessage();

	/// <summary>
	/// ポストプロセス関連の初期化
	/// </summary>
	/// <param name="service"></param>
	void InitializePostProcess(class PipelineService* service);

	/// <summary>
	/// ポストエフェクトの実行
	/// </summary>
	/// <param name="service"></param>
	void ExecutePostEffect(const class  PipelineService* service);

	//* パイプラインの作成 ==============================*/
	void CreatePipelines();
	void LinePipeline();
	void EffectPipeline();
	void SkyBoxPipeline();

	//===================================================================*/
	//                    getter / setter
	//===================================================================*/
	static HINSTANCE GetHinstance(){ return hInstance_; }
	static HWND GetHWND(){ return hwnd_; }
	DxCore* GetDxCore()const{ return dxCore_.get(); }
	void SetEngineUICore(EngineUICore* engineUI) { pEngineUICore_ = engineUI; }

private:
	//===================================================================*/
	//                    private members
	//===================================================================*/
	std::unique_ptr<DxCore> dxCore_ = nullptr;

	/*window*/
	std::unique_ptr<WinApp> winApp_;	//ウィンドウ
	static HINSTANCE hInstance_;		//インスタンス
	static HWND hwnd_;					//ウィンドウハンドル

	// ImGuiの初期化
	std::unique_ptr<ImGuiManager> imguiManager_ = nullptr;

private:
	// grapics
	std::shared_ptr<ShaderManager>shaderManager_;					//shader管理
	std::unique_ptr<PipelineStateManager>pipelineStateManager_;		//パイプライン管理

private:
	// engineEditors
	EngineUICore* pEngineUICore_;			//engineUIの描画
	
	float radialTimer_ = 0.0f;
	const float kRadialDurationSec_ = 1.0f;
	bool isRadialActive_ = false;
};