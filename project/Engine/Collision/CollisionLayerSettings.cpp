#include "CollisionLayerSettings.h"

#include <algorithm>
#include <array>

CollisionLayerSettings* CollisionLayerSettings::activeSettings_ = nullptr;

CollisionLayerSettings* CollisionLayerSettings::GetInstance() {
	// Collider、CollisionManager、Editor UIは現在シーンが所有する同じ設定を参照する。
	// SceneContext外でColliderコードが呼ばれた場合に備え、Defaultだけを持つfallbackも用意する。
	static CollisionLayerSettings fallbackSettings;
	return activeSettings_ ? activeSettings_ : &fallbackSettings;
}

void CollisionLayerSettings::SetActiveSettings(CollisionLayerSettings* settings) {
	// 所有権はSceneContextにあり、ここでは現在シーンを指す非所有ポインタだけを切り替える。
	activeSettings_ = settings;
}

CollisionLayerSettings::CollisionLayerSettings() {
	ResetToDefault();
}

void CollisionLayerSettings::ResetToDefault() {
	// シーン読込前の設定を完全に破棄し、旧シーンにも必ずDefaultが存在する状態へ戻す。
	layers_.clear();
	matrix_ = CollisionMatrix{};
	// Default は設定が未作成の Collider も必ず参照できる基準 Layer なので、削除不可で常設する。
	layers_.push_back({kDefaultCollisionLayerId, "Default"});
	matrix_.SetCanCollide(kDefaultCollisionLayerId, kDefaultCollisionLayerId, true);
}

bool CollisionLayerSettings::AddLayer(const std::string& name, CollisionLayerId* outLayerId) {
	// 名前はInspector上の識別子として使うため、空文字と重複を許可しない。
	// Layer数もMatrixが表現可能な32を超えないよう、ID探索より前に拒否する。
	if(layers_.size() >= kMaxCollisionLayerCount ||
		!IsLayerNameAvailable(name, static_cast<CollisionLayerId>(0xff))) {
		return false;
	}

	// 0はDefault専用なので1から探索する。
	// 削除によって空いたIDを小さい順に再利用し、一覧とMatrixをコンパクトに保つ。
	CollisionLayerId newId = kDefaultCollisionLayerId;
	for(uint32_t candidate = 1; candidate < kMaxCollisionLayerCount; ++candidate) {
		const auto id = static_cast<CollisionLayerId>(candidate);
		if(!IsValidLayerId(id)) {
			newId = id;
			break;
		}
	}
	if(newId == kDefaultCollisionLayerId) {
		// sizeチェックと探索結果が矛盾した場合も、Defaultを上書きせず安全に失敗させる。
		return false;
	}

	layers_.push_back({newId, name});
	// 削除済みIDの再利用では末尾追加だけでは順序が崩れるため、ID順へ戻す。
	std::sort(layers_.begin(), layers_.end(), [](const CollisionLayer& lhs, const CollisionLayer& rhs) {
		return lhs.id < rhs.id;
	});

	// 新規 Layer は既存 Layer と衝突可能にし、追加直後も従来の形状判定が機能する初期値にする。
	for(const auto& layer : layers_) {
		matrix_.SetCanCollide(newId, layer.id, true);
	}

	if(outLayerId) {
		// UIは追加直後のLayerを選択するため、要求された場合だけ割り当てIDを返す。
		*outLayerId = newId;
	}
	return true;
}

bool CollisionLayerSettings::RemoveLayer(CollisionLayerId layerId) {
	// Layer 0 は未設定データのフォールバック先なので、削除すると既存 Collider の意味が不安定になる。
	if(layerId == kDefaultCollisionLayerId) {
		return false;
	}

	const auto it = std::find_if(layers_.begin(), layers_.end(), [layerId](const CollisionLayer& layer) {
		return layer.id == layerId;
	});
	if(it == layers_.end()) {
		// 二重削除や範囲内だが未登録のIDは、状態を変更せず失敗として返す。
		return false;
	}

	layers_.erase(it);
	// 削除済み ID を保持する Collider が残っていても衝突しないよう、行と列をともに無効化する。
	matrix_.ClearLayer(layerId);
	return true;
}

bool CollisionLayerSettings::RenameLayer(CollisionLayerId layerId, const std::string& newName) {
	// Defaultはシリアライズの基準名として固定し、シーン間でLayer 0の意味が変わらないようにする。
	if(layerId == kDefaultCollisionLayerId || !IsLayerNameAvailable(newName, layerId)) {
		return false;
	}

	const auto it = std::find_if(layers_.begin(), layers_.end(), [layerId](const CollisionLayer& layer) {
		return layer.id == layerId;
	});
	if(it == layers_.end()) {
		// 削除済みIDのリネーム要求は無視し、参照切れLayerを再生成しない。
		return false;
	}

	it->name = newName;
	return true;
}

const std::string& CollisionLayerSettings::GetLayerName(CollisionLayerId layerId) const {
	const auto it = std::find_if(layers_.begin(), layers_.end(), [layerId](const CollisionLayer& layer) {
		return layer.id == layerId;
	});
	if(it != layers_.end()) {
		return it->name;
	}

	// 一時stringを返すと参照が破棄されるため、寿命がプログラム全体の固定文字列を返す。
	// Default名へ偽装せず参照切れを表示することで、設定漏れをEditor上で発見しやすくする。
	static const std::string invalidLayerName = "Invalid Layer";
	return invalidLayerName;
}

bool CollisionLayerSettings::IsValidLayerId(CollisionLayerId layerId) const {
	return std::any_of(layers_.begin(), layers_.end(), [layerId](const CollisionLayer& layer) {
		return layer.id == layerId;
	});
}

nlohmann::json CollisionLayerSettings::ToJson() const {
	nlohmann::json layersJson = nlohmann::json::array();
	for(const auto& layer : layers_) {
		layersJson.push_back({{"id", layer.id}, {"name", layer.name}});
	}

	// 32行を固定長で保存すると、削除済みIDの行も含めて形式が安定し、読込処理も単純になる。
	nlohmann::json matrixRows = nlohmann::json::array();
	for(uint32_t row = 0; row < kMaxCollisionLayerCount; ++row) {
		uint32_t rowBits = 0u;
		for(uint32_t column = 0; column < kMaxCollisionLayerCount; ++column) {
			if(matrix_.CanCollide(static_cast<CollisionLayerId>(row), static_cast<CollisionLayerId>(column))) {
				rowBits |= (1u << column);
			}
		}
		matrixRows.push_back(rowBits);
	}

	return nlohmann::json{{"layers", std::move(layersJson)}, {"matrixRows", std::move(matrixRows)}};
}

void CollisionLayerSettings::ApplyJson(const nlohmann::json& json) {
	ResetToDefault();
	if(!json.is_object()) {
		return;
	}

	// Defaultはファイル内容を信用せず固定値を維持し、それ以外の妥当なLayerだけを復元する。
	if(const auto layersIt = json.find("layers"); layersIt != json.end() && layersIt->is_array()) {
		for(const auto& layerJson : *layersIt) {
			if(!layerJson.is_object() || !layerJson.contains("id") ||
				!layerJson.at("id").is_number_unsigned() || !layerJson.contains("name") ||
				!layerJson.at("name").is_string()) {
				continue;
			}
			const uint32_t rawId = layerJson.at("id").get<uint32_t>();
			const std::string name = layerJson.at("name").get<std::string>();
			if(rawId == kDefaultCollisionLayerId || rawId >= kMaxCollisionLayerCount || name.empty()) {
				continue;
			}
			const auto id = static_cast<CollisionLayerId>(rawId);
			if(IsValidLayerId(id) || !IsLayerNameAvailable(name, static_cast<CollisionLayerId>(0xff))) {
				continue;
			}
			layers_.push_back({id, name});
		}
	}
	std::sort(layers_.begin(), layers_.end(), [](const CollisionLayer& lhs, const CollisionLayer& rhs) {
		return lhs.id < rhs.id;
	});

	const auto matrixIt = json.find("matrixRows");
	if(matrixIt == json.end() || !matrixIt->is_array()) {
		// Matrixを持たない設定では、追加済みLayer同士を衝突可能にして旧データの挙動を保つ。
		for(const auto& layerA : layers_) {
			for(const auto& layerB : layers_) {
				matrix_.SetCanCollide(layerA.id, layerB.id, true);
			}
		}
		return;
	}

	// 一度全セルを消し、現在存在するLayer同士の設定だけを対称API経由で復元する。
	std::array<uint32_t, kMaxCollisionLayerCount> serializedRows{};
	for(size_t row = 0; row < serializedRows.size() && row < matrixIt->size(); ++row) {
		if((*matrixIt)[row].is_number_unsigned()) {
			serializedRows[row] = (*matrixIt)[row].get<uint32_t>();
		}
	}

	matrix_ = CollisionMatrix{};
	for(size_t indexA = 0; indexA < layers_.size(); ++indexA) {
		for(size_t indexB = indexA; indexB < layers_.size(); ++indexB) {
			const auto layerA = layers_[indexA].id;
			const auto layerB = layers_[indexB].id;
			// 壊れたファイルで片側だけONでも情報を失わず、SetCanCollideで対称な状態へ正規化する。
			const bool canCollide =
				(serializedRows[layerA] & ToCollisionLayerBit(layerB)) != 0u ||
				(serializedRows[layerB] & ToCollisionLayerBit(layerA)) != 0u;
			matrix_.SetCanCollide(layerA, layerB, canCollide);
		}
	}
}

bool CollisionLayerSettings::IsLayerNameAvailable(const std::string& name, CollisionLayerId ignoredLayerId) const {
	if(name.empty()) {
		return false;
	}

	// Renameでは自分自身の現在名を許可する必要があるため、ignoredLayerIdだけ比較対象から外す。
	// Addでは存在しない0xffを渡し、全Layerを比較対象にする。
	return std::none_of(layers_.begin(), layers_.end(), [&](const CollisionLayer& layer) {
		return layer.id != ignoredLayerId && layer.name == name;
	});
}
