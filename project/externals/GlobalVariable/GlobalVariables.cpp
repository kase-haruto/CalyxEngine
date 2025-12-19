#include "GlobalVariables.h"
#include<fstream>
#include<Windows.h>
#include <iostream>


GlobalVariables* GlobalVariables::GetInstance() {
	static GlobalVariables instance;
	return &instance;
}

#ifdef _DEBUG
void GlobalVariables::Update() {
	if (!ImGui::Begin("Global Variables", nullptr, ImGuiWindowFlags_MenuBar)) {
		ImGui::End();
		return;
	}
	if (ImGui::BeginMenuBar()) {
		for (auto itGroup = datas_.begin(); itGroup != datas_.end(); ++itGroup) {
			const std::string& groupName = itGroup->first;
			Group& group = itGroup->second;

			if (ImGui::BeginMenu(groupName.c_str())) {
				for (auto itItem = group.items.begin(); itItem != group.items.end(); ++itItem) {
					const std::string& itemName = itItem->first;
					Item& item = itItem->second;

					std::visit([&](auto& value) {
						using T = std::decay_t<decltype(value)>;
						if constexpr (std::is_same_v<T, bool>) {
							CheckBox(itemName, value);
						} else {
							ShowSlider(itemName, value);
						}
					}, item.value);
				}

				ImGui::Text("\n");

				if (ImGui::Button("save")) {
					SaveFile(groupName);
					std::string message = std::format("{}.json saved.", groupName);
					MessageBoxA(nullptr, message.c_str(), "GlobalVariables", 0);
				}

				ImGui::EndMenu();
			}
		}

		ImGui::EndMenuBar();
	}
	ImGui::End();
}
#endif // _DEBUG

void GlobalVariables::Initialize() {
	LoadFiles();
}

void GlobalVariables::CreateGroup(const std::string& groupName) {
	datas_[groupName];
}

void GlobalVariables::SaveFile(const std::string& groupName) {
	// グループを検索
	auto itGroup = datas_.find(groupName);
	if (itGroup == datas_.end()) {
		LogError("Group not found: " + groupName);
		return;
	}

	json root;
	root[groupName] = json::object();

	for (auto itItem = itGroup->second.items.begin(); itItem != itGroup->second.items.end(); ++itItem) {
		const std::string& itemName = itItem->first;
		Item& item = itItem->second;

		if (std::holds_alternative<int32_t>(item.value)) {
			root[groupName][itemName] = std::get<int32_t>(item.value);
		} else if (std::holds_alternative<float>(item.value)) {
			root[groupName][itemName] = std::get<float>(item.value);
		} else if (std::holds_alternative<CalyxMath::Vector3>(item.value)) {
			CalyxMath::Vector3 value = std::get<CalyxMath::Vector3>(item.value);
			root[groupName][itemName] = json::array({ value.x, value.y, value.z });
		} else if (std::holds_alternative<bool>(item.value)) {
			root[groupName][itemName] = std::get<bool>(item.value);
		}
	}

	std::filesystem::path dir(kDirectoryPath);
	if (!std::filesystem::exists(kDirectoryPath)) {
		std::filesystem::create_directory(kDirectoryPath);
	}

	std::string filePath = kDirectoryPath + groupName + ".json";
	std::ofstream ofs(filePath);

	if (ofs.fail()) {
		LogError("Failed to open data file for write: " + filePath);
		return;
	}

	ofs << std::setw(4) << root << std::endl;
	ofs.close();
}


void GlobalVariables::LoadFiles() {
	std::filesystem::path dir(kDirectoryPath);
	if (!std::filesystem::exists(kDirectoryPath)) {
		return;
	}

	std::filesystem::directory_iterator dir_it(kDirectoryPath);
	for (const std::filesystem::directory_entry& entry : dir_it) {
		const std::filesystem::path& filePath = entry.path();
		std::string extension = filePath.extension().string();
		if (extension.compare(".json") != 0) {
			continue;
		}

		LoadFile(filePath.stem().string());
	}
}

void GlobalVariables::LoadFile(const std::string& groupName) {
	std::string filePath = kDirectoryPath + groupName + ".json";
	std::ifstream ifs(filePath);

	if (ifs.fail()) {
		LogError("Failed to open data file for read: " + filePath);
		return;
	}

	json root;
	ifs >> root;
	ifs.close();

	auto itGroup = root.find(groupName);
	if (itGroup == root.end()) {
		LogError("Group not found in JSON: " + groupName);
		return;
	}

	for (auto itItem = itGroup->begin(); itItem != itGroup->end(); ++itItem) {
		const std::string& itemName = itItem.key();

		if (itItem->is_number_integer()) {
			int32_t value = itItem->get<int32_t>();
			SetValue(groupName, itemName, value);
		} else if (itItem->is_number_float()) {
			float value = itItem->get<float>();
			SetValue(groupName, itemName, value);
		} else if (itItem->is_array() && itItem->size() == 3) {
			CalyxMath::Vector3 value = { itItem->at(0), itItem->at(1), itItem->at(2) };
			SetValue(groupName, itemName, value);
		} else if (itItem->is_boolean()) {
			bool value = itItem->get<bool>();
			SetValue(groupName, itemName, value);
		}
	}
}

void GlobalVariables::LogError(const std::string& message) {
	// ログファイルにエラーを書き込む
	std::ofstream logFile("error_log.txt", std::ios_base::app);
	if (logFile.is_open()) {
		logFile << message << std::endl;
		logFile.close();
	}

	// MessageBoxAでエラーメッセージを表示
	MessageBoxA(nullptr, message.c_str(), "Error", MB_OK | MB_ICONERROR);
}

#ifdef _DEBUG
void GlobalVariables::ShowSlider(const std::string& itemName, int32_t& value) {
	ImGui::DragInt(itemName.c_str(), &value, 0.01f);
}

void GlobalVariables::ShowSlider(const std::string& itemName, float& value) {
	ImGui::DragFloat(itemName.c_str(), &value, 0.01f);
}

void GlobalVariables::ShowSlider(const std::string& itemName, CalyxMath::Vector3& value) {
	ImGui::DragFloat3(itemName.c_str(), &value.x, 0.01f);
}

void GlobalVariables::CheckBox(const std::string& itemName, bool& value) {
	ImGui::Checkbox(itemName.c_str(), &value);
}
#endif // _DEBUG
