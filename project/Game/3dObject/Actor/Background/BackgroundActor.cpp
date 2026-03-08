#include "BackgroundActor.h"

#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>
#include <Engine/Scene/Context/SceneContext.h>
#include <Game/3d/GameCamera/RailCamera.h>

REGISTER_SCENE_OBJECT(BackgroundActor)

BackgroundActor::BackgroundActor(const std::string& modelName, std::optional<std::string> objectName)
	: BaseGameObject(modelName, objectName) {}

BackgroundActor::BackgroundActor()	= default;
BackgroundActor::~BackgroundActor() = default;

void BackgroundActor::DerivativeGui() {
#if defined(_DEBUG) || defined(DEVELOP)
	ImGui::Checkbox("Stop Rail", &isStopRail_);
	if(isStopRail_) {
		ImGui::DragFloat("Stop Offset", &stopOffset_, 0.1f, 0.0f, 300.0f);
		ImGui::Checkbox("Auto Progress", &autoCalculateProgress_);
		if(autoCalculateProgress_) {
			if(ImGui::Button("Sync Rail Progress")) {
				auto* ctx = SceneContext::Current();
				if(ctx) {
					auto railCam = ctx->FindFirst<RailCamera>();
					if(railCam) {
						// 最近接点からオフセット分を引いて停止位置とする
						float nearestS = railCam->GetSplineData().FindNearestDistance(GetWorldPosition());
						stopProgress_  = (std::max)(0.0f, nearestS - stopOffset_);
					}
				}
			}
			ImGui::Text("Auto Stop Progress: %.2f (Base-Offset)", stopProgress_);
		} else {
			ImGui::DragFloat("Stop Progress", &stopProgress_, 0.1f, 0.0f, 10000.0f);
		}
	}
#endif
}

void BackgroundActor::ApplyDerivedConfigFromJson(const nlohmann::json& root, const nlohmann::json* derived) {
	(void)root;
	if(!derived) return;

	isStopRail_			   = derived->value("isStopRail", false);
	stopProgress_		   = derived->value("stopProgress", 0.0f);
	stopOffset_			   = derived->value("stopOffset", 2.0f);
	autoCalculateProgress_ = derived->value("autoCalculateProgress", true);
}

void BackgroundActor::ExtractDerivedConfigToJson(nlohmann::json& root, nlohmann::json& derived) const {
	(void)root;
	derived["isStopRail"]			 = isStopRail_;
	derived["stopProgress"]			 = stopProgress_;
	derived["stopOffset"]			 = stopOffset_;
	derived["autoCalculateProgress"] = autoCalculateProgress_;
}