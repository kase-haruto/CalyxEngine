#include <Engine/Assets/Model/ModelBuilder.h>
#include <Engine/Assets/Model/ModelManager.h>

//c++
#include <fstream>

//external
#include <externals/nlohmann/json.hpp> // JSONライブラリ
#include <Externals/imgui/imgui.h>

using json = nlohmann::json;
ModelBuilder::ModelBuilder(const std::string& name):BaseEditor(name){}
void ModelBuilder::Initialize(){
	LoadModels("Resources/json/models.json");
}

void ModelBuilder::Update(){
	for (auto& pair : models_){
		pair.second.second->Update();
	}
}

void ModelBuilder::Draw([[maybe_unused]] const Matrix4x4& vp){

}

void ModelBuilder::RemoveModel(size_t index){
	if (index < models_.size()){
		auto it = std::next(models_.begin(), index);
		models_.erase(it);
	}
}

void ModelBuilder::ShowImGuiInterface(){

	// === 操作ボタン ===
	ImGui::Text("Operations:");
	ImGui::Separator();

	// モデル追加ボタン
	if (ImGui::Button("Add Model")){
		if (!selectedModelName_.empty()){
			AddModel(selectedModelName_);
		}
	}
	ImGui::SameLine();
	// モデル削除ボタン
	if (ImGui::Button("Remove Selected Model")){
		auto it = models_.find(selectedModelName_);
		if (it != models_.end()){
			models_.erase(it);
			selectedModelName_.clear();
		}
	}

	// 保存ボタン
	if (ImGui::Button("Save Models")){
		SaveModels("Resources/json/models.json");
	}
	ImGui::Separator();

	// === モデル選択 ===
	ImGui::Text("Select Model:");
	const auto& loadedModels = ModelManager::GetInstance()->GetLoadedModelNames();

	if (ImGui::BeginCombo("Loaded Models", selectedModelName_.c_str())){
		for (const auto& modelName : loadedModels){
			bool isSelected = (selectedModelName_ == modelName);
			if (ImGui::Selectable(modelName.c_str(), isSelected)){
				selectedModelName_ = modelName; // 選択されたモデル名を更新
			}
			if (isSelected){
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}

	ImGui::Separator();

	// === モデルリストと詳細ビュー ===
	ImGui::Columns(2, nullptr, true);

	// 左側: モデルリスト
	ImGui::Text("Model List:");
	ImGui::Separator();
	size_t index = 0;
	for (const auto& pair : models_){
		bool isSelected = (selectedModelName_ == pair.first);
		if (ImGui::Selectable(pair.first.c_str(), isSelected)){
			selectedModelName_ = pair.first; // ユニークキーを選択
		}
		index++;
	}
	ImGui::NextColumn();

	// 右側: 選択したモデルの詳細
	ImGui::Text("Model Details:");
	ImGui::Separator();
	if (!selectedModelName_.empty()){
		auto it = models_.find(selectedModelName_);
		if (it != models_.end() && it->second.second){
			ImGui::Text("Selected Model: %s", it->second.first.c_str());
			it->second.second->ShowImGuiInterface(); // 選択されたモデルの詳細UIを表示
		} else{
			ImGui::Text("Model details are unavailable.");
		}
	} else{
		ImGui::Text("No Model Selected");
	}

	ImGui::Columns(1);
}

void ModelBuilder::AddModel(const std::string& modelName){
	static int uniqueId = 0;
	std::string uniqueKey = modelName + "_" + std::to_string(uniqueId++);
	std::optional<ModelData> modelData = ModelManager::GetInstance()->GetModelData(modelName);
	if (!modelData){
		return;
	}
	auto model = CreateModel(modelName);
	models_.emplace(uniqueKey, std::make_pair(modelName, std::move(model)));
}
std::unique_ptr<Model> ModelBuilder::CreateModel(const std::string& modelName){
	auto model = std::make_unique<Model>(modelName);
	return model;
}

/////////////////////////////////////////////////////////////////////////////////////////
//                          json
/////////////////////////////////////////////////////////////////////////////////////////
void ModelBuilder::SaveModels([[maybe_unused]]const std::string& filePath){
	
}
void ModelBuilder::LoadModels([[maybe_unused]] const std::string& filePath){
	
}