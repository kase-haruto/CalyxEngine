#include "AssetPanel.h"
#include <Engine/Assets/Database/AssetDatabase.h>

#include <externals/imgui/imgui.h>
#include <externals/imgui/ImGuiFileDialog.h>

#include <algorithm>
#include <vector>
#include <system_error>
#include <cmath>

namespace CalyxEditor {

	void AssetPanel::Initialize(const std::filesystem::path& assetsRoot) {
		assetsRootAbs_	  = std::filesystem::weakly_canonical(assetsRoot);
		currentFolderAbs_ = assetsRootAbs_;

		// アイコン（存在しなければ任意の代替に差し替え）
		auto& tm	 = *TextureManager::GetInstance();
		iconFolder_	 = (ImTextureID)tm.LoadTexture("UI/Tool/AssetPanel/folder.png").ptr;
		iconGeneric_ = (ImTextureID)tm.LoadTexture("UI/Tool/AssetPanel/generic.png").ptr;

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

		if(ImGui::TreeNodeEx("Assets", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanFullWidth)) {
			DrawDirNode(rootNode_.get());
			ImGui::TreePop();
		}
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
			ImGui::TreePop();
		}
	}

	void AssetPanel::DrawDirNode(DirNode* node) {
		if(!node) return;

		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow |
								   ImGuiTreeNodeFlags_SpanFullWidth;
		if(node->children.empty()) flags |= ImGuiTreeNodeFlags_Leaf;

		bool open = ImGui::TreeNodeEx(node, flags, "%s", node->name.c_str());

		// クリックでフォルダ選択
		if(ImGui::IsItemClicked()) {
			currentFolderAbs_ = node->absPath;
			scope_			  = Scope::SelectedFolder;
			typeFilter_.reset();
		}

		if(open) {
			for(auto& [_, ch] : node->children) {
				DrawDirNode(ch.get());
			}
			ImGui::TreePop();
		}
	}

	/* ===================== 右ペイン ===================== */
	static inline void toLowerInplace(std::string& s) {
		std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return (char)std::tolower(c); });
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

			auto isLikelyNon2D = [](const std::filesystem::path& p) {
				std::string ext = p.extension().string();
				for(auto& c : ext) c = (char)std::tolower((unsigned char)c);
				return (ext == ".dds");
			};
			(void)isLikelyNon2D; // 必要なら draw 側で再利用

			for(auto* rec : items) {
				if(!rec) continue;

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
		auto isLikelyNon2D = [](const std::filesystem::path& p) {
			std::string ext = p.extension().string();
			for(auto& c : ext) c = (char)std::tolower((unsigned char)c);
			return (ext == ".dds");
		};

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
						((rec->type == AssetType::Texture) && !isLikelyNon2D(rec->sourcePath) && rec->previewTex)
							? rec->previewTex
							: (iconGeneric_ ? iconGeneric_ : nullptr);

					if(thumb)
						ImGui::ImageButton("##thumb", thumb, ImVec2(20, 20));
					else
						ImGui::Button("No Preview", ImVec2(20, 20));

					if(ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
						AssetDragPayload payload{rec->type, rec->guid};
						ImGui::SetDragDropPayload("CALYX_ASSET", &payload, sizeof(payload));
						ImGui::TextUnformatted(rec->sourcePath.filename().string().c_str());
						ImGui::EndDragDropSource();
					}

					ImGui::SameLine();
					ImGui::TextUnformatted(rec->sourcePath.filename().string().c_str());

					ImGui::EndGroup();
					ImGui::PopID();
				}
			}
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
				const bool	non2D = (rec->type == AssetType::Texture) && isLikelyNon2D(rec->sourcePath);
				ImTextureID thumb = (!non2D && rec->previewTex) ? rec->previewTex
																: (iconGeneric_ ? iconGeneric_ : nullptr);

				if(thumb)
					ImGui::ImageButton("##thumb", thumb, sz);
				else
					ImGui::Button("No Preview", sz);

				if(ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
					AssetDragPayload payload{rec->type, rec->guid};
					ImGui::SetDragDropPayload("CALYX_ASSET", &payload, sizeof(payload));
					ImGui::TextUnformatted(rec->sourcePath.filename().string().c_str());
					ImGui::EndDragDropSource();
				}

				ImGui::TextWrapped("%s", rec->sourcePath.filename().string().c_str());

				ImGui::EndGroup();
				ImGui::PopID();

				ImGui::NextColumn();
				if(++col == columns) col = 0;
			}

			// 行末の列埋め（次行の先頭に戻す）
			while(col++ && col <= columns) ImGui::NextColumn();
		}

		ImGui::Columns(1);
	}

	/* ================= Inspector 側でも使える共通ドロップターゲット ================= */
	bool AssetPanel::DrawAssetDropTarget(AssetType expect, Guid* inoutGuid, float height) {
		bool changed = false;

		ImGui::PushID(inoutGuid);
		ImGui::BeginGroup();

		ImVec2 avail = ImGui::GetContentRegionAvail();
		ImGui::Dummy(ImVec2(avail.x, height));
		bool hovered = ImGui::IsItemHovered();

		if(hovered) {
			ImGui::GetWindowDrawList()->AddRect(
				ImGui::GetItemRectMin(), ImGui::GetItemRectMax(),
				IM_COL32(180, 180, 255, 255), 6.0f, 0, 2.0f);
		}

		if(ImGui::BeginDragDropTarget()) {
			if(auto p = ImGui::AcceptDragDropPayload("CALYX_ASSET")) {
				auto payload = *reinterpret_cast<const AssetDragPayload*>(p->Data);
				if(payload.type == expect) {
					*inoutGuid = payload.guid;
					changed	   = true;
				}
			}
			ImGui::EndDragDropTarget();
		}

		ImGui::EndGroup();
		ImGui::PopID();
		return changed;
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

		// 実ディレクトリから構築（空フォルダも拾う）
		std::error_code ec;
		for(auto it = std::filesystem::recursive_directory_iterator(assetsRootAbs_, ec);
			it != std::filesystem::recursive_directory_iterator(); ++it) {
			if(ec) break;
			const auto& e = *it;
			if(e.is_directory(ec)) {
				InsertPath(rootNode_.get(), e.path());
			}
		}
	}

	void AssetPanel::InsertPath(DirNode* root, const std::filesystem::path& absDir) {
		// assetsRootAbs_ からの相対に分解して一段ずつ挿入
		std::error_code ec;
		auto			rel = std::filesystem::relative(absDir, assetsRootAbs_, ec);
		if(ec) return;

		DirNode*			  cur = root;
		std::filesystem::path acc = assetsRootAbs_;
		for(auto& part : rel) {
			std::string key = part.string();
			acc /= part;
			auto it = cur->children.find(key);
			if(it == cur->children.end()) {
				auto node	  = std::make_unique<DirNode>();
				node->name	  = key;
				node->absPath = acc;
				auto* raw	  = node.get();
				cur->children.emplace(key, std::move(node));
				cur = raw;
			} else {
				cur = it->second.get();
			}
		}
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

} // namespace CalyxEditor