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

		bool CopyText(std::span<char> buffer, const std::string& text) {
			if(buffer.empty()) return false;
			const size_t length = (std::min)(buffer.size() - 1, text.size());
			std::copy_n(text.data(), length, buffer.data());
			buffer[length] = '\0';
			return length == text.size();
		}

		bool IsBlank(const char* text) {
			if(!text) return true;
			while(*text) {
				if(!std::isspace(static_cast<unsigned char>(*text))) return false;
				++text;
			}
			return true;
		}

	} // namespace

	ProjectBrowser::ProjectBrowser()
		: registryPath_(Calyx::DefaultProjectRegistryPath()) {
		CopyText(newProjectName_, "NewProject");
		CopyText(newProjectDirectory_, DefaultUserProjectDirectory().string());
		ReloadRecentProjects();
	}

	bool ProjectBrowser::Draw(Calyx::ProjectInfo& outProject) {
		bool selected = false;

		const ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->WorkPos);
		ImGui::SetNextWindowSize(viewport->WorkSize);
		ImGui::Begin("Project Browser", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

		LoadIcons();

		ImGui::TextUnformatted("Calyx Project Browser");
		ImGui::Separator();

		if(ImGui::Button("Open Project", ImVec2(140.0f, 28.0f))) {
			IGFD::FileDialogConfig config;
			ImGuiFileDialog::Instance()->OpenDialog("OpenCalyxProject", "Open Calyx Project", ".calyxproj", config);
		}
		ImGui::SameLine();
		if(ImGui::Button("Refresh", ImVec2(100.0f, 28.0f))) {
			ReloadRecentProjects();
		}

		if(!statusMessage_.empty()) {
			ImGui::SameLine();
			ImGui::TextDisabled("%s", statusMessage_.c_str());
		}

		ImGui::Spacing();
		const float footerHeight = 72.0f;
		if(ImGui::BeginTable("ProjectBrowserLayout", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV, ImVec2(0.0f, -footerHeight))) {
			ImGui::TableSetupColumn("Recent", ImGuiTableColumnFlags_WidthStretch, 0.68f);
			ImGui::TableSetupColumn("Details", ImGuiTableColumnFlags_WidthStretch, 0.32f);

			ImGui::TableNextColumn();
			DrawRecentProjects(outProject, selected);

			ImGui::TableNextColumn();
			DrawTemplateDetails();

			ImGui::EndTable();
		}

		ImGui::Separator();
		DrawNewProject(outProject, selected);
		DrawOpenProjectDialog(outProject, selected);
		DrawLocationDialog();

		ImGui::End();
		return selected;
	}

	void ProjectBrowser::ReloadRecentProjects() {
		if(!Calyx::LoadRecentProjects(registryPath_, recentProjects_)) {
			recentProjects_.clear();
			statusMessage_ = "Recent project list could not be loaded.";
			return;
		}
		statusMessage_.clear();
	}

	void ProjectBrowser::DrawRecentProjects(Calyx::ProjectInfo& outProject, bool& selected) {
		ImGui::TextUnformatted("Recent Projects");
		ImGui::Separator();

		if(recentProjects_.empty()) {
			ImGui::TextDisabled("No recent projects.");
		} else {
			ImGui::BeginChild("RecentProjectList", ImVec2(0.0f, 0.0f), false);

			const float cardWidth = 142.0f;
			const float cardHeight = 146.0f;
			const float spacing = ImGui::GetStyle().ItemSpacing.x;
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
				drawList->AddRect(cardMin, cardMax, IM_COL32(70, 70, 70, 255));

				ImGui::SetCursorScreenPos(ImVec2(cardMin.x + 10.0f, cardMin.y + 10.0f));
				if(genericIcon_) {
					ImGui::Image(genericIcon_, ImVec2(cardWidth - 20.0f, 82.0f));
				} else {
					ImGui::Dummy(ImVec2(cardWidth - 20.0f, 82.0f));
				}

				const std::string label = entry.name.empty() ? entry.projectFile.stem().string() : entry.name;
				ImGui::SetCursorScreenPos(ImVec2(cardMin.x + 10.0f, cardMin.y + 100.0f));
				ImGui::TextWrapped("%s", label.c_str());
				ImGui::SetCursorScreenPos(ImVec2(cardMin.x + 10.0f, cardMin.y + 122.0f));
				ImGui::TextDisabled("%s", entry.engineVersion.empty() ? "0.1.0" : entry.engineVersion.c_str());

				ImGui::EndGroup();
				ImGui::PopID();
			}

			ImGui::EndChild();
		}
	}

	void ProjectBrowser::DrawTemplateDetails() {
		ImGui::TextUnformatted("Template");
		ImGui::Separator();

		const float previewWidth = ImGui::GetContentRegionAvail().x;
		const float previewHeight = (std::min)(180.0f, previewWidth * 0.52f);
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
		ImGui::SameLine(120.0f);
		ImGui::TextUnformatted("C++");

		ImGui::TextDisabled("Target");
		ImGui::SameLine(120.0f);
		ImGui::TextUnformatted("Desktop");

		ImGui::TextDisabled("Startup Scene");
		ImGui::SameLine(120.0f);
		ImGui::TextUnformatted("None");
	}

	void ProjectBrowser::DrawNewProject(Calyx::ProjectInfo& outProject, bool& selected) {
		ImGui::BeginGroup();

		ImGui::TextUnformatted("Project Location");
		ImGui::SameLine();
		ImGui::SetNextItemWidth((std::max)(240.0f, ImGui::GetContentRegionAvail().x * 0.54f));
		ImGui::InputText("##ProjectDirectory", newProjectDirectory_.data(), newProjectDirectory_.size());
		ImGui::SameLine();
		if(folderIcon_) {
			if(ImGui::ImageButton("##BrowseProjectLocation", folderIcon_, ImVec2(22.0f, 22.0f))) {
				IGFD::FileDialogConfig config;
				config.path = newProjectDirectory_.data();
				ImGuiFileDialog::Instance()->OpenDialog("SelectProjectDirectory", "Select Project Location", nullptr, config);
			}
		} else if(ImGui::Button("...", ImVec2(30.0f, 24.0f))) {
			IGFD::FileDialogConfig config;
			config.path = newProjectDirectory_.data();
			ImGuiFileDialog::Instance()->OpenDialog("SelectProjectDirectory", "Select Project Location", nullptr, config);
		}

		ImGui::SameLine();
		ImGui::TextUnformatted("Project Name");
		ImGui::SameLine();
		ImGui::SetNextItemWidth((std::max)(160.0f, ImGui::GetContentRegionAvail().x - 180.0f));
		ImGui::InputText("##ProjectName", newProjectName_.data(), newProjectName_.size());

		const float buttonWidth = 104.0f;
		ImGui::SameLine((std::max)(0.0f, ImGui::GetWindowContentRegionMax().x - buttonWidth - 8.0f));
		if(ImGui::Button("Create", ImVec2(buttonWidth, 30.0f))) {
			selected = CreateBlankProject(outProject);
		}

		ImGui::EndGroup();
	}

	void ProjectBrowser::DrawOpenProjectDialog(Calyx::ProjectInfo& outProject, bool& selected) {
		if(ImGuiFileDialog::Instance()->Display("OpenCalyxProject")) {
			if(ImGuiFileDialog::Instance()->IsOk()) {
				selected = LoadProject(ImGuiFileDialog::Instance()->GetFilePathName(), outProject);
			}
			ImGuiFileDialog::Instance()->Close();
		}
	}

	void ProjectBrowser::DrawLocationDialog() {
		if(ImGuiFileDialog::Instance()->Display("SelectProjectDirectory")) {
			if(ImGuiFileDialog::Instance()->IsOk()) {
				CopyText(newProjectDirectory_, ImGuiFileDialog::Instance()->GetCurrentPath());
			}
			ImGuiFileDialog::Instance()->Close();
		}
	}

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
