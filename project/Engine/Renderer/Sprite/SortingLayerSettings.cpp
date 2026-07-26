#include "SortingLayerSettings.h"

#include <algorithm>
#include <limits>

SortingLayerSettings* SortingLayerSettings::activeSettings_ = nullptr;

SortingLayerSettings::SortingLayerSettings() {
	ResetToDefault();
}

SortingLayerSettings* SortingLayerSettings::GetInstance() {
	// Scene固有設定が未登録の期間はFallbackを返し、Editor起動直後の参照を安全にする。
	static SortingLayerSettings fallback;
	return activeSettings_ ? activeSettings_ : &fallback;
}

void SortingLayerSettings::SetActiveSettings(SortingLayerSettings* settings) {
	// 所有権はScene側に残し、現在SceneのLayer設定だけを非所有参照として切り替える。
	activeSettings_ = settings;
}

void SortingLayerSettings::ResetToDefault() {
	layers_.clear();
	layers_.push_back({kDefaultSortingLayerId, "Default", 0});
}

bool SortingLayerSettings::AddLayer(const std::string& name, SortingLayerId* outLayerId) {
	if(layers_.size() >= kMaxSortingLayerCount ||
		!IsLayerNameAvailable(name, std::numeric_limits<SortingLayerId>::max())) {
		return false;
	}

	// 保存済みObjectが参照するIDを再利用しないよう、現在未使用の最小IDを割り当てる。
	SortingLayerId newId = kDefaultSortingLayerId;
	for(uint32_t candidate = 1; candidate <= std::numeric_limits<SortingLayerId>::max(); ++candidate) {
		const auto id = static_cast<SortingLayerId>(candidate);
		if(!IsValidLayerId(id)) {
			newId = id;
			break;
		}
	}
	if(newId == kDefaultSortingLayerId) return false;

	// 配列順を描画優先順へ変換するため、追加後に全Orderを再構築する。
	layers_.push_back({newId, name, 0});
	RebuildOrders();
	if(outLayerId) *outLayerId = newId;
	return true;
}

bool SortingLayerSettings::RemoveLayer(SortingLayerId layerId) {
	// 既存ObjectのFallback先を保証するため、Default Layerは削除させない。
	if(layerId == kDefaultSortingLayerId) return false;
	const auto it = std::find_if(layers_.begin(), layers_.end(), [layerId](const SortingLayer& layer) {
		return layer.id == layerId;
	});
	if(it == layers_.end()) return false;
	layers_.erase(it);
	RebuildOrders();
	return true;
}

bool SortingLayerSettings::RenameLayer(SortingLayerId layerId, const std::string& newName) {
	if(layerId == kDefaultSortingLayerId || !IsLayerNameAvailable(newName, layerId)) return false;
	auto* layer = const_cast<SortingLayer*>(FindLayer(layerId));
	if(!layer) return false;
	layer->name = newName;
	return true;
}

bool SortingLayerSettings::MoveLayer(SortingLayerId layerId, int direction) {
	if(direction == 0) return false;
	const auto it = std::find_if(layers_.begin(), layers_.end(), [layerId](const SortingLayer& layer) {
		return layer.id == layerId;
	});
	if(it == layers_.end()) return false;
	const auto index = static_cast<size_t>(std::distance(layers_.begin(), it));
	if((direction < 0 && index == 0) || (direction > 0 && index + 1 >= layers_.size())) return false;
	// IDはScene互換性のため固定し、配列位置と描画Orderだけを入れ替える。
	const size_t target = direction < 0 ? index - 1 : index + 1;
	std::iter_swap(layers_.begin() + index, layers_.begin() + target);
	RebuildOrders();
	return true;
}

const SortingLayer* SortingLayerSettings::FindLayer(SortingLayerId layerId) const {
	const auto it = std::find_if(layers_.begin(), layers_.end(), [layerId](const SortingLayer& layer) {
		return layer.id == layerId;
	});
	return it == layers_.end() ? nullptr : &*it;
}

std::optional<SortingLayerId> SortingLayerSettings::FindLayerId(std::string_view name) const {
	const auto it = std::find_if(layers_.begin(), layers_.end(), [name](const SortingLayer& layer) {
		return layer.name == name;
	});
	return it == layers_.end() ? std::nullopt : std::optional<SortingLayerId>(it->id);
}

bool SortingLayerSettings::IsValidLayerId(SortingLayerId layerId) const {
	return FindLayer(layerId) != nullptr;
}

int32_t SortingLayerSettings::GetLayerOrder(SortingLayerId layerId) const {
	const SortingLayer* layer = FindLayer(layerId);
	if(!layer) layer = FindLayer(kDefaultSortingLayerId);
	return layer ? layer->order : 0;
}

nlohmann::json SortingLayerSettings::ToJson() const {
	// ID・名前・順序を明示保存し、Scene再読込後もSpriteのLayer参照を維持する。
	nlohmann::json layers = nlohmann::json::array();
	for(const auto& layer : layers_) {
		layers.push_back({{"id", layer.id}, {"name", layer.name}, {"order", layer.order}});
	}
	return nlohmann::json{{"layers", std::move(layers)}};
}

void SortingLayerSettings::ApplyJson(const nlohmann::json& json) {
	// 破損または旧形式データでも最低限Default Layerを残せる状態から読込を開始する。
	ResetToDefault();
	const auto it = json.find("layers");
	if(!json.is_object() || it == json.end() || !it->is_array()) return;

	// 型、ID範囲、空名称、ID/名称重複を検証し、不正なLayerを設定へ混入させない。
	std::vector<SortingLayer> loaded;
	for(const auto& value : *it) {
		if(!value.is_object() || !value.contains("id") || !value.contains("name")) continue;
		if(!value.at("id").is_number_unsigned() || !value.at("name").is_string()) continue;
		const uint32_t rawId = value.at("id").get<uint32_t>();
		const std::string name = value.at("name").get<std::string>();
		if(rawId > std::numeric_limits<SortingLayerId>::max() || name.empty()) continue;
		const auto id = static_cast<SortingLayerId>(rawId);
		if(std::any_of(loaded.begin(), loaded.end(), [id, &name](const SortingLayer& layer) {
			return layer.id == id || layer.name == name;
		})) continue;
		loaded.push_back({id, name, value.value("order", 0)});
	}

	// 保存Orderを優先し、同値の場合はIDで安定した並びへ正規化する。
	std::sort(loaded.begin(), loaded.end(), [](const SortingLayer& lhs, const SortingLayer& rhs) {
		if(lhs.order != rhs.order) return lhs.order < rhs.order;
		return lhs.id < rhs.id;
	});
	if(loaded.size() > kMaxSortingLayerCount) loaded.resize(kMaxSortingLayerCount);
	layers_ = std::move(loaded);
	// JSONにDefaultがない場合は補完し、名称も予約名へ強制してFallback契約を守る。
	if(!IsValidLayerId(kDefaultSortingLayerId)) {
		layers_.insert(layers_.begin(), {kDefaultSortingLayerId, "Default", 0});
	}
	if(auto* defaultLayer = const_cast<SortingLayer*>(FindLayer(kDefaultSortingLayerId))) {
		defaultLayer->name = "Default";
	}
	RebuildOrders();
}

bool SortingLayerSettings::IsLayerNameAvailable(const std::string& name, SortingLayerId ignoredLayerId) const {
	if(name.empty()) return false;
	return std::none_of(layers_.begin(), layers_.end(), [&](const SortingLayer& layer) {
		return layer.id != ignoredLayerId && layer.name == name;
	});
}

void SortingLayerSettings::RebuildOrders() {
	// Layer間に間隔を持たせ、同Layer内のorderInLayerを合成できる基準値を生成する。
	for(size_t i = 0; i < layers_.size(); ++i) {
		layers_[i].order = static_cast<int32_t>(i) * 100;
	}
}
