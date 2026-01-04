#pragma once
#include <Engine/Foundation/Math/Vector3.h>

#ifdef _DEBUG
#include <externals/imgui/imgui.h>
#endif // _DEBUG

#include <externals/nlohmann/json.hpp>
#include <map>
#include <string>
#include <variant>
#include <cstdint>
#include <filesystem>

using json = nlohmann::json;

namespace AdjustmentTarget{
    struct Item{
        std::variant<int32_t, float, Vector3, bool> value;
    };

    struct Group{
        std::map<std::string, Item> items;
    };
}

using AdjustmentTarget::Item;
using AdjustmentTarget::Group;

class GlobalVariables{
public:
    static GlobalVariables* GetInstance();

    void Initialize();
    void CreateGroup(const std::string& groupName);
    void Update();
    void SaveFile(const std::string& groupName);
    void LoadFiles();
    void LoadFile(const std::string& groupName);
    void LogError(const std::string& message);

    template<typename T>
    void SetValue(const std::string& groupName, const std::string& key, const T& value);

    template<typename T>
    void AddItem(const std::string& groupName, const std::string& key, const T& value);

    template<typename T>
    T GetValue(const std::string& groupName, const std::string& key) const;

#ifdef _DEBUG
    void ShowSlider(const std::string& itemName, float& value);
    void ShowSlider(const std::string& itemName, int32_t& value);
    void ShowSlider(const std::string& itemName, Vector3& value);
    void CheckBox(const std::string& itemName, bool& value);
#endif // _DEBUG

private:
    GlobalVariables() = default;
    ~GlobalVariables() = default;
    GlobalVariables(const GlobalVariables&) = delete;
    const GlobalVariables& operator=(const GlobalVariables&) = delete;

private:
    std::map<std::string, Group> datas_;
    const std::string kDirectoryPath = "Resources/GlobalVariables/";
};

///=======================================================================================================
///		テンプレート関数の設定
///=======================================================================================================
template<typename T>
void GlobalVariables::SetValue(const std::string& groupName, const std::string& key, const T& value){
    Group& group = datas_[groupName];
    group.items[key].value = value;
}

template<typename T>
void GlobalVariables::AddItem(const std::string& groupName, const std::string& key, const T& value){
    Group& group = datas_[groupName];
    if (group.items.find(key) == group.items.end()){
        SetValue(groupName, key, value);
    }
}

template<typename T>
T GlobalVariables::GetValue(const std::string& groupName, const std::string& key) const{
    const Group& group = datas_.at(groupName);
    return std::get<T>(group.items.at(key).value);
}
