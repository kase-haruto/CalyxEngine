#pragma
/* ========================================================================
/* include space
/* ===================================================================== */
// engine
#include <Engine/Application/System/System.h>
#include <Engine/Application/Platform/WinApp.h>
#include <Engine/Scene/System/SceneManager.h>
#include <Engine/Application/UI/EngineUI/Core/EngineUICore.h>
#include <Engine/Graphics/Core/GraphicsSystem.h>
#include <Engine/Editor/Collection/EditorCollection.h>

#include <Engine/PostProcess/Collection/PostProcessCollection.h>
#include <Engine/PostProcess/Graph/PostEffectGraph.h>

// c++
#include <Windows.h>

class EngineController {
public:
	EngineController() = default;
	~EngineController() = default;

	void Initialize(HINSTANCE hInstance);
	bool Update();
	void BeginUpdate();
	void EndUpdate();
	void Render();
	void Run();
	void Finalize();

private:
	std::unique_ptr<System> system_;
	std::unique_ptr<GraphicsSystem> graphicsSystem_;

	//ui
	std::unique_ptr<EngineUICore> engineUICore_;

	//scene
	std::unique_ptr<SceneManager> sceneManager_;
	std::unique_ptr<EditorCollection> editorCollection_;

	std::vector<PostEffectSlot> postEffectSlots_;

	// ポストエフェクトの適用と管理
	PostEffectGraph* postEffectGraph_;
	PostProcessCollection* postProcessCollection_;

	// ブラー演出の有効フラグと時間
	bool isRadialActive_ = false;
	float radialTimer_ = 0.0f;

	// ブラーの継続時間（秒単位）
	// 例: 1.0f秒など、お好みで設定
	const float kRadialDurationSec_ = 1.0f;

};
