#include "SpriteAnimationEditorPanel.h"

#include <Engine\Application\UI\Panels\AssetPanel.h>
#include <Engine\Assets\Database\AssetDatabase.h>
#include <Engine\Assets\DataAsset\DataAssetManager.h>
#include <Engine\Assets\DataAsset\SpriteAnimationAsset.h>
#include <Engine\Assets\Manager\AssetManager.h>
#include <Engine\Assets\System\AssetRecord.h>

#include <externals\imgui\imgui.h>

#include <algorithm>
#include <array>
#include <fstream>

namespace CalyxEngine {
	namespace {
		std::shared_ptr<SpriteAnimationAsset> GetSpriteAnimationAsset(const Guid& guid) {
			// DataAssetManagerを所有元として共有Assetを取得し、Panel側ではLifetimeを管理しない。
			if(!guid.isValid()) return nullptr;
			if(auto* manager = AssetManager::GetInstance()->GetDataAssetManager()) {
				return manager->GetAsset<SpriteAnimationAsset>(guid);
			}
			return nullptr;
		}

		std::string RelativeAssetPath(const std::filesystem::path& path) {
			auto* db = AssetDatabase::GetInstance();
			if(!db) return path.generic_string();
			std::error_code ec;
			// Project移動後も参照できるようAsset root相対へ変換し、失敗時だけ元Pathを残す。
			auto rel = std::filesystem::relative(path, db->GetRoot(), ec);
			return ec ? path.generic_string() : rel.generic_string();
		}
	}

	SpriteAnimationEditorPanel::SpriteAnimationEditorPanel()
		: IEngineUI("Sprite Animation") {}

	void SpriteAnimationEditorPanel::Render() {
		if(!IsShow()) return;

		bool open = true;
		// Asset一覧と詳細Editorを分割し、選択変更時も同一Window内で編集を継続する。
		if(ImGui::Begin(panelName_.c_str(), &open)) {
			if(ImGui::BeginTable("SpriteAnimationEditorLayout", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV)) {
				ImGui::TableSetupColumn("Assets", ImGuiTableColumnFlags_WidthFixed, 240.0f);
				ImGui::TableSetupColumn("Editor", ImGuiTableColumnFlags_None);

				ImGui::TableNextColumn();
				DrawAssetList();

				ImGui::TableNextColumn();
				auto asset = GetSpriteAnimationAsset(selectedAnimation_);
				const AssetRecord* record = AssetDatabase::GetInstance()->Get(selectedAnimation_);
				if(asset && record) {
					DrawEditor(*asset, record->sourcePath);
				} else {
					ImGui::Dummy(ImVec2(0.0f, 32.0f));
					ImGui::TextDisabled("No sprite animation selected.");
					if(ImGui::Button("Create Sprite Animation", ImVec2(190.0f, 28.0f))) {
						CreateSpriteAnimationAsset();
					}
				}

				ImGui::EndTable();
			}
		}
		ImGui::End();
		if(!open) SetShow(false);
	}

	void SpriteAnimationEditorPanel::DrawAssetList() {
		ImGui::TextUnformatted("Sprite Animations");
		ImGui::SameLine();
		ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 78.0f);
		if(ImGui::Button("New", ImVec2(58.0f, 24.0f))) {
			CreateSpriteAnimationAsset();
		}
		ImGui::Separator();

		ImGui::BeginChild("##sprite-animation-list", ImVec2(0.0f, 0.0f), false);
		// Database ViewからSpriteAnimation種別だけを抽出し、GUIDを選択状態として保持する。
		for(auto* rec : AssetDatabase::GetInstance()->GetView()) {
			if(!rec || rec->type != AssetType::SpriteAnimation) continue;
			const bool selected = rec->guid == selectedAnimation_;
			ImGui::PushID(rec->guid.ToString().c_str());
			const std::string label = rec->sourcePath.stem().string();
			if(ImGui::Selectable(label.c_str(), selected, ImGuiSelectableFlags_None, ImVec2(0.0f, 28.0f))) {
				selectedAnimation_ = rec->guid;
				selectedClipIndex_ = 0;
			}
			ImGui::PopID();
		}
		ImGui::EndChild();
	}

	void SpriteAnimationEditorPanel::DrawEditor(SpriteAnimationAsset& asset, const std::filesystem::path& path) {
		ImGui::TextUnformatted(asset.GetName().c_str());
		ImGui::SameLine();
		ImGui::TextDisabled("%s", RelativeAssetPath(path).c_str());
		ImGui::SameLine();
		if(ImGui::Button("Save", ImVec2(76.0f, 0.0f))) {
			Save(asset, path);
		}

		ImGui::Separator();
		DrawTexture(asset);
		ImGui::Separator();
		DrawClips(asset);
	}

	void SpriteAnimationEditorPanel::DrawTexture(SpriteAnimationAsset& asset) {
		ImGui::TextUnformatted("Texture");
		Guid textureGuid = asset.textureGuid;
		// Texture Drop時はGUIDと相対Pathを同時更新し、移動追従と旧形式互換を両立する。
		if(AssetPanel::DrawAssetDropTarget(AssetType::Texture, &textureGuid, 48.0f)) {
			asset.textureGuid = textureGuid;
			if(const AssetRecord* record = AssetDatabase::GetInstance()->Get(textureGuid)) {
				asset.texturePath = RelativeAssetPath(record->sourcePath);
			}
		}

		if(const AssetRecord* texture = AssetDatabase::GetInstance()->Get(asset.textureGuid)) {
			ImGui::TextDisabled("%s", RelativeAssetPath(texture->sourcePath).c_str());
			if(texture->previewTex) {
				ImGui::Image(texture->previewTex, ImVec2(128.0f, 128.0f));
			}
		} else if(!asset.texturePath.empty()) {
			// GUIDがない旧AssetではPathを直接編集できるFallback UIを表示する。
			std::array<char, 260> buffer{};
			std::copy_n(asset.texturePath.c_str(), (std::min)(asset.texturePath.size(), buffer.size() - 1), buffer.data());
			if(ImGui::InputText("Texture Path", buffer.data(), buffer.size())) {
				asset.texturePath = buffer.data();
			}
		}

		// 0分割を許可せず、Texture SheetのFrame容量を常に有効値として計算する。
		int division[2] = {
			(std::max)(1, asset.GetDivisionX()),
			(std::max)(1, asset.GetDivisionY())};
		if(ImGui::DragInt2("Division", division, 1.0f, 1, 128)) {
			asset.division = {
				static_cast<float>((std::max)(1, division[0])),
				static_cast<float>((std::max)(1, division[1]))};
		}
		ImGui::TextDisabled("Frames: %d", asset.GetFrameCapacity());
	}

	void SpriteAnimationEditorPanel::DrawClips(SpriteAnimationAsset& asset) {
		ImGui::TextUnformatted("Clips");
		ImGui::SameLine();
		if(ImGui::Button("+", ImVec2(28.0f, 24.0f))) {
			SpriteAnimationClip clip;
			clip.name = "Clip " + std::to_string(asset.clips.size() + 1);
			asset.clips.push_back(clip);
			selectedClipIndex_ = static_cast<int>(asset.clips.size()) - 1;
		}

		// Animatorが選択可能なClipを必ず一つ持つよう、空配列には既定Clipを補完する。
		if(asset.clips.empty()) {
			asset.clips.push_back({});
		}
		// Clip削除やAsset切替後の選択Indexを有効範囲へ補正する。
		selectedClipIndex_ = std::clamp(selectedClipIndex_, 0, static_cast<int>(asset.clips.size()) - 1);

		if(ImGui::BeginTable("SpriteAnimationClips", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV)) {
			ImGui::TableSetupColumn("ClipList", ImGuiTableColumnFlags_WidthFixed, 180.0f);
			ImGui::TableSetupColumn("ClipEditor", ImGuiTableColumnFlags_None);
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
			}
			ImGui::DragInt("Start Frame", &clip.startFrame, 1.0f, 0, (std::max)(0, asset.GetFrameCapacity() - 1));
			ImGui::DragInt("Frame Count", &clip.frameCount, 1.0f, 1, asset.GetFrameCapacity());
			ImGui::DragFloat("Frame Duration", &clip.frameDuration, 0.005f, 0.001f, 10.0f, "%.3f");
			ImGui::Checkbox("Loop", &clip.loop);
			// 最後の一つは削除不可とし、空Clip配列の保存を防ぐ。
			if(asset.clips.size() > 1 && ImGui::Button("Remove Clip", ImVec2(120.0f, 0.0f))) {
				asset.clips.erase(asset.clips.begin() + selectedClipIndex_);
				selectedClipIndex_ = std::clamp(selectedClipIndex_, 0, static_cast<int>(asset.clips.size()) - 1);
			}
			ImGui::EndTable();
		}
	}

	void SpriteAnimationEditorPanel::CreateSpriteAnimationAsset() {
		auto* db = AssetDatabase::GetInstance();
		std::filesystem::path folder = db->GetRoot() / "SpriteAnimations";
		std::filesystem::create_directories(folder);

		// 既存Assetを上書きしない連番Pathを探索する。
		std::filesystem::path path = folder / "New Sprite Animation.spriteanim";
		for(int i = 1; std::filesystem::exists(path); ++i) {
			path = folder / ("New Sprite Animation " + std::to_string(i) + ".spriteanim");
		}

		{
			std::ofstream ofs(path);
			if(!ofs) return;
		}

		// 空ファイルをDatabase登録してDataAssetを生成し、初期値を正式形式で保存し直す。
		const Guid guid = db->RegisterOrUpdate(path, AssetType::SpriteAnimation);
		if(auto asset = GetSpriteAnimationAsset(guid)) {
			asset->SetName(path.stem().string());
			Save(*asset, path);
		}
		db->Scan();
		selectedAnimation_ = guid;
		selectedClipIndex_ = 0;
	}

	void SpriteAnimationEditorPanel::Save(SpriteAnimationAsset& asset, const std::filesystem::path& path) {
		// DataAssetManagerで内容を保存後、Database Metadataも同じPathへ同期する。
		if(auto* manager = AssetManager::GetInstance()->GetDataAssetManager()) {
			manager->SaveAsset(asset, path);
		}
		AssetDatabase::GetInstance()->RegisterOrUpdate(path, AssetType::SpriteAnimation);
	}

} // namespace CalyxEngine
