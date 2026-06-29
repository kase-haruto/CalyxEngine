#pragma once

class CollisionLayerSettings;

namespace CollisionLayerTableUI {
	// Scene Settings等の既存ImGuiウィンドウ内から呼び出す独立UI。
	// ウィンドウ名や表示状態は呼び出し側のEditor構造に任せるため、
	// この関数自身はImGui::Begin / Endを行わない。
	// 引数なし版は現在SceneContextが有効化したCollisionLayerSettingsを編集する。
	void Draw();

	// Scene Settingsウィンドウ等、対象シーンの設定を明示できる場合はこちらを使用する。
	void Draw(CollisionLayerSettings& settings);
}
