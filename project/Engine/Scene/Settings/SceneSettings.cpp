#include "SceneSettings.h"

nlohmann::json SceneSettings::ToJson() const {
	// 各カテゴリを独立したキーへ分け、将来カテゴリが増えても既存JSONとの互換性を維持する。
	return nlohmann::json{
		{"version", 2},
		{"collision", collisionSettings_.ToJson()},
		{"sortingLayers", sortingLayerSettings_.ToJson()},
	};
}

void SceneSettings::ApplyJson(const nlohmann::json& json) {
	ResetToDefault();
	if(!json.is_object()) {
		return;
	}

	// 未知カテゴリは無視するため、新しいEditorで保存したシーンも古い実装で安全に開ける。
	if(const auto collisionIt = json.find("collision"); collisionIt != json.end()) {
		collisionSettings_.ApplyJson(*collisionIt);
	}
	if(const auto sortingIt = json.find("sortingLayers"); sortingIt != json.end()) {
		sortingLayerSettings_.ApplyJson(*sortingIt);
	}
}

void SceneSettings::ResetToDefault() {
	// Sceneの新規作成やsettingsを持たない旧シーン読込では、各カテゴリを既定値へ戻す。
	collisionSettings_.ResetToDefault();
	sortingLayerSettings_.ResetToDefault();
}
