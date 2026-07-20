#pragma once

#include <externals/nlohmann/json.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

using SortingLayerId = uint16_t;

constexpr SortingLayerId kDefaultSortingLayerId = 0;
constexpr uint32_t kMaxSortingLayerCount = 256;

struct SortingLayer {
	SortingLayerId id = kDefaultSortingLayerId;
	std::string name;
	int32_t order = 0;
};

class SortingLayerSettings {
public:
	SortingLayerSettings();

	static SortingLayerSettings* GetInstance();
	static void SetActiveSettings(SortingLayerSettings* settings);

	bool AddLayer(const std::string& name, SortingLayerId* outLayerId = nullptr);
	bool RemoveLayer(SortingLayerId layerId);
	bool RenameLayer(SortingLayerId layerId, const std::string& newName);
	bool MoveLayer(SortingLayerId layerId, int direction);

	const std::vector<SortingLayer>& GetLayers() const { return layers_; }
	const SortingLayer* FindLayer(SortingLayerId layerId) const;
	std::optional<SortingLayerId> FindLayerId(std::string_view name) const;
	bool IsValidLayerId(SortingLayerId layerId) const;
	int32_t GetLayerOrder(SortingLayerId layerId) const;

	nlohmann::json ToJson() const;
	void ApplyJson(const nlohmann::json& json);
	void ResetToDefault();

private:
	bool IsLayerNameAvailable(const std::string& name, SortingLayerId ignoredLayerId) const;
	void RebuildOrders();

	static SortingLayerSettings* activeSettings_;
	std::vector<SortingLayer> layers_;
};
