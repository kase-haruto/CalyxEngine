#pragma once
#include <Engine/Foundation/Json/JsonUtils.h>
#include <Engine/Objects/3D/Actor/SceneObject.h>
#include <Engine/Objects/ConfigurableObject/IConfigurable.h>

#if defined(_DEBUG) || defined(DEVELOP)
#include <externals/imgui/ImGuiFileDialog.h>
#include <externals/imgui/imgui.h>
#endif // _DEBUG

#include <functional>

/*-----------------------------------------------------------------------------------------
 * ConfigurableObject
 * - 設定可能オブジェクトテンプレートクラス
 * - JSON形式での設定の保存/読込とImGuiによる編集機能を提供
 *---------------------------------------------------------------------------------------*/
template <typename TConfig>
class ConfigurableObject
	: public IConfigurable {
public:
	/*------------- JSON ⇄ Config --------------*/
	void ApplyConfigFromJson(const nlohmann::json& j) override;
	void ExtractConfigToJson(nlohmann::json& j) const override;

	/*------------- ファイル入出力 --------------*/
	void LoadConfig(std::string path);
	void SaveConfig(const std::string& path) const;

	void SetOnApplied(std::function<void(const TConfig&)> cb) { onApplied_ = std::move(cb); }
	void SetOnExtracted(std::function<void(const TConfig&)> cb) { onExtract_ = std::move(cb); }

	/*------------- ImGui GUI --------------*/
	void ShowGui(const std::string& path = "", const std::string& label = "");

	/*------------- アクセサ --------------*/
	TConfig&	   GetConfig() { return config_; }
	const TConfig& GetConfig() const { return config_; }

protected:
	virtual void OnApplyConfig() {}
	virtual void OnExtractConfig() {}

private:
	std::function<void(const TConfig&)> onApplied_;
	std::function<void(const TConfig&)> onExtract_;

	TConfig config_;
};

/////////////////////////////////////////////////////////////////////////////////////////
//      jsonからコンフィグを適用
/////////////////////////////////////////////////////////////////////////////////////////
template <typename TConfig>
inline void ConfigurableObject<TConfig>::ApplyConfigFromJson(const nlohmann::json& j) {
	config_ = j.get<TConfig>();
	OnApplyConfig();

	if(onApplied_) onApplied_(config_);
}

/////////////////////////////////////////////////////////////////////////////////////////
//      コンフィグをjsonに変換
/////////////////////////////////////////////////////////////////////////////////////////
template <typename TConfig>
inline void ConfigurableObject<TConfig>::ExtractConfigToJson(nlohmann::json& j) const {
	const_cast<ConfigurableObject*>(this)->OnExtractConfig(); // 状態→config_
	if(onExtract_) onExtract_(config_);
	j = config_;
}

/////////////////////////////////////////////////////////////////////////////////////////
//      コンフィグのロード
/////////////////////////////////////////////////////////////////////////////////////////
template <typename TConfig>
inline void ConfigurableObject<TConfig>::LoadConfig(std::string categoryAndName) {
	// --- 正規化 ---
	std::replace(categoryAndName.begin(), categoryAndName.end(), '\\', '/');

	// ルート接頭辞の除去（絶対/相対どちらでもOKに）
	const std::string prefix = "Resources/Assets/Configs/";
	if(size_t pos = categoryAndName.find(prefix); pos != std::string::npos) {
		categoryAndName = categoryAndName.substr(pos + prefix.size());
	}

	// 拡張子を除去
	if(size_t extPos = categoryAndName.rfind(".json"); extPos != std::string::npos) {
		categoryAndName = categoryAndName.substr(0, extPos);
	}

	// "Category/Name" 形式チェック
	const size_t slashPos = categoryAndName.find_last_of('/');
	if(slashPos == std::string::npos) {
		throw std::runtime_error("Invalid category/name format. Expected \"Category/Name\"");
	}

	const std::string category = categoryAndName.substr(0, slashPos);
	const std::string name	   = categoryAndName.substr(slashPos + 1);

	// "(1)" などを除いたベース名（テンプレと個別の二層構成を想定）
	std::string baseName = name;
	if(size_t paren = baseName.find('('); paren != std::string::npos) {
		baseName = baseName.substr(0, paren);
		// 末尾空白をトリム
		while(!baseName.empty() && std::isspace(static_cast<unsigned char>(baseName.back()))) {
			baseName.pop_back();
		}
	}

	// パス構築
	const std::string basePath	   = prefix + category + "/" + baseName + ".json";
	const std::string instancePath = prefix + category + "/" + name + ".json";

	// --- 読み込み（存在する方だけ）---
	nlohmann::json jBase, jInst, jMerged;

	const bool hasBase = CalyxEngine::JsonUtils::Load(basePath, jBase);
	const bool hasInst = CalyxEngine::JsonUtils::Load(instancePath, jInst);

	if(!hasBase && !hasInst) {
		// 何も無ければ何もしない（現在値を保持）
		return;
	}

	// --- マージ（上書き優先：個別 > ベース）---
	if(hasBase) jMerged = jBase;
	if(hasInst) {
		if(jMerged.is_null()) {
			jMerged = jInst;
		} else {
			jMerged.update(jInst, /*merge_objects=*/true);
		}
	}

	ApplyConfigFromJson(jMerged);
}

/////////////////////////////////////////////////////////////////////////////////////////
//      コンフィグのセーブ 個別設定を保存
/////////////////////////////////////////////////////////////////////////////////////////
template <typename TConfig>
inline void ConfigurableObject<TConfig>::SaveConfig(const std::string& categoryAndName) const {
	const auto slashPos = categoryAndName.find_last_of('/');
	if(slashPos == std::string::npos) {
		throw std::runtime_error("Invalid category/name format. Expected \"Category/Name\"");
	}

	const std::string category = categoryAndName.substr(0, slashPos);
	const std::string name	   = categoryAndName.substr(slashPos + 1);

	const std::string configPath = "Resources/Assets/Configs/" + category + "/" + name + ".json";

	// Extract処理
	const_cast<ConfigurableObject*>(this)->OnExtractConfig();
	if(onExtract_) onExtract_(config_); // 一貫性のため追加

	// 保存
	CalyxEngine::JsonUtils::Save(configPath, config_);
}
/////////////////////////////////////////////////////////////////////////////////////////
//      コンフィグのgui
/////////////////////////////////////////////////////////////////////////////////////////
template <typename TConfig>
void ConfigurableObject<TConfig>::ShowGui([[maybe_unused]] const std::string& path, [[maybe_unused]] const std::string& label) {
#if defined(_DEBUG) || defined(DEVELOP)
	const std::string loadDlg = "ConfigLoadDialog##" + label;
	const std::string baseDir = "Resources/Assets/Configs/";

	// --- ロード ---
	if(ImGui::Button(("Load##" + label).c_str())) {
		IGFD::FileDialogConfig cfg;
		cfg.path = baseDir;
		ImGuiFileDialog::Instance()->OpenDialog(loadDlg, "Load Config", ".json", cfg);
	}

	ImGui::SameLine();

	// --- セーブ ---
	if(ImGui::Button(("Save##" + label).c_str())) {
		std::filesystem::path dirPath = std::filesystem::path(baseDir + path).parent_path();
		if(!std::filesystem::exists(dirPath)) {
			std::filesystem::create_directories(dirPath);
		}
		SaveConfig(path);
	}

	ImGui::Text("Config Path: %s", (baseDir + path).c_str());

	// --- ダイアログ処理 ---
	if(ImGuiFileDialog::Instance()->Display(loadDlg)) {
		if(ImGuiFileDialog::Instance()->IsOk()) {
			std::string selectedPath = ImGuiFileDialog::Instance()->GetFilePathName();

			// 相対化して category/name に変換
			const std::string base = "Resources/Assets/Configs/";
			size_t			  pos  = selectedPath.find(base);
			if(pos != std::string::npos) {
				std::string			  relative = selectedPath.substr(pos + base.size());
				std::filesystem::path relPath(relative);
				relative = relPath.replace_extension("").string();
				LoadConfig(relative);
			} else {
				LoadConfig(selectedPath); // fallback
			}
		}
		ImGuiFileDialog::Instance()->Close();
	}
#endif
}