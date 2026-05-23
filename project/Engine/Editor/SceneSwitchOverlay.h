#pragma once
#include <Engine/Scene/System/SceneManager.h>

namespace CalyxEngine {

	/*-----------------------------------------------------------------------------------------
	 * SceneSwitchOverlay
	 * - シーン切り替えオーバーレイ
	 * - 登録されたシーンをツールバー形式で表示し、即座に切り替えを可能にする
	 *---------------------------------------------------------------------------------------*/
	class SceneSwitchOverlay {
	public:
		void SetSceneManager(CalyxEngine::SceneManager* manager) { sceneManager_ = manager; }
		void RenderToolbar();

	private:
		CalyxEngine::SceneManager* sceneManager_ = nullptr;
	};

} // namespace CalyxEngine