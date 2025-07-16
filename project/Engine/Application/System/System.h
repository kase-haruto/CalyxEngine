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
#include <Engine/PostProcess/Graph/PostEffectGraph.h>
#include <Engine/PostProcess/Slot/PostEffectSlot.h>

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

	void Initialize(HINSTANCE hInstance, int32_t clientWidth, int32_t clientHeight,const std::string _windowTitle);
	void Finalize();
	void InitializeEditor();
	void BeginFrame();
	void EndFrame(const class PipelineService* pipelineSet);

	void EditorUpdate();	//engine内部Editorの更新

	int ProcessMessage();

	void InitializePostProcess(class PipelineService* service);


	//* パイプラインの作成 ==============================*/
	void CreatePipelines();
	void Object2DPipelines();
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
	PostProcessCollection* GetPostProcessCollection() const{ return postProcessCollection_.get(); }
	PostEffectGraph* GetPostEffectGraph() const{ return postEffectGraph_.get(); }

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
	
	// postprocess
	std::unique_ptr<PostProcessCollection> postProcessCollection_;
	std::unique_ptr<PostEffectGraph> postEffectGraph_;

	float radialTimer_ = 0.0f;
	const float kRadialDurationSec_ = 1.0f;
	bool isRadialActive_ = false;
};

