#include "Collider.h"

#include <Data/Engine/Configs/Scene/Objects/Collider/ColliderConfig.h>
#include <Engine/Collision/CollisionLayerSettings.h>
#include <Engine/Collision/CollisionManager.h>
#include <Engine/Objects/3D/Actor/BaseGameObject.h>
#include <Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h>
#include <Engine/System/Command/EditorCommand/ValueEditCommand.h>
#include <externals/imgui/imgui.h>

#include <cstdint>
#include <functional>
#include <memory>

namespace {
	void ExecuteColliderLayerEdit(Collider* collider, CollisionLayerId before, CollisionLayerId after) {
		if(!collider || before == after) {
			return;
		}

		// Layer変更も他のInspector編集と同じCommandへ載せ、Undo/Redoで同じIDを復元できるようにする。
		// ValueEditCommandがintを扱うため境界で変換し、Collider内部ではCollisionLayerIdを維持する。
		auto setter = [collider](const int& value) {
			if(collider) {
				collider->SetLayerId(static_cast<CollisionLayerId>(value));
			}
		};
		CommandManager::GetInstance()->Execute(
			std::make_unique<ValueEditCommand<int>>(
				"Change Collision Layer", static_cast<int>(before), static_cast<int>(after), setter));
	}

	void DrawColliderLayerEditor(Collider* collider) {
		if(!collider) {
			return;
		}

		// 固定配列を使わずSettingsの現在の一覧を参照するため、追加・削除・リネームが即座に反映される。
		auto* settings = CollisionLayerSettings::GetInstance();
		const CollisionLayerId current = collider->GetLayerId();
		const std::string& preview = settings->GetLayerName(current);
		if(ImGui::BeginCombo("Collision Layer", preview.c_str())) {
			for(const auto& layer : settings->GetLayers()) {
				const bool selected = layer.id == current;
				if(ImGui::Selectable(layer.name.c_str(), selected)) {
					ExecuteColliderLayerEdit(collider, current, layer.id);
				}
				if(selected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}
	}

	void DrawConfigLayerEditor(ColliderConfig& config) {
		auto* settings = CollisionLayerSettings::GetInstance();
		// JSON上は将来の型拡張を考慮してuint32_tだが、実行時IDは0～31だけを許可する。
		const CollisionLayerId current = config.layerId < kMaxCollisionLayerCount
			? static_cast<CollisionLayerId>(config.layerId)
			: kDefaultCollisionLayerId;
		const std::string& preview = settings->GetLayerName(current);
		if(ImGui::BeginCombo("Collision Layer", preview.c_str())) {
			for(const auto& layer : settings->GetLayers()) {
				const bool selected = layer.id == current;
				if(ImGui::Selectable(layer.name.c_str(), selected)) {
					config.layerId = layer.id;
				}
				if(selected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}
	}
}

Collider::Collider(bool isEnuble) {
	// 従来のライフサイクルを維持し、有効状態で生成されたColliderだけをManagerへ登録する。
	isCollisionEnabled_ = isEnuble;
	if(isCollisionEnabled_) {
		CollisionManager::GetInstance()->Register(this);
	}
}

Collider::~Collider() {
	// 無効Colliderや登録済みでないColliderでもUnregisterは安全に無視される。
	// 破棄後ポインタがCollisionPairへ残らないよう、必ず登録解除を要求する。
	CollisionManager::GetInstance()->Unregister(this);
}

void Collider::ShowGui() {
	bool enabled = isCollisionEnabled_;
	if(GuiCmd::CheckBox("Enable Collision", enabled)) {
		SetCollisionEnabled(enabled);
	}

	if(!isCollisionEnabled_) {
		return;
	}

	DrawColliderLayerEditor(this);
	GuiCmd::CheckBox("Draw Collider", isDraw_);
	GuiCmd::CheckBox("Is Trigger", isTrigger_);
	GuiCmd::ColorEdit4("Collider Color", color_);
	GuiCmd::DragFloat3("Offset", offset_);
	GuiCmd::DragFloat3("Rotate Offset", rotateOffset_);
}

void Collider::ShowGui(ColliderConfig& config) {
	if(!ImGui::CollapsingHeader("Collider")) {
		return;
	}

	GuiCmd::DragFloat3("offset", config.offset);
	GuiCmd::DragFloat3("rotate", config.rotate);

	bool enabled = config.isCollisionEnabled;
	if(GuiCmd::CheckBox("Enable Collision", enabled)) {
		config.isCollisionEnabled = enabled;
		SetCollisionEnabled(enabled);
	}

	if(!config.isCollisionEnabled) {
		return;
	}

	DrawConfigLayerEditor(config);
	GuiCmd::CheckBox("Draw Collider", config.isDraw);
	GuiCmd::CheckBox("Is Trigger", config.isTrigger);
	GuiCmd::ColorEdit4("Collider Color", color_);
}

void Collider::NotifyCollisionEnter(Collider* other) {
	if(onEnter_) {
		onEnter_(other);
	}
}

void Collider::NotifyCollisionStay(Collider* other) {
	if(onStay_) {
		onStay_(other);
	}
}

void Collider::NotifyCollisionExit(Collider* other) {
	if(onExit_) {
		onExit_(other);
	}
}

void Collider::ApplyConfig(const ColliderConfig& config) {
	// Layer名は保存せずIDだけを復元する。名前変更はSettings側だけで完結する。
	isDraw_ = config.isDraw;
	isTrigger_ = config.isTrigger;
	SetLayerId(config.layerId < kMaxCollisionLayerCount
		? static_cast<CollisionLayerId>(config.layerId)
		: kDefaultCollisionLayerId);
	offset_ = config.offset;
	rotateOffset_ = config.rotate;
	SetCollisionEnabled(config.isCollisionEnabled);
}

ColliderConfig Collider::ExtractConfig() const {
	// ゲーム固有分類や相手Maskは書き出さず、Matrix参照に必要なLayer IDだけを永続化する。
	ColliderConfig config;
	config.isCollisionEnabled = isCollisionEnabled_;
	config.isDraw = isDraw_;
	config.isTrigger = isTrigger_;
	config.layerId = layerId_;
	config.offset = offset_;
	config.rotate = rotateOffset_;
	return config;
}

CalyxEngine::Vector3 Collider::GetWorldPos() const {
	if(owner_) {
		return owner_->GetWorldPosition();
	}
	return CalyxEngine::Vector3::Zero();
}

void Collider::SetCollisionEnabled(bool enable) {
	// 同じ値の再設定ではRegister/Unregisterを発行せず、Managerの遅延キューを増やさない。
	if(isCollisionEnabled_ == enable) {
		return;
	}

	isCollisionEnabled_ = enable;
	if(enable) {
		CollisionManager::GetInstance()->Register(this);
	} else {
		CollisionManager::GetInstance()->Unregister(this);
	}
}
