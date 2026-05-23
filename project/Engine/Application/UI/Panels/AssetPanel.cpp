#define IMGUI_DEFINE_MATH_OPERATORS
#include "AssetPanel.h"

#include "Engine/Assets/Manager/AssetManager.h"

#include <Data/Engine/Prefab/Serializer/PrefabSerializer.h>
#include <Engine/Assets/Database/AssetDatabase.h>
#include <Engine/Assets/DataAsset/MaterialAsset.h>
#include <Engine/Foundation/Debug/CxAssert.h>
#include <Engine/Objects/3D/Actor/SceneObject.h>

#include <externals/imgui/ImGuiFileDialog.h>
#include <externals/imgui/imgui_internal.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <system_error>
#include <vector>

namespace CalyxEngine {
	namespace {

		std::string StripTrailingNumberSuffix(const std::string& name) {
			if(name.size() < 3 || name.back() != ')') return name;

			const auto open = name.find_last_of('(');
			if(open == std::string::npos || open + 1 >= name.size() - 1) return name;

			for(size_t i = open + 1; i < name.size() - 1; ++i) {
				if(!std::isdigit(static_cast<unsigned char>(name[i]))) {
					return name;
				}
			}

			return name.substr(0, open);
		}

		std::string SanitizeAssetFileStem(std::string name) {
			name = StripTrailingNumberSuffix(name);
			if(name.empty()) name = "NewPrefab";
			for(char& c : name) {
				const unsigned char uc = static_cast<unsigned char>(c);
				if(c == '<' || c == '>' || c == ':' || c == '"' || c == '/' || c == '\\' ||
				   c == '|' || c == '?' || c == '*' || std::iscntrl(uc)) {
					c = '_';
				}
			}
			while(!name.empty() && (name.back() == ' ' || name.back() == '.')) {
				name.pop_back();
			}
			return name.empty() ? "NewPrefab" : name;
		}

		std::filesystem::path MakeUniquePrefabPath(const std::filesystem::path& folder,
												   const std::string& preferredStem) {
			const std::string stem = SanitizeAssetFileStem(preferredStem);
			std::filesystem::path path = folder / (stem + ".prefab");
			for(int i = 1; std::filesystem::exists(path); ++i) {
				path = folder / (stem + " " + std::to_string(i) + ".prefab");
			}
			return path;
		}

		void SetPrefabLinkRecursive(SceneObject* object, const Guid& prefabGuid) {
			if(!object || !prefabGuid.isValid()) return;
			object->SetPrefabLink(prefabGuid, object->GetGuid());
			for(const auto& child : object->GetChildren()) {
				SetPrefabLinkRecursive(child.get(), prefabGuid);
			}
		}

		const char* AssetTypeName(AssetType type) {
			switch(type) {
			case AssetType::Texture: return "Texture";
			case AssetType::Model: return "Model";
			case AssetType::Shader: return "Shader";
			case AssetType::Material: return "Material";
			case AssetType::Audio: return "Audio";
			case AssetType::Prefab: return "Prefab";
			case AssetType::Effect: return "Effect";
			case AssetType::SpriteAnimation: return "Sprite Animation";
			case AssetType::Unknown:
			default:
				return "Unknown";
			}
		}

		void WarnRejectedAssetDrop(AssetType expected, const AssetDragPayload& payload) {
			std::string name = "(unknown asset)";
			if(auto* db = AssetDatabase::GetInstance()) {
				if(const AssetRecord* record = db->Get(payload.guid)) {
					name = record->sourcePath.filename().string();
				}
			}

			std::string message;
			if(payload.type == AssetType::Unknown) {
				message = "Unsupported asset extension: " + name;
			} else {
				message = "Cannot drop " + std::string(AssetTypeName(payload.type)) +
						  " into " + AssetTypeName(expected) + " slot: " + name;
			}
			CX_WARN(message.c_str());
		}

	} // namespace

	void AssetPanel::Initialize(const std::filesystem::path& assetsRoot) {
		assetsRootAbs_	  = std::filesystem::weakly_canonical(assetsRoot);
		currentFolderAbs_ = assetsRootAbs_;

		// アイコン（存在しなければ任意の代替に差し替え）
		auto& tm	 = *AssetManager::GetInstance()->GetTextureManager();
		iconFolder_	 = (ImTextureID)tm.LoadTexture("UI/Tool/AssetPanel/folder.dds").ptr;
		iconGeneric_ = (ImTextureID)tm.LoadTexture("UI/Tool/AssetPanel/generic.dds").ptr;

		needsRebuildTree_ = true;
		search_[0]		  = '\0';
		scope_			  = Scope::SelectedFolder;
	}

	void AssetPanel::Render() {
		bool isopen = true;
		ImGui::Begin(panelName_.c_str(), &isopen, ImGuiWindowFlags_MenuBar);
		DrawMenuBar();
		DrawToolbar();

		// レイアウト（左右）
		if(showLeftTree_) {
			ImGui::Columns(2, nullptr, true);
			ImGui::SetColumnWidth(0, leftWidth_);

			ImGui::BeginChild("##left-tree", ImVec2(0, 0), true);
			DrawLeftTree();
			ImGui::EndChild();

			ImGui::NextColumn();
			ImGui::BeginChild("##right-view", ImVec2(0, 0), false);
			DrawRightView();
			ImGui::EndChild();

			ImGui::Columns(1);
		} else {
			// 1カラム（右ビューのみ）
			ImGui::BeginChild("##right-view-1col", ImVec2(0, 0), false);
			DrawRightView();
			ImGui::EndChild();
		}

		ImGui::End();
		if(!isopen) {
			SetShow(false);
		}
	}

	void AssetPanel::DrawMenuBar() {
		if(ImGui::BeginMenuBar()) {
			if(ImGui::BeginMenu("Layout")) {
				bool one = !showLeftTree_;
				bool two = showLeftTree_;
				if(ImGui::MenuItem("One Column Layout", nullptr, one)) showLeftTree_ = !one;
				if(ImGui::MenuItem("Two Column Layout", nullptr, two)) showLeftTree_ = true;
				ImGui::EndMenu();
			}
			if(ImGui::BeginMenu("View")) {
				ImGui::Checkbox("Grid", &gridMode_);
				ImGui::SliderFloat("Thumb", &thumbSize_, 48.0f, 160.0f, "%.0f px");
				ImGui::EndMenu();
			}
			if(ImGui::BeginMenu("Assets")) {
				if(ImGui::MenuItem("New Material")) {
					CreateMaterialAssetInCurrentFolder();
				}
				if(ImGui::MenuItem("Rescan")) {
					AssetDatabase::GetInstance()->Scan();
					needsRebuildTree_ = true;
				}
				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		}
	}

	void AssetPanel::DrawToolbar() {
		ImGui::Separator();

		// 検索
		ImGui::SetNextItemWidth(240);
		ImGui::InputTextWithHint("##search", "Search...", search_, IM_ARRAYSIZE(search_));

		ImGui::SameLine();
		if(typeFilter_.has_value()) {
			ImGui::TextDisabled("Filter:");
			ImGui::SameLine();
			const char* tname = "(unknown)";
			switch(*typeFilter_) {
			case AssetType::Texture:
				tname = "Texture";
				break;
			case AssetType::Model:
				tname = "Model";
				break;
			case AssetType::Shader:
				tname = "Shader";
				break;
			case AssetType::Material:
				tname = "Material";
				break;
			case AssetType::Audio:
				tname = "Audio";
				break;
			case AssetType::Prefab:
				tname = "Prefab";
				break;
			case AssetType::SpriteAnimation:
				tname = "SpriteAnimation";
				break;
			default:
				break;
			}
			ImGui::TextUnformatted(tname);
			ImGui::SameLine();
			if(ImGui::SmallButton("Clear##type")) {
				typeFilter_.reset();
			}
		}

		ImGui::Separator();
	}

	void AssetPanel::DrawLeftTree() {
		EnsureFolderTreeBuilt();

		DrawFavorites();
		ImGui::Separator();

		if(ImGui::TreeNodeEx("Assets", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanFullWidth)) {
			DrawDirNode(rootNode_.get());
			ImGui::TreePop();
		}
	}

	void AssetPanel::CreateMaterialAssetInCurrentFolder() {
		std::filesystem::path folder = currentFolderAbs_.empty() ? assetsRootAbs_ : currentFolderAbs_;

		std::filesystem::path path = folder / "New Material.mat";
		for(int i = 1; std::filesystem::exists(path); ++i) {
			path = folder / ("New Material " + std::to_string(i) + ".mat");
		}

		{
			std::ofstream ofs(path);
			if(!ofs) return;
		}

		auto* db = AssetDatabase::GetInstance();
		const Guid guid = db->RegisterOrUpdate(path, AssetType::Material);
		if(auto* dataAssets = AssetManager::GetInstance()->GetDataAssetManager()) {
			auto asset = dataAssets->GetAsset<MaterialAsset>(guid);
			if(asset) {
				asset->SetName(path.stem().string());
				dataAssets->SaveAsset(*asset, path);
			}
		}

		db->Scan();
		cacheValid_ = false;
		needsRebuildTree_ = true;
	}

	void AssetPanel::CreatePrefabFromSceneObject(SceneObject* object,
												 const std::filesystem::path& folder) {
		if(!object) return;

		std::error_code ec;
		std::filesystem::create_directories(folder, ec);
		if(ec) return;

		const std::filesystem::path path = MakeUniquePrefabPath(folder, object->GetName());
		if(!PrefabSerializer::Save(
			   {object},
			   path.string(),
			   PrefabSerializer::SaveOptions{true})) return;

		auto* db = AssetDatabase::GetInstance();
		const Guid prefabGuid = db->RegisterOrUpdate(path, AssetType::Prefab);
		if(prefabGuid.isValid()) {
			SetPrefabLinkRecursive(object, prefabGuid);
		}
		db->Scan();

		cacheValid_ = false;
		needsRebuildTree_ = true;
	}

	void AssetPanel::BeginRenameAsset(const std::filesystem::path& path) {
		renamingAsset_ = true;
		renameAssetPath_ = path;
		const std::string stem = path.stem().string();
		snprintf(renameAssetBuf_, sizeof(renameAssetBuf_), "%s", stem.c_str());
	}

	void AssetPanel::CommitRenameAsset() {
		if(!renamingAsset_ || renameAssetPath_.empty()) return;

		std::string newStem = renameAssetBuf_;
		const auto first = newStem.find_first_not_of(" \t\r\n");
		if(first == std::string::npos) {
			newStem.clear();
		} else {
			const auto last = newStem.find_last_not_of(" \t\r\n");
			newStem = newStem.substr(first, last - first + 1);
		}
		if(newStem.empty()) {
			renamingAsset_ = false;
			renameAssetPath_.clear();
			return;
		}

		const std::filesystem::path oldPath = renameAssetPath_;
		std::filesystem::path newPath = oldPath.parent_path() / (newStem + oldPath.extension().string());
		if(newPath != oldPath && !std::filesystem::exists(newPath)) {
			std::error_code ec;
			std::filesystem::rename(oldPath, newPath, ec);
			if(!ec) {
				std::filesystem::path oldMeta = oldPath;
				oldMeta += ".meta";
				std::filesystem::path newMeta = newPath;
				newMeta += ".meta";
				if(std::filesystem::exists(oldMeta) && !std::filesystem::exists(newMeta)) {
					std::filesystem::rename(oldMeta, newMeta, ec);
				}
			}
		}

		renamingAsset_ = false;
		renameAssetPath_.clear();
		if(auto* db = AssetDatabase::GetInstance()) {
			db->Scan();
		}
		cacheValid_ = false;
		needsRebuildTree_ = true;
	}

	bool AssetPanel::AcceptSceneObjectPrefabDrop(const std::filesystem::path& folder) {
		bool accepted = false;
		if(ImGui::BeginDragDropTarget()) {
			if(const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SceneObjectPtr")) {
				if(payload->Data && payload->DataSize == sizeof(SceneObject*)) {
					SceneObject* object = *reinterpret_cast<SceneObject**>(payload->Data);
					CreatePrefabFromSceneObject(object, folder);
					accepted = true;
				}
			}
			ImGui::EndDragDropTarget();
		}
		return accepted;
	}

	bool AssetPanel::AcceptSceneObjectPrefabDropOnCurrentWindow(const std::filesystem::path& folder) {
		ImGuiWindow* window = ImGui::GetCurrentWindowRead();
		if(!window || window->SkipItems) return false;

		const ImVec2 windowPos = ImGui::GetWindowPos();
		const ImVec2 minRel = ImGui::GetWindowContentRegionMin();
		const ImVec2 maxRel = ImGui::GetWindowContentRegionMax();
		const ImRect dropRect(
			ImVec2(windowPos.x + minRel.x, windowPos.y + minRel.y),
			ImVec2(windowPos.x + maxRel.x, windowPos.y + maxRel.y));

		bool accepted = false;
		if(ImGui::BeginDragDropTargetCustom(dropRect, window->ID)) {
			if(const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SceneObjectPtr")) {
				if(payload->Data && payload->DataSize == sizeof(SceneObject*)) {
					SceneObject* object = *reinterpret_cast<SceneObject**>(payload->Data);
					CreatePrefabFromSceneObject(object, folder);
					accepted = true;
				}
			}
			ImGui::EndDragDropTarget();
		}
		return accepted;
	}

	/* ===================== 右ペイン ===================== */
	static inline void toLowerInplace(std::string& s) {
		std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return (char)std::tolower(c); });
	}

	static bool HasExtension(const std::filesystem::path& path, const char* expectedExt) {
		std::string ext = path.extension().string();
		toLowerInplace(ext);
		return ext == expectedExt;
	}

	static bool IsPngPreviewSidecar(const AssetRecord& rec) {
		if(rec.type != AssetType::Texture || !HasExtension(rec.sourcePath, ".png")) return false;

		std::filesystem::path ddsPath = rec.sourcePath;
		ddsPath.replace_extension(".dds");
		return std::filesystem::exists(ddsPath);
	}
	
	void AssetPanel::DrawRightView() {
		auto&		db	  = *AssetDatabase::GetInstance();
		const auto& items = db.GetView();

		const bool	hasSearch = (search_[0] != '\0');
		std::string searchStr = std::string(search_);
		toLowerInplace(searchStr);

		// キャッシュのキーが変わったら再構築
		bool keyChanged =
			(cacheScope_ != scope_) ||
			(cacheType_ != typeFilter_) ||
			(cacheFolder_ != currentFolderAbs_) ||
			(cacheSearch_ != searchStr) ||
			(cacheItemsCount_ != items.size());

		if(!cacheValid_ || keyChanged) {
			cacheValid_		 = true;
			cacheScope_		 = scope_;
			cacheType_		 = typeFilter_;
			cacheFolder_	 = currentFolderAbs_;
			cacheSearch_	 = searchStr;
			cacheItemsCount_ = items.size();

			// 1) サブフォルダ（SelectedFolder & 非検索時のみ）
			cacheSubDirs_.clear();
			if(!hasSearch && scope_ == Scope::SelectedFolder) {
				cacheSubDirs_.reserve(16);
				ListSubdirectories(currentFolderAbs_, cacheSubDirs_);
			}

			// 2) この階層（または配下）にあるアセットを抽出
			cacheFilesHere_.clear();
			cacheFilesHere_.reserve(items.size());

			// 事前に現在フォルダ lower を1回だけ作る（IsInFolderの高速版）
			std::string curFolderLower = NormalizeLower(currentFolderAbs_);

			for(auto* rec : items) {
				if(!rec) continue;
				if(IsPngPreviewSidecar(*rec)) continue;

				// スコープ
				if(scope_ == Scope::SelectedFolder) {
					if(!hasSearch) {
						// 同一階層のみ（親パス文字列で比較）
						std::string parentLower = rec->sourcePath.parent_path().lexically_normal().generic_string();
						toLowerInplace(parentLower);
						if(parentLower != curFolderLower) continue;
					} else {
						// 検索時は配下すべて
						if(!IsUnder(rec->sourcePath, currentFolderAbs_)) continue;
					}
				}
				// タイプ
				if(typeFilter_.has_value() && rec->type != *typeFilter_) continue;

				// 検索
				if(!searchStr.empty()) {
					auto fname = FilenameNoExt(rec->sourcePath);
					toLowerInplace(fname);
					if(fname.find(searchStr) == std::string::npos) continue;
				}

				cacheFilesHere_.push_back(rec);
			}

			// ソート（名前）
			std::sort(cacheFilesHere_.begin(), cacheFilesHere_.end(),
					  [](const AssetRecord* a, const AssetRecord* b) {
						  return a->sourcePath.filename().string() < b->sourcePath.filename().string();
					  });
		}

		// 3) 表示：フォルダ → ファイル（可視行のみ描画）
		const float cell	= thumbSize_ + 18.0f;
		const int	columns = (std::max)(1, (int)std::floor(ImGui::GetContentRegionAvail().x / cell));

		// --- フォルダ（検索中は出さない）---
		if(!hasSearch && scope_ == Scope::SelectedFolder) {
			if(gridMode_) ImGui::Columns(columns, nullptr, false);
			for(auto& dir : cacheSubDirs_) {
				ImGui::BeginGroup();
				ImGui::Image(iconFolder_ ? iconFolder_ : iconGeneric_, ImVec2(thumbSize_, thumbSize_));
				AcceptSceneObjectPrefabDrop(dir);
				ImGui::TextWrapped("%s", dir.filename().string().c_str());
				if(ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
					currentFolderAbs_ = dir;
					cacheValid_		  = false; // 次フレームで再構築
				}
				ImGui::EndGroup();
				if(gridMode_) ImGui::NextColumn();
			}
			if(gridMode_) ImGui::Columns(1);
			if(!cacheSubDirs_.empty()) ImGui::Separator();
		}

		// --- ファイル（クリッピングあり）---
		if(!gridMode_) {
			// List：1行=1アイテム → ListClipper で可視分のみ描画
			ImGuiListClipper clip;
			clip.Begin((int)cacheFilesHere_.size());
			while(clip.Step()) {
				for(int i = clip.DisplayStart; i < clip.DisplayEnd; ++i) {
					const AssetRecord* rec = cacheFilesHere_[i];

					// サムネボタン（20x20）
					ImGui::PushID(&rec->guid);
					ImGui::BeginGroup();

					ImTextureID thumb =
						((rec->type == AssetType::Texture) && rec->previewTex)
							? rec->previewTex
							: (iconGeneric_ ? iconGeneric_ : nullptr);

					if(thumb)
						ImGui::ImageButton("##thumb", thumb, ImVec2(20, 20));
					else
						ImGui::Button("No Preview", ImVec2(20, 20));
					AcceptSceneObjectPrefabDrop(currentFolderAbs_);

					if(ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
						AssetDragPayload payload{rec->type, rec->guid};
						ImGui::SetDragDropPayload("CALYX_ASSET", &payload, sizeof(payload));
						ImGui::TextUnformatted(rec->sourcePath.filename().string().c_str());
						ImGui::EndDragDropSource();
					}

					ImGui::SameLine();
					if(renamingAsset_ && renameAssetPath_ == rec->sourcePath) {
						ImGui::SetKeyboardFocusHere();
						ImGui::SetNextItemWidth((std::max)(120.0f, ImGui::GetContentRegionAvail().x));
						if(ImGui::InputText("##RenameAsset", renameAssetBuf_, sizeof(renameAssetBuf_),
											ImGuiInputTextFlags_EnterReturnsTrue |
												ImGuiInputTextFlags_AutoSelectAll)) {
							CommitRenameAsset();
						}
						if(ImGui::IsItemDeactivatedAfterEdit()) {
							CommitRenameAsset();
						}
						if(ImGui::IsKeyPressed(ImGuiKey_Escape)) {
							renamingAsset_ = false;
							renameAssetPath_.clear();
						}
					} else {
						ImGui::TextUnformatted(rec->sourcePath.filename().string().c_str());
					}
					if(ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
						AssetDragPayload payload{rec->type, rec->guid};
						ImGui::SetDragDropPayload("CALYX_ASSET", &payload, sizeof(payload));
						ImGui::TextUnformatted(rec->sourcePath.filename().string().c_str());
						ImGui::EndDragDropSource();
					}
					if(rec->type == AssetType::Prefab && ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
						if(onPrefabEditRequested_) onPrefabEditRequested_(rec->sourcePath);
					}
					if(ImGui::BeginPopupContextItem("AssetContext")) {
						if(rec->type == AssetType::Prefab && ImGui::MenuItem("Edit Prefab")) {
							if(onPrefabEditRequested_) onPrefabEditRequested_(rec->sourcePath);
						}
						if(ImGui::MenuItem("Rename")) {
							BeginRenameAsset(rec->sourcePath);
						}
						ImGui::EndPopup();
					}

					ImGui::EndGroup();
					ImGui::PopID();
				}
			}
			const float dropHeight = (std::max)(48.0f, ImGui::GetContentRegionAvail().y);
			ImGui::InvisibleButton("##PrefabDropTargetList", ImVec2(ImGui::GetContentRegionAvail().x, dropHeight));
			AcceptSceneObjectPrefabDrop(currentFolderAbs_);
			AcceptSceneObjectPrefabDropOnCurrentWindow(currentFolderAbs_);
			return;
		}

		// Grid：行単位でクリップ（1行=columns個）
		const int total = (int)cacheFilesHere_.size();
		const int rows	= (total + columns - 1) / columns;

		ImGui::Columns(columns, nullptr, false);

		ImGuiListClipper clip;
		clip.Begin(rows);
		while(clip.Step()) {
			const int startIdx = clip.DisplayStart * columns;
			const int endIdx   = (std::min)(total, clip.DisplayEnd * columns);

			// 行頭の列位置調整
			int col = startIdx % columns;
			for(int k = 0; k < col; ++k) ImGui::NextColumn();

			for(int i = startIdx; i < endIdx; ++i) {
				const AssetRecord* rec = cacheFilesHere_[i];
				ImGui::PushID(&rec->guid);
				ImGui::BeginGroup();

				ImVec2		sz(thumbSize_, thumbSize_);
				ImTextureID thumb = ((rec->type == AssetType::Texture) && rec->previewTex)
										 ? rec->previewTex
										 : (iconGeneric_ ? iconGeneric_ : nullptr);

				if(thumb)
					ImGui::ImageButton("##thumb", thumb, sz);
				else
					ImGui::Button("No Preview", sz);
				AcceptSceneObjectPrefabDrop(currentFolderAbs_);

				if(ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
					AssetDragPayload payload{rec->type, rec->guid};
					ImGui::SetDragDropPayload("CALYX_ASSET", &payload, sizeof(payload));
					ImGui::TextUnformatted(rec->sourcePath.filename().string().c_str());
					ImGui::EndDragDropSource();
				}

				if(renamingAsset_ && renameAssetPath_ == rec->sourcePath) {
					ImGui::SetKeyboardFocusHere();
					ImGui::SetNextItemWidth(thumbSize_);
					if(ImGui::InputText("##RenameAsset", renameAssetBuf_, sizeof(renameAssetBuf_),
										ImGuiInputTextFlags_EnterReturnsTrue |
											ImGuiInputTextFlags_AutoSelectAll)) {
						CommitRenameAsset();
					}
					if(ImGui::IsItemDeactivatedAfterEdit()) {
						CommitRenameAsset();
					}
					if(ImGui::IsKeyPressed(ImGuiKey_Escape)) {
						renamingAsset_ = false;
						renameAssetPath_.clear();
					}
				} else {
					ImGui::TextWrapped("%s", rec->sourcePath.filename().string().c_str());
				}
				if(ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
					AssetDragPayload payload{rec->type, rec->guid};
					ImGui::SetDragDropPayload("CALYX_ASSET", &payload, sizeof(payload));
					ImGui::TextUnformatted(rec->sourcePath.filename().string().c_str());
					ImGui::EndDragDropSource();
				}
				if(rec->type == AssetType::Prefab && ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
					if(onPrefabEditRequested_) onPrefabEditRequested_(rec->sourcePath);
				}
				if(ImGui::BeginPopupContextItem("AssetContext")) {
					if(rec->type == AssetType::Prefab && ImGui::MenuItem("Edit Prefab")) {
						if(onPrefabEditRequested_) onPrefabEditRequested_(rec->sourcePath);
					}
					if(ImGui::MenuItem("Rename")) {
						BeginRenameAsset(rec->sourcePath);
					}
					ImGui::EndPopup();
				}

				ImGui::EndGroup();
				ImGui::PopID();

				ImGui::NextColumn();
				if(++col == columns) col = 0;
			}

			// 行末の列埋め（次行の先頭に戻す）
			while(col++ && col <= columns) ImGui::NextColumn();
		}

		ImGui::Columns(1);
		const float dropHeight = (std::max)(48.0f, ImGui::GetContentRegionAvail().y);
		ImGui::InvisibleButton("##PrefabDropTargetGrid", ImVec2(ImGui::GetContentRegionAvail().x, dropHeight));
		AcceptSceneObjectPrefabDrop(currentFolderAbs_);
		AcceptSceneObjectPrefabDropOnCurrentWindow(currentFolderAbs_);
	}

	void AssetPanel::DrawFavorites() {
		if(ImGui::TreeNodeEx("Favorites", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanFullWidth)) {
			if(ImGui::Selectable("All Textures")) {
				typeFilter_ = AssetType::Texture;
				scope_		= Scope::All;
			}
			if(ImGui::Selectable("All Models")) {
				typeFilter_ = AssetType::Model;
				scope_		= Scope::All;
			}
			if(ImGui::Selectable("All Shaders")) {
				typeFilter_ = AssetType::Shader;
				scope_		= Scope::All;
			}
			if(ImGui::Selectable("All Audio")) {
				typeFilter_ = AssetType::Audio;
				scope_		= Scope::All;
			}
			if(ImGui::Selectable("All Materials")) {
				typeFilter_ = AssetType::Material;
				scope_		= Scope::All;
			}
			if(ImGui::Selectable("All Prefabs")) {
				typeFilter_ = AssetType::Prefab;
				scope_		= Scope::All;
			}
			if(ImGui::Selectable("All Sprite Animations")) {
				typeFilter_ = AssetType::SpriteAnimation;
				scope_		= Scope::All;
			}
			ImGui::TreePop();
		}
	}

	bool AssetPanel::DrawAssetDropTarget(AssetType expect, Guid* inoutGuid, float height) {
		if(!inoutGuid) return false;

		ImGui::PushID(inoutGuid);
		ImVec2 dropSize(ImGui::GetContentRegionAvail().x, height);
		ImGui::InvisibleButton("##AssetDropTarget", dropSize);

		const bool hovered = ImGui::IsItemHovered();
		const ImVec2 rmin = ImGui::GetItemRectMin();
		const ImVec2 rmax = ImGui::GetItemRectMax();
		ImGui::GetWindowDrawList()->AddRect(
			rmin, rmax,
			hovered ? IM_COL32(120, 180, 255, 220) : IM_COL32(90, 90, 90, 160),
			8.0f, 0, 2.0f);

		const char* label = "Drop Asset here";
		switch(expect) {
		case AssetType::Texture: label = "Drop Texture here"; break;
		case AssetType::Material: label = "Drop Material here"; break;
		case AssetType::Model: label = "Drop Model here"; break;
		case AssetType::Prefab: label = "Drop Prefab here"; break;
		case AssetType::SpriteAnimation: label = "Drop Sprite Animation here"; break;
		default: break;
		}
		ImGui::GetWindowDrawList()->AddText(
			ImVec2(rmin.x + 8.0f, rmin.y + 8.0f),
			IM_COL32(230, 230, 230, 255),
			label);

		bool changed = false;
		if(ImGui::BeginDragDropTarget()) {
			if(const ImGuiPayload* p = ImGui::AcceptDragDropPayload("CALYX_ASSET")) {
				const AssetDragPayload payload =
					*reinterpret_cast<const AssetDragPayload*>(p->Data);
				if(payload.type == expect) {
					*inoutGuid = payload.guid;
					changed = true;
				} else {
					WarnRejectedAssetDrop(expect, payload);
				}
			}
			ImGui::EndDragDropTarget();
		}

		ImGui::PopID();
		return changed;
	}

	void AssetPanel::DrawDirNode(DirNode* node) {
		if(!node) return;

		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow |
								   ImGuiTreeNodeFlags_SpanFullWidth;

		// スキャン済みで子がないならリーフ。未スキャンならとりあえず矢印出す
		if(node->scanned && node->children.empty()) flags |= ImGuiTreeNodeFlags_Leaf;

		bool open = ImGui::TreeNodeEx(node, flags, "%s", node->name.c_str());

		// クリックでフォルダ選択
		if(ImGui::IsItemClicked()) {
			currentFolderAbs_ = node->absPath;
			scope_			  = Scope::SelectedFolder;
			typeFilter_.reset();
		}
		AcceptSceneObjectPrefabDrop(node->absPath);

		if(open) {
			// 未スキャンならここでスキャン
			if(!node->scanned) {
				std::error_code ec;
				for(const auto& entry : std::filesystem::directory_iterator(node->absPath, ec)) {
					if(ec) break;
					if(entry.is_directory()) {
						auto child	   = std::make_unique<DirNode>();
						child->name	   = entry.path().filename().string();
						child->absPath = entry.path();
						child->scanned = false; // 子も未スキャン
						node->children.emplace(child->name, std::move(child));
					}
				}
				node->scanned = true;
			}

			// 描画
			for(auto& [_, ch] : node->children) {
				DrawDirNode(ch.get());
			}
			ImGui::TreePop();
		}
	}

	/* ================= フォルダツリー構築 ================= */
	void AssetPanel::EnsureFolderTreeBuilt() {
		if(!needsRebuildTree_ && rootNode_) return;
		RebuildFolderTree();
		needsRebuildTree_ = false;
	}

	void AssetPanel::RebuildFolderTree() {
		rootNode_		   = std::make_unique<DirNode>();
		rootNode_->name	   = assetsRootAbs_.filename().string().empty() ? "Assets" : assetsRootAbs_.filename().string();
		rootNode_->absPath = assetsRootAbs_;
		rootNode_->open	   = true;
		rootNode_->scanned = false; // ルートから遅延ロード
	}

	void AssetPanel::InsertPath(DirNode*, const std::filesystem::path&) {
		// Deprecated / Unused in lazy load mode
	}

	bool AssetPanel::IsUnder(const std::filesystem::path& file, const std::filesystem::path& folder) {
		std::error_code ec;
		auto			rel = std::filesystem::relative(file, folder, ec);
		if(ec) return false;
		if(rel.empty()) return true; // same
		auto s = rel.generic_string();
		return !(s.size() >= 2 && s[0] == '.' && s[1] == '.'); // 先頭 ".." でなければ配下
	}

	std::string AssetPanel::FilenameNoExt(const std::filesystem::path& p) {
		return p.stem().string();
	}

	std::string AssetPanel::NormalizeLower(const std::filesystem::path& p) {
		std::error_code ec;
		auto			canon = std::filesystem::weakly_canonical(p, ec).generic_string();
		if(ec) canon = p.generic_string();
		for(auto& c : canon) c = (char)std::tolower((unsigned char)c);
		return canon;
	}

	bool AssetPanel::IsInFolder(const std::filesystem::path& file, const std::filesystem::path& folder) {
		// 「同一階層のみ」判定（親ディレクトリが一致）
		return NormalizeLower(file.parent_path()) == NormalizeLower(folder);
	}

	void AssetPanel::ListSubdirectories(const std::filesystem::path& folder, std::vector<std::filesystem::path>& out) {
		std::error_code ec;
		for(auto& e : std::filesystem::directory_iterator(folder, ec)) {
			if(ec) break;
			if(e.is_directory(ec)) out.emplace_back(e.path());
		}
		std::sort(out.begin(), out.end(),
				  [](const auto& a, const auto& b) { return a.filename().string() < b.filename().string(); });
	}

} // namespace CalyxEngine
