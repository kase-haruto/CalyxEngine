#pragma once
#include <Engine/Objects/3D/Actor/SceneObject.h>
#include <Engine/Foundation/Json/JsonUtils.h>


template<typename TConfig, class TBase = SceneObject>
class ConfigurableObject
	: public TBase{
protected:

	//* apply/extract ========================================//
	void ApplyConfigFromJson(const nlohmann::json& j);
	void ExtractConfigToJson(nlohmann::json& j) const;

	//* save/load ============================================//
	void LoadConfig(const std::string& path);
	void SaveConfig(const std::string& path)const;
	virtual void ApplyConfig() = 0;
protected:
	TConfig config_;
};

/////////////////////////////////////////////////////////////////////////////////////////
//      jsonからコンフィグを適用
/////////////////////////////////////////////////////////////////////////////////////////
template<typename TConfig, class TBase>
inline void ConfigurableObject<TConfig, TBase>::ApplyConfigFromJson(const nlohmann::json& j){
	config_ = j.get<TConfig>();
	ApplyConfig();
}

/////////////////////////////////////////////////////////////////////////////////////////
//      コンフィグをjsonに変換
/////////////////////////////////////////////////////////////////////////////////////////
template<typename TConfig,class TBase>
inline void ConfigurableObject<TConfig, TBase>::ExtractConfigToJson(nlohmann::json& j) const{
	j = config_;
}

/////////////////////////////////////////////////////////////////////////////////////////
//      コンフィグのロード
/////////////////////////////////////////////////////////////////////////////////////////
template<typename TConfig, class TBase>
inline void ConfigurableObject<TConfig, TBase>::LoadConfig(const std::string& path){
	nlohmann::json j;
	if (JsonUtils::Load(path, j)) ApplyConfigFromJson(j);
}

/////////////////////////////////////////////////////////////////////////////////////////
//      コンフィグのセーブ
/////////////////////////////////////////////////////////////////////////////////////////
template<typename TConfig, class TBase>
inline void ConfigurableObject<TConfig, TBase>::SaveConfig(const std::string& path) const{
	nlohmann::json j;
	ExtractConfigToJson(j);
	JsonUtils::Save(path, j);
}
