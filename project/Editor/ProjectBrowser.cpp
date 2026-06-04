#include "ProjectBrowser.h"

#include <Engine/Assets/Manager/AssetManager.h>

#include <externals/imgui/ImGuiFileDialog.h>
#include <externals/imgui/imgui.h>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <span>

namespace CalyxEditor {

	namespace {

		// 新規プロジェクトの初期作成先
		std::filesystem::path DefaultUserProjectDirectory() {
			char*  userProfile = nullptr;
			size_t length		= 0;
			if(_dupenv_s(&userProfile, &length, "USERPROFILE") == 0 && userProfile) {
				std::filesystem::path path = std::filesystem::path(userProfile) / "Documents" / "Calyx Projects";
				std::free(userProfile);
				return path;
			}
			return std::filesystem::path("Calyx Projects");
		}

		// std::array<char> への安全な文字列コピー
		bool CopyText(std::span<char> buffer, const std::string& text) {
			if(buffer.empty()) return false;
			const size_t length = (std::min)(buffer.size() - 1, text.size());
			std::copy_n(text.data(), length, buffer.data());
			buffer[length] = '\0';
			return length == text.size();
		}

		// 入力欄の未入力判定
		bool IsBlank(const char* text) {
			if(!text) return true;
			while(*text) {
				if(!std::isspace(static_cast<unsigned char>(*text))) return false;
				++text;
			}
			return true;
		}

	} // namespace

	////////////////////////////////////////////////////////////////////////////////////////////
	//						初期化
	////////////////////////////////////////////////////////////////////////////////////////////
	ProjectBrowser::ProjectBrowser()
		: registryPath_(Calyx::DefaultProjectRegistryPath()) {
		if(!param_.LoadParams()) {
			param_.SaveParams();
		}

		CopyText(newProjectName_, "NewProject");
		CopyText(newProjectDirectory_, DefaultUserProjectDirectory().string());
		ReloadRecentProjects();
	}

	////////////////////////////////////////////////////////////////////////////////////////////
	//						描画
	////////////////////////////////////////////////////////////////////////////////////////////
	bool ProjectBrowser::Draw(Calyx::ProjectInfo& outProject) {
		
		bool selected = false;

		const ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->WorkPos);
		ImGui::SetNextWindowSize(viewport->WorkSize);
		ImGui::Begin("Project Browser", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
		LoadIcons();

		ImGui::TextUnformatted("Calyx Project Browser");
		ImGui::Separator();

		if(ImGui::Button("Open Project", ImVec2(param_.openButtonSize_.x, param_.openButtonSize_.y))) {
			IGFD::FileDialogConfig config;
			ImGuiFileDialog::Instance()->OpenDialog("OpenCalyxProject", "Open Calyx Project", ".calyxproj", config);
		}
		ImGui::SameLine();
		if(ImGui::Button("Refresh", ImVec2(param_.refreshButtonSize_.x, param_.refreshButtonSize_.y))) {
			ReloadRecentProjects();
		}

		if(!statusMessage_.empty()) {
			ImGui::SameLine();
			ImGui::TextDisabled("%s", statusMessage_.c_str());
		}

		ImGui::Spacing();
		ImGuiStyle& style = ImGui::GetStyle();

		// Project Location + Project Name の2行分
		const float footerHeight =
			ImGui::GetFrameHeightWithSpacing() * 2.0f +
			style.ItemSpacing.y * 2.0f +
			style.WindowPadding.y;

		if(ImGui::BeginChild(
			"ProjectBrowserMainArea",
			ImVec2(0.0f, -footerHeight),
			false,
			ImGuiWindowFlags_NoScrollbar)) {

			if(ImGui::BeginTable(
				"ProjectBrowserLayout",
				param_.tableColumnCount_,
				ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV,
				ImVec2(0.0f, 0.0f))) {

				ImGui::TableSetupColumn("Recent", ImGuiTableColumnFlags_WidthStretch, param_.recentColumnWeight_);
				ImGui::TableSetupColumn("Details", ImGuiTableColumnFlags_WidthStretch, param_.detailsColumnWeight_);

				ImGui::TableNextColumn();
				DrawRecentProjects(outProject, selected);

				ImGui::TableNextColumn();
				DrawTemplateDetails();

				ImGui::EndTable();
				}
			}
		ImGui::EndChild();

		ImGui::Separator();
		DrawNewProject(outProject, selected);
		DrawOpenProjectDialog(outProject, selected);
		DrawLocationDialog();

		ImGui::End();
		return selected;
	}

	////////////////////////////////////////////////////////////////////////////////////////////
	//						最近使ったプロジェクトの再読み込み
	////////////////////////////////////////////////////////////////////////////////////////////
	void ProjectBrowser::ReloadRecentProjects() {
		if(!Calyx::LoadRecentProjects(registryPath_, recentProjects_)) {
			recentProjects_.clear();
			statusMessage_ = "Recent project list could not be loaded.";
			return;
		}
		statusMessage_.clear();
	}

	////////////////////////////////////////////////////////////////////////////////////////////
	//						最近使ったプロジェクト一覧
	////////////////////////////////////////////////////////////////////////////////////////////
	void ProjectBrowser::DrawRecentProjects(Calyx::ProjectInfo& outProject, bool& selected) {
		ImGui::TextUnformatted("Recent Projects");
		ImGui::Separator();

		if(recentProjects_.empty()) {
			ImGui::TextDisabled("No recent projects.");
		} else {
			ImGui::BeginChild("RecentProjectList", ImVec2(0.0f, 0.0f), false);

			const float cardWidth  = param_.cardSize_.x;
			const float cardHeight = param_.cardSize_.y;
			const float spacing	= ImGui::GetStyle().ItemSpacing.x;
			const float availableWidth = ImGui::GetContentRegionAvail().x;
			int columns = static_cast<int>(availableWidth / (cardWidth + spacing));
			columns = (std::max)(1, columns);

			for(size_t i = 0; i < recentProjects_.size(); ++i) {
				const auto& entry = recentProjects_[i];
				ImGui::PushID(static_cast<int>(i));

				if(i > 0 && static_cast<int>(i % columns) != 0) {
					ImGui::SameLine();
				}

				ImGui::BeginGroup();
				if(ImGui::Selectable("##recent-card", false, 0, ImVec2(cardWidth, cardHeight))) {
					selected = LoadProject(entry.projectFile, outProject);
				}
				const ImVec2 cardMin = ImGui::GetItemRectMin();
				const ImVec2 cardMax = ImGui::GetItemRectMax();
				auto* drawList = ImGui::GetWindowDrawList();
				drawList->AddRect(
					cardMin,
					cardMax,
					ImGui::ColorConvertFloat4ToU32(
						ImVec4(
							param_.cardBorderColor_.x,
							param_.cardBorderColor_.y,
							param_.cardBorderColor_.z,
							param_.cardBorderColor_.w)));

				ImGui::SetCursorScreenPos(ImVec2(cardMin.x + param_.cardPadding_, cardMin.y + param_.cardPadding_));
				if(genericIcon_) {
					ImGui::Image(genericIcon_, ImVec2(cardWidth - param_.cardImageWidthOffset_, param_.cardImageHeight_));
				} else {
					ImGui::Dummy(ImVec2(cardWidth - param_.cardImageWidthOffset_, param_.cardImageHeight_));
				}

				const std::string label = entry.name.empty() ? entry.projectFile.stem().string() : entry.name;
				ImGui::SetCursorScreenPos(ImVec2(cardMin.x + param_.cardPadding_, cardMin.y + param_.cardNameOffsetY_));
				ImGui::TextWrapped("%s", label.c_str());
				ImGui::SetCursorScreenPos(ImVec2(cardMin.x + param_.cardPadding_, cardMin.y + param_.cardVersionOffsetY_));
				ImGui::TextDisabled("%s", entry.engineVersion.empty() ? "0.1.0" : entry.engineVersion.c_str());

				ImGui::EndGroup();
				ImGui::PopID();
			}

			ImGui::EndChild();
		}
	}

	////////////////////////////////////////////////////////////////////////////////////////////
	//						テンプレート詳細
	////////////////////////////////////////////////////////////////////////////////////////////
	void ProjectBrowser::DrawTemplateDetails() {
		ImGui::TextUnformatted("Template");
		ImGui::Separator();

		const float previewWidth = ImGui::GetContentRegionAvail().x;
		const float previewHeight = (std::min)(param_.templatePreviewMaxHeight_, previewWidth * param_.templatePreviewAspect_);
		if(genericIcon_) {
			ImGui::Image(genericIcon_, ImVec2(previewWidth, previewHeight));
		} else {
			ImGui::Dummy(ImVec2(previewWidth, previewHeight));
		}

		ImGui::Spacing();
		ImGui::TextUnformatted("Blank");
		ImGui::TextWrapped("An empty game project. No gameplay code is generated yet.");

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::TextDisabled("Language");
		ImGui::SameLine(param_.templateValueOffsetX_);
		ImGui::TextUnformatted("C++");

		ImGui::TextDisabled("Target");
		ImGui::SameLine(param_.templateValueOffsetX_);
		ImGui::TextUnformatted("Desktop");

		ImGui::TextDisabled("Startup Scene");
		ImGui::SameLine(param_.templateValueOffsetX_);
		ImGui::TextUnformatted("None");
	}

	////////////////////////////////////////////////////////////////////////////////////////////
	//						新規プロジェクト作成欄
	////////////////////////////////////////////////////////////////////////////////////////////
	void ProjectBrowser::DrawNewProject(Calyx::ProjectInfo& outProject, bool& selected) {
		ImGui::BeginGroup();

		const float labelWidth	= param_.labelWidth_;
		const float buttonWidth	= param_.buttonWidth_;
		const float browseSize	= param_.browseSize_;
		const float fullWidth	= ImGui::GetContentRegionAvail().x;
		const float inputWidth	= (std::max)(param_.minInputWidth_, fullWidth - labelWidth - browseSize - ImGui::GetStyle().ItemSpacing.x * param_.inputSpacingCount_);

		ImGui::TextUnformatted("Project Location");
		ImGui::SameLine(labelWidth);
		ImGui::SetNextItemWidth(inputWidth);
		ImGui::InputText("##ProjectDirectory", newProjectDirectory_.data(), newProjectDirectory_.size());
		ImGui::SameLine();
		if(folderIcon_) {
			if(ImGui::ImageButton("##BrowseProjectLocation", folderIcon_, ImVec2(param_.folderIconSize_, param_.folderIconSize_))) {
				IGFD::FileDialogConfig config;
				config.path = newProjectDirectory_.data();
				ImGuiFileDialog::Instance()->OpenDialog("SelectProjectDirectory", "Select Project Location", nullptr, config);
			}
		} else if(ImGui::Button("...", ImVec2(param_.fallbackBrowseButtonSize_.x, param_.fallbackBrowseButtonSize_.y))) {
			IGFD::FileDialogConfig config;
			config.path = newProjectDirectory_.data();
			ImGuiFileDialog::Instance()->OpenDialog("SelectProjectDirectory", "Select Project Location", nullptr, config);
		}

		ImGui::TextUnformatted("Project Name");
		ImGui::SameLine(labelWidth);
		ImGui::SetNextItemWidth((std::max)(param_.minInputWidth_, fullWidth - labelWidth - buttonWidth - ImGui::GetStyle().ItemSpacing.x * param_.inputSpacingCount_));
		ImGui::InputText("##ProjectName", newProjectName_.data(), newProjectName_.size());

		ImGui::SameLine();
		if(ImGui::Button("Create", ImVec2(buttonWidth, param_.createButtonHeight_))) {
			selected = CreateBlankProject(outProject);
		}

		ImGui::EndGroup();
	}

	////////////////////////////////////////////////////////////////////////////////////////////
	//						既存プロジェクトを開くダイアログ
	////////////////////////////////////////////////////////////////////////////////////////////
	void ProjectBrowser::DrawOpenProjectDialog(Calyx::ProjectInfo& outProject, bool& selected) {
		if(ImGuiFileDialog::Instance()->Display("OpenCalyxProject")) {
			if(ImGuiFileDialog::Instance()->IsOk()) {
				selected = LoadProject(ImGuiFileDialog::Instance()->GetFilePathName(), outProject);
			}
			ImGuiFileDialog::Instance()->Close();
		}
	}

	////////////////////////////////////////////////////////////////////////////////////////////
	//						プロジェクト作成先フォルダの選択
	////////////////////////////////////////////////////////////////////////////////////////////
	void ProjectBrowser::DrawLocationDialog() {
		if(ImGuiFileDialog::Instance()->Display("SelectProjectDirectory")) {
			if(ImGuiFileDialog::Instance()->IsOk()) {
				CopyText(newProjectDirectory_, ImGuiFileDialog::Instance()->GetCurrentPath());
			}
			ImGuiFileDialog::Instance()->Close();
		}
	}

	////////////////////////////////////////////////////////////////////////////////////////////
	//						アイコン読み込み
	////////////////////////////////////////////////////////////////////////////////////////////
	void ProjectBrowser::LoadIcons() {
		if(genericIcon_ && folderIcon_) {
			return;
		}

		auto* assetManager = CalyxEngine::AssetManager::GetInstance();
		if(!assetManager || !assetManager->GetTextureManager()) {
			return;
		}

		auto* textureManager = assetManager->GetTextureManager();
		if(!genericIcon_) {
			genericIcon_ = reinterpret_cast<void*>(textureManager->LoadTexture("UI/Tool/AssetPanel/generic.png").ptr);
		}
		if(!folderIcon_) {
			folderIcon_ = reinterpret_cast<void*>(textureManager->LoadTexture("UI/Tool/AssetPanel/folder.png").ptr);
		}
	}

	////////////////////////////////////////////////////////////////////////////////////////////
	//						プロジェクト読み込み
	////////////////////////////////////////////////////////////////////////////////////////////
	bool ProjectBrowser::LoadProject(const std::filesystem::path& path, Calyx::ProjectInfo& outProject) {
		Calyx::ProjectInfo project;
		if(!Calyx::LoadProjectFile(path, project)) {
			statusMessage_ = "Project could not be opened.";
			return false;
		}

		outProject = std::move(project);
		statusMessage_.clear();
		return true;
	}

	////////////////////////////////////////////////////////////////////////////////////////////
	//						Blankプロジェクト作成
	////////////////////////////////////////////////////////////////////////////////////////////
	bool ProjectBrowser::CreateBlankProject(Calyx::ProjectInfo& outProject) {
		if(IsBlank(newProjectName_.data()) || IsBlank(newProjectDirectory_.data())) {
			statusMessage_ = "Project name and directory are required.";
			return false;
		}

		Calyx::ProjectInfo project;
		project.name			 = newProjectName_.data();
		project.engineVersion	 = "0.1.0";
		project.rootDirectory	 = std::filesystem::path(newProjectDirectory_.data()) / project.name;
		project.projectFile		 = project.rootDirectory / (project.name + ".calyxproj");
		project.assetDirectory	 = "Resources/Assets";
		project.sourceDirectory	 = "Game";

		if(!Calyx::CreateProject(project)) {
			statusMessage_ = "Project could not be created.";
			return false;
		}

		return LoadProject(project.projectFile, outProject);
	}

} // namespace CalyxEditor
