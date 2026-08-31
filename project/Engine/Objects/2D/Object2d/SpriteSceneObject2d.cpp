#include "SpriteSceneObject2d.h"

#include <Engine\Application\UI\Panels\AssetPanel.h>
#include <Engine\Application\UI\Panels\InspectorPanel.h>
#include <Engine\Assets\Database\AssetDatabase.h>
#include <Engine\Assets\DataAsset\DataAssetManager.h>
#include <Engine\Assets\Manager\AssetManager.h>
#include <Engine\Assets\System\AssetRecord.h>
#include <Engine\Objects\3D\Actor\Registry\SceneObjectRegistry.h>
#include <Engine\Renderer\Sprite\SpriteRenderer.h>
#include <Engine\Scene\Context\SceneContext.h>
#include <Engine\System\Command\EditorCommand\GuiCommand\ImGuiHelper\GuiCmd.h>

#include <externals\imgui\imgui.h>

#include <filesystem>
#include <algorithm>
#include <array>
#include <cmath>

namespace CalyxEngine {
	namespace {
		std::string RelativeAssetPath(const std::filesystem::path& path) {
			auto* db = AssetDatabase::GetInstance();
			if(!db) return path.generic_string();
			std::error_code ec;
			auto rel = std::filesystem::relative(path, db->GetRoot(), ec);
			return ec ? path.generic_string() : rel.generic_string();
		}

		std::string AssetLabel(const Guid& guid, AssetType type) {
			if(!guid.isValid()) return "(none)";
			if(auto* db = AssetDatabase::GetInstance()) {
				if(const AssetRecord* record = db->Get(guid); record && record->type == type) {
					return record->sourcePath.filename().string();
				}
			}
			return guid.ToString();
		}
	}

	SpriteSceneObject2d::SpriteSceneObject2d() {
		SetName("Sprite2D", ObjectType::Object2D);
		SetCastShadow(false);
		worldTransform_.translation = {640.0f, 360.0f, 0.0f};
		worldTransform_.scale = {128.0f, 128.0f, 1.0f};
	}

	void SpriteSceneObject2d::Initialize() {
		EnsureSprite();
	}

	void SpriteSceneObject2d::AlwaysUpdate(float dt) {
		// EditorとRuntimeの両方でTransform Animationを反映し、描画用Spriteへ最新状態を転送する。
		EnsureSprite();
		transformAnimation2d_.Update(worldTransform_, dt);
		worldTransform_.Update();
		SyncSpriteFromTransform();
		sprite_->SetColor(color_);
		sprite_->Update(dt);
	}

	void SpriteSceneObject2d::ShowGui() {
		DrawBaseGui();
	}

	void SpriteSceneObject2d::DrawSprite(SpriteRenderer* renderer) const {
		if(renderer) SubmitSprites(*renderer);
	}

	void SpriteSceneObject2d::SubmitSprites(SpriteRenderer& renderer) const {
		if(sprite_ && IsDrawEnable()) {
			// 同一Layer・Orderでも描画順がフレームごとに揺れないようGUID由来の安定キーを渡す。
			const uint64_t stableOrder = static_cast<uint64_t>(std::hash<Guid>{}(GetGuid()));
			renderer.Register(sprite_->GetSprite(), sortingLayerId_, orderInLayer_, stableOrder);
		}
	}

	void SpriteSceneObject2d::SetSortingLayer(SortingLayerId layerId) {
		// Sceneに存在しないLayer IDは既定Layerへ退避し、古い設定の読込後も描画対象を失わない。
		sortingLayerId_ = SortingLayerSettings::GetInstance()->IsValidLayerId(layerId)
			? layerId
			: kDefaultSortingLayerId;
	}

	bool SpriteSceneObject2d::SetSortingLayer(std::string_view layerName) {
		const auto layerId = SortingLayerSettings::GetInstance()->FindLayerId(layerName);
		if(!layerId) return false;
		SetSortingLayer(*layerId);
		return true;
	}

	void SpriteSceneObject2d::SetAnchor(const Vector2& anchor) {
		anchor_ = {
			std::clamp(anchor.x, 0.0f, 1.0f),
			std::clamp(anchor.y, 0.0f, 1.0f)};
		if(sprite_) {
			sprite_->SetAnchorPoint(anchor_);
		}
	}

	void SpriteSceneObject2d::SetFillAmount(float amt) {
		sprite_->SetFillAmount(amt);
	}

	void SpriteSceneObject2d::SetFillOrigin(float x, float y) {
		sprite_->SetFillOrigin(x, y);
	}



	void SpriteSceneObject2d::ApplyConfigFromJson(const nlohmann::json& j) {
		// JSON Keyは既存Sceneとの互換性を維持し、不足項目には現在値を利用する。
		id_ = j.value("guid", id_);
		name_ = j.value("name", name_);
		parentId_ = j.value("parentGuid", parentId_);
		textureGuid_ = j.value("textureGuid", textureGuid_);
		texturePath_ = j.value("texturePath", texturePath_);
		anchor_ = j.value("anchor", anchor_);
		color_ = j.value("color", color_);
		SetSortingLayer(j.value("sortingLayerId", kDefaultSortingLayerId));
		orderInLayer_ = j.value("orderInLayer", 0);
		// AnimationとTransformをSprite生成前に復元し、初期GPU表示へ正しい値を反映する。
		if(j.contains("transformAnimation2d")) {
			transformAnimation2d_.ApplyConfigFromJson(j.at("transformAnimation2d"));
		}
		if(j.contains("transform")) {
			worldTransform_.ApplyConfig(j.at("transform").get<WorldTransformConfig>());
		}
		// Asset GUIDを優先してTextureを解決し、旧SceneのPathはFallbackとして利用する。
		EnsureSprite();
		sprite_->SetTexture(ResolveTexturePath());
	}

	void SpriteSceneObject2d::ExtractConfigToJson(nlohmann::json& j) const {
		// Runtime用Sprite内部値ではなく、SceneObject側の正規化済み設定を保存する。
		j["name"] = name_;
		j["guid"] = id_;
		j["parentGuid"] = parentId_;
		j["transform"] = const_cast<WorldTransform&>(worldTransform_).ExtractConfig();
		j["textureGuid"] = textureGuid_;
		j["texturePath"] = texturePath_;
		j["anchor"] = anchor_;
		j["color"] = color_;
		j["sortingLayerId"] = sortingLayerId_;
		j["orderInLayer"] = orderInLayer_;
		// 空Animationは出力せず、既存Sceneファイルの不要な肥大化を避ける。
		if(!transformAnimation2d_.IsEmpty()) {
			nlohmann::json animationJson;
			transformAnimation2d_.ExtractConfigToJson(animationJson);
			j["transformAnimation2d"] = animationJson;
		}
	}

	void SpriteSceneObject2d::SetFillMethod(int method) {
		sprite_->SetFillMethod(method);
	}

	void SpriteSceneObject2d::EnsureSprite() {
		if(sprite_) return;
		// SceneObjectがSpriteの所有権を持ち、初回利用時にだけ描画Resourceを初期化する。
		sprite_ = std::make_unique<SpriteObject2d>();
		sprite_->Initialize(ResolveTexturePath());
		SyncSpriteFromTransform();
		sprite_->SetColor(color_);
	}

	void SpriteSceneObject2d::SyncSpriteFromTransform() {
		if(!sprite_) return;
		// WorldTransformの2D成分を描画Objectへ変換し、Scene編集値とRenderer入力を同期する。
		sprite_->SetPosition({worldTransform_.translation.x, worldTransform_.translation.y});
		sprite_->SetScale({worldTransform_.scale.x, worldTransform_.scale.y});
		sprite_->SetRotation(worldTransform_.eulerRotation.z);
		sprite_->SetAnchorPoint(anchor_);
		sprite_->SetVisibility(IsDrawEnable());
	}

	void SpriteSceneObject2d::DrawBaseGui() {
		if(GuiCmd::BeginSection(ParamFilterSection::Object)) {
			worldTransform_.ShowImGui("2D Transform");
			if(ImGui::TreeNodeEx("Rendering", ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DefaultOpen)) {
				auto* settings = SortingLayerSettings::GetInstance();
				const SortingLayer* current = settings->FindLayer(sortingLayerId_);
				const char* preview = current ? current->name.c_str() : "Default";
				if(ImGui::BeginCombo("Sorting Layer", preview)) {
					for(const auto& layer : settings->GetLayers()) {
						const bool selected = layer.id == sortingLayerId_;
						if(ImGui::Selectable(layer.name.c_str(), selected)) SetSortingLayer(layer.id);
						if(selected) ImGui::SetItemDefaultFocus();
					}
					ImGui::EndCombo();
				}
				ImGui::InputInt("Order in Layer", &orderInLayer_);
				ImGui::TreePop();
			}
			if(ImGui::TreeNodeEx("2D Layout", ImGuiTreeNodeFlags_SpanAvailWidth)) {
				struct AnchorPreset {
					const char* name;
					Vector2 value;
				};
				const AnchorPreset presets[] = {
					{"Top Left", {0.0f, 0.0f}},
					{"Top", {0.5f, 0.0f}},
					{"Top Right", {1.0f, 0.0f}},
					{"Left", {0.0f, 0.5f}},
					{"Center", {0.5f, 0.5f}},
					{"Right", {1.0f, 0.5f}},
					{"Bottom Left", {0.0f, 1.0f}},
					{"Bottom", {0.5f, 1.0f}},
					{"Bottom Right", {1.0f, 1.0f}},
				};

				const char* currentName = "Custom";
				for(const auto& preset : presets) {
					if(std::abs(anchor_.x - preset.value.x) < 0.001f &&
					   std::abs(anchor_.y - preset.value.y) < 0.001f) {
						currentName = preset.name;
						break;
					}
				}

				if(ImGui::BeginCombo("Anchor Preset", currentName)) {
					for(const auto& preset : presets) {
						const bool selected = currentName == preset.name;
						if(ImGui::Selectable(preset.name, selected)) {
							SetAnchor(preset.value);
						}
						if(selected) ImGui::SetItemDefaultFocus();
					}
					ImGui::EndCombo();
				}

				Vector2 editedAnchor = anchor_;
				if(ImGui::DragFloat2("Anchor", &editedAnchor.x, 0.01f, 0.0f, 1.0f, "%.2f")) {
					SetAnchor(editedAnchor);
				}
				ImGui::TreePop();
			}
			GuiCmd::EndSection();
		}

		if(GuiCmd::BeginSection(ParamFilterSection::Material)) {
			if(ImGui::TreeNodeEx("Texture", ImGuiTreeNodeFlags_SpanAvailWidth)) {
				Guid droppedGuid = textureGuid_;
				if(AssetPanel::DrawAssetDropTarget(AssetType::Texture, &droppedGuid)) {
					ApplyTextureByGuid(droppedGuid);
				}
				ImGui::TextDisabled("Current: %s", AssetLabel(textureGuid_, AssetType::Texture).c_str());
				ImGui::ColorEdit4("Color", &color_.x);
				ImGui::TreePop();
			}
			GuiCmd::EndSection();
		}
	}

	void SpriteSceneObject2d::ApplyTextureByGuid(const Guid& guid) {
		if(!guid.isValid()) return;
		// Asset種別を検証してからGUIDと相対Pathを同時更新し、移動追従と旧形式互換を両立する。
		if(const AssetRecord* record = AssetDatabase::GetInstance()->Get(guid); record && record->type == AssetType::Texture) {
			textureGuid_ = guid;
			texturePath_ = RelativeAssetPath(record->sourcePath);
			EnsureSprite();
			sprite_->SetTexture(texturePath_);
		}
	}

	std::string SpriteSceneObject2d::ResolveTexturePath() const {
		// GUID解決を優先することでAsset移動へ追従し、未登録Assetだけ保存済みPathへFallbackする。
		if(textureGuid_.isValid()) {
			if(const AssetRecord* record = AssetDatabase::GetInstance()->Get(textureGuid_); record && record->type == AssetType::Texture) {
				return RelativeAssetPath(record->sourcePath);
			}
		}
		return texturePath_;
	}

	AnimatedSpriteSceneObject2d::AnimatedSpriteSceneObject2d() {
		SetName("AnimatedSprite2D", ObjectType::Object2D);
	}

	void AnimatedSpriteSceneObject2d::Initialize() {
		SpriteSceneObject2d::Initialize();
		animator_.Bind(sprite_.get());
		RefreshAnimationAsset();
	}

	void AnimatedSpriteSceneObject2d::Update(float dt) {
		UpdateAnimatedSprite(dt, true);
	}

	void AnimatedSpriteSceneObject2d::AlwaysUpdate(float dt) {
		const SceneContext* context = SceneContext::Current();
		// Runtimeでは通常Updateが進めるため、AlwaysUpdateとの二重更新を避ける。
		if(context && context->IsRuntime()) return;
		UpdateAnimatedSprite(dt, true);
	}

	void AnimatedSpriteSceneObject2d::UpdateAnimatedSprite(float dt, bool updateAnimation) {
		// Transform Animationを先に適用し、その配置でUV Animation付きSpriteを更新する。
		EnsureSprite();
		transformAnimation2d_.Update(worldTransform_, dt);
		worldTransform_.Update();
		SyncSpriteFromTransform();
		sprite_->SetColor(color_);
		// Hot Reloadや遅延Assetロード完了を検出してAnimatorの参照を更新する。
		RefreshAnimationAsset();
		if(updateAnimation) {
			if(autoPlay_ && !clipName_.empty() && animator_.GetCurrentClipName() != clipName_ && !animator_.IsFinished()) {
				PlayCurrentClip(true);
			}
			animator_.Update(dt);
		}
		sprite_->Update(dt);
	}

	void AnimatedSpriteSceneObject2d::ShowGui() {
		DrawBaseGui();

		if(GuiCmd::BeginSection(ParamFilterSection::ParameterData)) {
			if(ImGui::TreeNodeEx("Sprite Animation", ImGuiTreeNodeFlags_SpanAvailWidth)) {
				Guid droppedGuid = animationGuid_;
				if(AssetPanel::DrawAssetDropTarget(AssetType::SpriteAnimation, &droppedGuid)) {
					SetAnimationGuid(droppedGuid);
				}
				ImGui::TextDisabled("Current: %s", AssetLabel(animationGuid_, AssetType::SpriteAnimation).c_str());
				ImGui::SameLine();
				if(ImGui::Button("Create", ImVec2(72.0f, 0.0f))) {
					CreateAnimationAsset();
				}
				ImGui::Checkbox("Auto Play", &autoPlay_);
				if(ImGui::Checkbox("Override Loop", &useLoopOverride_)) {
					ApplyLoopSettings();
				}
				if(useLoopOverride_) {
					ImGui::SameLine();
					if(ImGui::Checkbox("Loop", &loopOverride_)) {
						ApplyLoopSettings();
					}
				}

				auto asset = animator_.GetAnimationAsset();
				if(asset && !asset->clips.empty()) {
					if(clipName_.empty()) clipName_ = asset->clips.front().name;
					const char* preview = clipName_.empty() ? "(none)" : clipName_.c_str();
					if(ImGui::BeginCombo("Clip", preview)) {
						for(const auto& clip : asset->clips) {
							const bool selected = clip.name == clipName_;
							if(ImGui::Selectable(clip.name.c_str(), selected)) {
								clipName_ = clip.name;
								PlayCurrentClip(true);
							}
							if(selected) ImGui::SetItemDefaultFocus();
						}
						ImGui::EndCombo();
					}
					if(ImGui::Button("Play", ImVec2(64.0f, 0.0f))) PlayCurrentClip(true);
					ImGui::SameLine();
					if(ImGui::Button("Stop", ImVec2(64.0f, 0.0f))) animator_.Stop();
					ImGui::TextDisabled("Frame: %d", animator_.GetCurrentFrame());
					DrawAnimationAssetEditor(*asset);
				}
				ImGui::TreePop();
			}
			GuiCmd::EndSection();
		}
	}

	void AnimatedSpriteSceneObject2d::ApplyConfigFromJson(const nlohmann::json& j) {
		// 基底Sprite設定を復元してからAnimation AssetをBindingし、Texture依存を解決する。
		SpriteSceneObject2d::ApplyConfigFromJson(j);
		animationGuid_ = j.value("animationGuid", animationGuid_);
		clipName_ = j.value("clipName", clipName_);
		autoPlay_ = j.value("autoPlay", autoPlay_);
		useLoopOverride_ = j.value("useLoopOverride", useLoopOverride_);
		loopOverride_ = j.value("loopOverride", loopOverride_);
		RefreshAnimationAsset();
		ApplyLoopSettings();
	}

	void AnimatedSpriteSceneObject2d::ExtractConfigToJson(nlohmann::json& j) const {
		SpriteSceneObject2d::ExtractConfigToJson(j);
		j["animationGuid"] = animationGuid_;
		j["clipName"] = clipName_;
		j["autoPlay"] = autoPlay_;
		j["useLoopOverride"] = useLoopOverride_;
		j["loopOverride"] = loopOverride_;
	}

	void AnimatedSpriteSceneObject2d::SetAnimationGuid(const Guid& guid) {
		animationGuid_ = guid;
		boundAnimationGuid_ = Guid::Empty();
		RefreshAnimationAsset();
	}

	void AnimatedSpriteSceneObject2d::RefreshAnimationAsset() {
		// 同じGUIDを既にBinding済みなら再設定せず、再生時刻の不要なResetを防ぐ。
		if(!animationGuid_.isValid() || animationGuid_ == boundAnimationGuid_) return;
		auto* manager = AssetManager::GetInstance()->GetDataAssetManager();
		auto asset = manager ? manager->GetAsset<SpriteAnimationAsset>(animationGuid_) : nullptr;
		if(!asset) return;
		if(clipName_.empty() && !asset->clips.empty()) {
			clipName_ = asset->clips.front().name;
		}
		// Animation Assetが指定するTextureをSpriteへ同期し、Frame UVと画像を一致させる。
		if(textureGuid_ != asset->textureGuid && asset->textureGuid.isValid()) {
			ApplyTextureByGuid(asset->textureGuid);
		} else if(!asset->texturePath.empty()) {
			texturePath_ = asset->texturePath;
			EnsureSprite();
			sprite_->SetTexture(texturePath_);
		}
		// SpriteはSceneObjectが所有し、Animatorには非所有参照としてBindingする。
		animator_.Bind(sprite_.get());
		animator_.SetAnimationAsset(asset);
		ApplyLoopSettings();
		boundAnimationGuid_ = animationGuid_;
		if(autoPlay_) PlayCurrentClip(true);
	}

	void AnimatedSpriteSceneObject2d::PlayCurrentClip(bool restart) {
		if(clipName_.empty()) return;
		ApplyLoopSettings();
		animator_.Play(clipName_, restart);
	}

	void AnimatedSpriteSceneObject2d::ApplyLoopSettings() {
		// Object側Overrideが無効ならClip Asset本来のLoop設定へ戻す。
		if(useLoopOverride_) {
			animator_.SetLoopOverride(loopOverride_);
		} else {
			animator_.ClearLoopOverride();
		}
	}

	void AnimatedSpriteSceneObject2d::DrawAnimationAssetEditor(SpriteAnimationAsset& asset) {
		if(!ImGui::TreeNodeEx("UV Animation Editor", ImGuiTreeNodeFlags_SpanAvailWidth)) return;

		bool changed = false;
		Guid textureGuid = asset.textureGuid;
		if(AssetPanel::DrawAssetDropTarget(AssetType::Texture, &textureGuid, 48.0f)) {
			asset.textureGuid = textureGuid;
			if(const AssetRecord* record = AssetDatabase::GetInstance()->Get(textureGuid)) {
				asset.texturePath = RelativeAssetPath(record->sourcePath);
			}
			SetAnimationGuid(animationGuid_);
			changed = true;
		}
		if(!asset.texturePath.empty()) {
			ImGui::TextDisabled("%s", asset.texturePath.c_str());
		}

		int division[2] = {
			(std::max)(1, asset.GetDivisionX()),
			(std::max)(1, asset.GetDivisionY())};
		if(ImGui::DragInt2("Division", division, 1.0f, 1, 128)) {
			asset.division = {
				static_cast<float>((std::max)(1, division[0])),
				static_cast<float>((std::max)(1, division[1]))};
			changed = true;
		}
		ImGui::TextDisabled("Frames: %d", asset.GetFrameCapacity());

		if(ImGui::Button("Add Clip", ImVec2(86.0f, 0.0f))) {
			SpriteAnimationClip clip;
			clip.name = "Clip " + std::to_string(asset.clips.size() + 1);
			asset.clips.push_back(clip);
			selectedClipIndex_ = static_cast<int>(asset.clips.size()) - 1;
			changed = true;
		}

		if(asset.clips.empty()) {
			asset.clips.push_back({});
			selectedClipIndex_ = 0;
			changed = true;
		}
		selectedClipIndex_ = std::clamp(selectedClipIndex_, 0, static_cast<int>(asset.clips.size()) - 1);

		if(ImGui::BeginTable("AnimatedSprite2DClips", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV)) {
			ImGui::TableSetupColumn("Clips", ImGuiTableColumnFlags_WidthFixed, 150.0f);
			ImGui::TableSetupColumn("Editor", ImGuiTableColumnFlags_None);
			ImGui::TableNextColumn();
			for(size_t i = 0; i < asset.clips.size(); ++i) {
				ImGui::PushID(static_cast<int>(i));
				if(ImGui::Selectable(asset.clips[i].name.c_str(), selectedClipIndex_ == static_cast<int>(i))) {
					selectedClipIndex_ = static_cast<int>(i);
				}
				ImGui::PopID();
			}

			ImGui::TableNextColumn();
			SpriteAnimationClip& clip = asset.clips[static_cast<size_t>(selectedClipIndex_)];
			std::array<char, 128> nameBuffer{};
			std::copy_n(clip.name.c_str(), (std::min)(clip.name.size(), nameBuffer.size() - 1), nameBuffer.data());
			if(ImGui::InputText("Name", nameBuffer.data(), nameBuffer.size())) {
				clip.name = nameBuffer.data();
				clipName_ = clip.name;
				changed = true;
			}
			changed |= ImGui::DragInt("Start Frame", &clip.startFrame, 1.0f, 0, (std::max)(0, asset.GetFrameCapacity() - 1));
			changed |= ImGui::DragInt("Frame Count", &clip.frameCount, 1.0f, 1, (std::max)(1, asset.GetFrameCapacity()));
			changed |= ImGui::DragFloat("Frame Duration", &clip.frameDuration, 0.005f, 0.001f, 10.0f, "%.3f");
			changed |= ImGui::Checkbox("Loop", &clip.loop);
			if(asset.clips.size() > 1 && ImGui::Button("Remove Clip", ImVec2(120.0f, 0.0f))) {
				asset.clips.erase(asset.clips.begin() + selectedClipIndex_);
				selectedClipIndex_ = std::clamp(selectedClipIndex_, 0, static_cast<int>(asset.clips.size()) - 1);
				clipName_ = asset.clips[static_cast<size_t>(selectedClipIndex_)].name;
				changed = true;
			}
			ImGui::EndTable();
		}

		if(changed) {
			boundAnimationGuid_ = Guid::Empty();
			RefreshAnimationAsset();
		}
		if(ImGui::Button("Save UV Animation", ImVec2(150.0f, 0.0f))) {
			SaveAnimationAsset(asset);
		}
		ImGui::TreePop();
	}

	void AnimatedSpriteSceneObject2d::SaveAnimationAsset(const SpriteAnimationAsset& asset) {
		if(!animationGuid_.isValid()) return;
		const AssetRecord* record = AssetDatabase::GetInstance()->Get(animationGuid_);
		if(!record) return;
		if(auto* manager = AssetManager::GetInstance()->GetDataAssetManager()) {
			manager->SaveAsset(asset, record->sourcePath);
			AssetDatabase::GetInstance()->RegisterOrUpdate(record->sourcePath, AssetType::SpriteAnimation);
		}
	}

	void AnimatedSpriteSceneObject2d::CreateAnimationAsset() {
		auto* db = AssetDatabase::GetInstance();
		auto* manager = AssetManager::GetInstance()->GetDataAssetManager();
		if(!db || !manager) return;
		std::filesystem::path folder = db->GetRoot() / "SpriteAnimations";
		std::filesystem::create_directories(folder);
		const std::string fileName = name_.empty() ? "AnimatedSprite2D.spriteanim" : name_ + ".spriteanim";
		std::filesystem::path path = folder / fileName;
		int suffix = 1;
		while(std::filesystem::exists(path)) {
			path = folder / (name_ + "_" + std::to_string(suffix++) + ".spriteanim");
		}

		const Guid guid = Guid::New();
		auto asset = manager->CreateSpriteAnimationAsset(path, guid, path.stem().string());
		if(!asset) return;
		if(textureGuid_.isValid()) {
			asset->textureGuid = textureGuid_;
			asset->texturePath = ResolveTexturePath();
			manager->SaveAsset(*asset, path);
		}
		db->RegisterOrUpdate(path, AssetType::SpriteAnimation);
		SetAnimationGuid(asset->GetGuid());
	}

} // namespace CalyxEngine

namespace {
	const bool registerSpriteSceneObject2d = [] {
		SceneObjectRegistry::Get().Register(
			"SpriteSceneObject2d",
			"SpriteSceneObject2d",
			ObjectType::Object2D,
			"",
			false,
			false,
			false,
			true,
			true,
			&CreateSceneObject<CalyxEngine::SpriteSceneObject2d>);
		return true;
	}();

	const bool registerAnimatedSpriteSceneObject2d = [] {
		SceneObjectRegistry::Get().Register(
			"AnimatedSpriteSceneObject2d",
			"AnimatedSpriteSceneObject2d",
			ObjectType::Object2D,
			"",
			false,
			false,
			false,
			true,
			true,
			&CreateSceneObject<CalyxEngine::AnimatedSpriteSceneObject2d>);
		return true;
	}();
}
