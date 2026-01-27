#pragma once
#include <Engine/Scene/System/SceneManager.h>

namespace CalyxEditor {

	/*-----------------------------------------------------------------------------------------
	 * SceneSwitchOverlay
	 * - シーン切り替えオーバーレイ
	 * - 登録されたシーンをツールバー形式で表示し、即座に切り替えを可能にする
	 *---------------------------------------------------------------------------------------*/
	class SceneSwitchOverlay {
	public:
		void SetSceneManager(CalyxScene::SceneManager* manager) { sceneManager_ = manager; }
		void RenderToolbar();

	private:
		CalyxScene::SceneManager* sceneManager_ = nullptr;
	};

} // namespace CalyxEditor