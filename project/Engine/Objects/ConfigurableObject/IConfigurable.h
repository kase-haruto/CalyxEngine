#pragma once
#include <externals/nlohmann/json.hpp>

/*-----------------------------------------------------------------------------------------
 * IConfigurable
 * - 設定可能インターフェース
 * - JSON形式での設定の適用/抽出を定義
 *---------------------------------------------------------------------------------------*/
class IConfigurable {
public:
	virtual ~IConfigurable() = default;

	virtual void ApplyConfigFromJson(const nlohmann::json& j) = 0;
	virtual void ExtractConfigToJson(nlohmann::json& j)const = 0;
};