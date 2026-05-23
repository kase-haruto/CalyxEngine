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
namespace CalyxEngine {

	/*-----------------------------------------------------------------------------------------
	 * CalyxCore
	 * - エンジンのコアシステム管理クラス
	 * - DirectXの初期化、ウィンドウ管理、パイプライン管理などを担当
	 *---------------------------------------------------------------------------------------*/
	class CalyxCore {
	public:
		//===================================================================*/
		//                    public functions
		//===================================================================*/
		CalyxCore();
		~CalyxCore() = default;

		void Initialize(HINSTANCE hInstance, int32_t clientWidth, int32_t clientHeight, const std::string _windowTitle);
		void Finalize();
		void InitializeEditor();
		void BeginFrame();
		void EndFrame();

		void EditorUpdate(); // engine内部Editorの更新
		int	 ProcessMessage();

		void InitializePostProcess(class PipelineService* service);
		void ExecutePostEffect(const class PipelineService* service);

		//* パイプラインの作成 ==============================*/
		void CreatePipelines();
		void LinePipeline();
		void EffectPipeline();
		void SkyBoxPipeline();

		//===================================================================*/
		//                    getter / setter
		//===================================================================*/
		static HINSTANCE GetHinstance() { return hInstance_; }
		static HWND		 GetHWND() { return hwnd_; }
		CalyxEngine::DxCore*			 GetDxCore() const { return dxCore_.get(); }
		void			 SetEngineUICore(CalyxEngine::EngineUICore* engineUI) { pEngineUICore_ = engineUI; }

	private:
		//===================================================================*/
		//                    private members
		//===================================================================*/
		std::unique_ptr<CalyxEngine::DxCore>dxCore_ = nullptr;

		/*window*/
		std::unique_ptr<WinApp> winApp_;	// ウィンドウ
		static HINSTANCE		hInstance_; // インスタンス
		static HWND				hwnd_;		// ウィンドウハンドル

		// ImGuiの初期化
		std::unique_ptr<ImGuiManager> imguiManager_ = nullptr;

	private:
		// grapics
		std::shared_ptr<ShaderManager>		  shaderManager_;		 // shader管理
		std::unique_ptr<PipelineStateManager> pipelineStateManager_; // パイプライン管理

	private:
		// engineEditors
		CalyxEngine::EngineUICore* pEngineUICore_; // engineUIの描画

		float		radialTimer_		= 0.0f;
		const float kRadialDurationSec_ = 1.0f;
		bool		isRadialActive_		= false;
	};
} // namespace CalyxEngine