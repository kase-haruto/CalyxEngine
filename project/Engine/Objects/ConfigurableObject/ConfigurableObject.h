#pragma once
#include <Engine/Objects/3D/Actor/SceneObject.h>
#include <Engine/Objects/ConfigurableObject/IConfigurable.h>
#include <Engine/Foundation/Json/JsonUtils.h>

#if defined(_DEBUG) || defined(DEVELOP)
#include <externals/imgui/imgui.h>
#include <externals/imgui/ImGuiFileDialog.h>
#endif // _DEBUG

template<typename TConfig>
class ConfigurableObject 
	: public IConfigurable{
public:
	/*------------- JSON ⇄ Config --------------*/
	void ApplyConfigFromJson(const nlohmann::json& j) override;
	void ExtractConfigToJson(nlohmann::json& j) const override;

	/*------------- ファイル入出力 --------------*/
	void LoadConfig(const std::string& path);
	void SaveConfig(const std::string& path) const;

	/*------------- ImGui GUI --------------*/
	void ShowGui(const std::string& label = "");

	/*------------- アクセサ --------------*/
	TConfig& GetConfig(){ return config_; }
	const TConfig& GetConfig() const{ return config_; }

protected:
	virtual void OnApplyConfig(){}
	virtual void OnExtractConfig(){}

private:
	TConfig config_;
};

/////////////////////////////////////////////////////////////////////////////////////////
//      jsonからコンフィグを適用
/////////////////////////////////////////////////////////////////////////////////////////
template<typename TConfig>
inline void ConfigurableObject<TConfig>::ApplyConfigFromJson(const nlohmann::json& j){
	config_ = j.get<TConfig>();
	OnApplyConfig();
}

/////////////////////////////////////////////////////////////////////////////////////////
//      コンフィグをjsonに変換
/////////////////////////////////////////////////////////////////////////////////////////
template<typename TConfig>
inline void ConfigurableObject<TConfig>::ExtractConfigToJson(nlohmann::json& j) const{
	const_cast< ConfigurableObject* >(this)->OnExtractConfig(); // 状態→config_
	j = config_;
}

/////////////////////////////////////////////////////////////////////////////////////////
//      コンフィグのロード
/////////////////////////////////////////////////////////////////////////////////////////
template<typename TConfig>
inline void ConfigurableObject<TConfig>::LoadConfig(const std::string& path){
	nlohmann::json j;
	if (JsonUtils::Load(path, j)) ApplyConfigFromJson(j);
}

/////////////////////////////////////////////////////////////////////////////////////////
//      コンフィグのセーブ
/////////////////////////////////////////////////////////////////////////////////////////
template<typename TConfig>
inline void ConfigurableObject<TConfig>::SaveConfig(const std::string& path) const{
	const_cast< ConfigurableObject* >(this)->OnExtractConfig();
	JsonUtils::Save(path, config_);
}

/////////////////////////////////////////////////////////////////////////////////////////
//      コンフィグのgui
/////////////////////////////////////////////////////////////////////////////////////////
template<typename TConfig>
inline void ConfigurableObject<TConfig>::ShowGui([[maybe_unused]]const std::string& label){
#if defined(_DEBUG) || defined(DEVELOP)
	const std::string loadDlg = "ConfigLoadDialog##" + label;
	const std::string saveDlg = "ConfigSaveDialog##" + label;

	if (ImGui::Button(("Load##" + label).c_str())){
		IGFD::FileDialogConfig cfg;  cfg.path = "Resources/Assets/Configs/";
		ImGuiFileDialog::Instance()->OpenDialog(loadDlg, "Load Config", ".json", cfg);
	}
	ImGui::SameLine();
	if (ImGui::Button(("Save##" + label).c_str())){
		IGFD::FileDialogConfig cfg;  cfg.path = "Resources/Assets/Configs/";
		ImGuiFileDialog::Instance()->OpenDialog(saveDlg, "Save Config", ".json", cfg);
	}

	if (ImGuiFileDialog::Instance()->Display(loadDlg)){
		if (ImGuiFileDialog::Instance()->IsOk())
			LoadConfig(ImGuiFileDialog::Instance()->GetFilePathName());
		ImGuiFileDialog::Instance()->Close();
	}
	if (ImGuiFileDialog::Instance()->Display(saveDlg)){
		if (ImGuiFileDialog::Instance()->IsOk())
			SaveConfig(ImGuiFileDialog::Instance()->GetFilePathName());
		ImGuiFileDialog::Instance()->Close();
	}
#endif
}