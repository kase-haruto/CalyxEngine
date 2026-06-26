#include "Collider.h"

#include "Engine/Objects/3D/Actor/BaseGameObject.h"

#include <Data/Engine/Configs/Scene/Objects/Collider/ColliderConfig.h>
#include <Engine/Collision/CollisionManager.h>

#include <Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h>
#include <Engine/System/Command/EditorCommand/ValueEditCommand.h>
#include <externals/imgui/imgui.h>

#include <array>
#include <cstdint>
#include <functional>
#include <string>

namespace {
	struct ColliderTypeItem {
		ColliderType type;
		const char*	 label;
	};

	constexpr std::array<ColliderTypeItem, 8> kColliderTypeItems = {{
		{ColliderType::Type_Player, "Player"},
		{ColliderType::Type_PlayerAttack, "Player Attack"},
		{ColliderType::Type_Enemy, "Enemy"},
		{ColliderType::Type_EnemySpawner, "Enemy Spawner"},
		{ColliderType::Type_EnemyAttack, "Enemy Attack"},
		{ColliderType::Type_EventObject, "Event Object"},
		{ColliderType::Type_StageGimmick, "Stage Gimmick"},
		{ColliderType::Type_Impediment, "Impediment"},
	}};

	uint32_t ToBits(ColliderType type) {
		return static_cast<uint32_t>(type);
	}

	ColliderType FromBits(uint32_t bits) {
		return static_cast<ColliderType>(bits);
	}

	const char* ToLabel(ColliderType type) {
		if(type == ColliderType::Type_None) return "None";
		for(const auto& item : kColliderTypeItems) {
			if(item.type == type) return item.label;
		}
		return "Custom";
	}

	std::string BuildMaskLabel(ColliderType mask) {
		const uint32_t bits = ToBits(mask);
		if(bits == 0u) return "None";

		std::string label;
		for(const auto& item : kColliderTypeItems) {
			if((bits & ToBits(item.type)) == 0u) continue;
			if(!label.empty()) label += ", ";
			label += item.label;
		}
		return label.empty() ? "Custom" : label;
	}

	void ExecuteColliderTypeEdit(
		const char* commandName,
		ColliderType before,
		ColliderType after,
		const std::function<void(ColliderType)>& setter) {
		if(before == after) return;
		auto intSetter = [setter](const int& value) {
			setter(static_cast<ColliderType>(value));
		};
		CommandManager::GetInstance()->Execute(
			std::make_unique<ValueEditCommand<int>>(commandName, static_cast<int>(before), static_cast<int>(after), intSetter));
	}

	void DrawColliderObjectTypeEditor(Collider* collider) {
		ColliderType current = collider->GetType();
		const char*	 preview = ToLabel(current);

		if(ImGui::BeginCombo("Object Type", preview)) {
			auto setter = [collider](ColliderType type) {
				if(collider) collider->SetType(type);
			};

			const bool noneSelected = current == ColliderType::Type_None;
			if(ImGui::Selectable("None", noneSelected)) {
				ExecuteColliderTypeEdit("Change Collider Object Type", current, ColliderType::Type_None, setter);
			}
			if(noneSelected) ImGui::SetItemDefaultFocus();

			for(const auto& item : kColliderTypeItems) {
				const bool selected = current == item.type;
				if(ImGui::Selectable(item.label, selected)) {
					ExecuteColliderTypeEdit("Change Collider Object Type", current, item.type, setter);
				}
				if(selected) ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}
	}

	void DrawColliderTargetMaskEditor(Collider* collider) {
		const ColliderType current = collider->GetTargetType();
		const std::string	 preview = BuildMaskLabel(current);

		if(!ImGui::BeginCombo("Collision Targets", preview.c_str())) {
			return;
		}

		auto setter = [collider](ColliderType targetType) {
			if(collider) collider->SetTargetType(targetType);
		};

		if(ImGui::SmallButton("All")) {
			uint32_t allBits = 0u;
			for(const auto& item : kColliderTypeItems) {
				allBits |= ToBits(item.type);
			}
			ExecuteColliderTypeEdit("Change Collision Targets", current, FromBits(allBits), setter);
		}
		ImGui::SameLine();
		if(ImGui::SmallButton("None")) {
			ExecuteColliderTypeEdit("Change Collision Targets", current, ColliderType::Type_None, setter);
		}
		ImGui::Separator();

		const uint32_t currentBits = ToBits(collider->GetTargetType());
		for(const auto& item : kColliderTypeItems) {
			bool enabled = (currentBits & ToBits(item.type)) != 0u;
			if(ImGui::Checkbox(item.label, &enabled)) {
				uint32_t afterBits = currentBits;
				if(enabled) {
					afterBits |= ToBits(item.type);
				} else {
					afterBits &= ~ToBits(item.type);
				}
				ExecuteColliderTypeEdit("Change Collision Targets", current, FromBits(afterBits), setter);
			}
		}

		ImGui::EndCombo();
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
//		ctor / dtor
/////////////////////////////////////////////////////////////////////////////////////////
Collider::Collider(bool isEnuble) {
	isCollisionEnabled_ = isEnuble;
	if(isCollisionEnabled_) {
		CollisionManager::GetInstance()->Register(this);
	}
}

Collider::~Collider() {
	CollisionManager::GetInstance()->Unregister(this);
}

/////////////////////////////////////////////////////////////////////////////////////////
//		デバッグ用ui
/////////////////////////////////////////////////////////////////////////////////////////
void Collider::ShowGui() {
	bool enabled = isCollisionEnabled_;
	if(GuiCmd::CheckBox("Enable Collision", enabled)) {
		SetCollisionEnabled(enabled);
	}

	if(!isCollisionEnabled_) return;

	DrawColliderObjectTypeEditor(this);
	DrawColliderTargetMaskEditor(this);

	GuiCmd::CheckBox("Draw Collider", isDraw_);
	GuiCmd::CheckBox("Is Trigger", isTrigger_);
	GuiCmd::ColorEdit4("Collider Color", color_);
	GuiCmd::DragFloat3("Offset", offset_);
	GuiCmd::DragFloat3("Rotate Offset", rotateOffset_);
}

void Collider::ShowGui(ColliderConfig& config) {

	if(ImGui::CollapsingHeader("Collider")) {

		GuiCmd::DragFloat3("offset", config.offset);
		GuiCmd::DragFloat3("rotate", config.rotate);

		bool enabled = config.isCollisionEnabled;
		if(GuiCmd::CheckBox("Enable Collision", enabled)) {
			SetCollisionEnabled(enabled);
		}

		if(!config.isCollisionEnabled) return;

		GuiCmd::CheckBox("Draw Collider", config.isDraw);
		GuiCmd::ColorEdit4("Collider Color", color_);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
//		衝突通知(衝突フレーム
/////////////////////////////////////////////////////////////////////////////////////////
void Collider::NotifyCollisionEnter(Collider* other) {
	if(onEnter_) onEnter_(other);
}

/////////////////////////////////////////////////////////////////////////////////////////
//		衝突通知(衝突中
/////////////////////////////////////////////////////////////////////////////////////////
void Collider::NotifyCollisionStay(Collider* other) {
	if(onStay_) onStay_(other);
}

/////////////////////////////////////////////////////////////////////////////////////////
//		衝突通知(離れた
/////////////////////////////////////////////////////////////////////////////////////////
void Collider::NotifyCollisionExit(Collider* other) {
	if(onExit_) onExit_(other);
}

/////////////////////////////////////////////////////////////////////////////////////////
//		config適用
/////////////////////////////////////////////////////////////////////////////////////////
void Collider::ApplyConfig(const ColliderConfig& config) {
	isDraw_				= config.isDraw;
	isTrigger_			= config.isTrigger;
	type_				= static_cast<ColliderType>(config.colliderType);
	targetType_			= static_cast<ColliderType>(config.targetType);
	offset_				= config.offset;
	rotateOffset_		= config.rotate;
	SetCollisionEnabled(config.isCollisionEnabled);
}

/////////////////////////////////////////////////////////////////////////////////////////
//		config吐き出し
/////////////////////////////////////////////////////////////////////////////////////////
ColliderConfig Collider::ExtractConfig() const {
	ColliderConfig config;
	config.isCollisionEnabled = isCollisionEnabled_;
	config.isDraw			  = isDraw_;
	config.isTrigger		  = isTrigger_;
	config.colliderType		  = static_cast<int>(type_);
	config.targetType		  = static_cast<int>(targetType_);
	config.offset			  = offset_;
	config.rotate			  = rotateOffset_;
	return config;
}

CalyxEngine::Vector3 Collider::GetWorldPos() const {
	if(owner_) {
		return owner_->GetWorldPosition();
	}
	// オーナーがいない場合はゼロベクトルを返す
	return CalyxEngine::Vector3::Zero();
}
/////////////////////////////////////////////////////////////////////////////////////////
//		Collisionするか
/////////////////////////////////////////////////////////////////////////////////////////
void Collider::SetCollisionEnabled(bool enable) {
	if(isCollisionEnabled_ == enable) return; // 状態が変わらないなら何もしない

	isCollisionEnabled_ = enable;

	if(enable) {
		CollisionManager::GetInstance()->Register(this);
	} else {
		CollisionManager::GetInstance()->Unregister(this);
	}
}
