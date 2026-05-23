#pragma once

#include <Engine/Foundation/Math/Quaternion.h>
#include <Engine/Foundation/Math/Vector2.h>
#include <Engine/Foundation/Math/Vector3.h>
#include <Engine/Foundation/Math/Vector4.h>
#include <Engine/Foundation/Utility/Guid/Guid.h>
#include <externals/nlohmann/json.hpp>


#include <cstdint>
#include <string>
#include <type_traits>
#include <variant>

namespace CalyxEngine {

	using Json = nlohmann::json;

	/* =========================================================================
	   対応型
	   ========================================================================= */
	using ValuePtr = std::variant<
		int32_t*,
		size_t*,
		float*,
		bool*,
		CalyxEngine::Vector2*,
		CalyxEngine::Vector3*,
		CalyxEngine::Vector4*,
		CalyxEngine::Quaternion*,
		Guid*,
		const int32_t*,
		const size_t*,
		const float*,
		const bool*,
		const CalyxEngine::Vector2*,
		const CalyxEngine::Vector3*,
		const CalyxEngine::Vector4*,
		const CalyxEngine::Quaternion*,
		const Guid*>;

	/* =========================================================================
	   シリアライズ可能なフィールド情報
	   ========================================================================= */
	struct SerializableField {
		std::string key;
		ValuePtr	ptr;

		// ===== Inspector metadata =====
		std::string category = "Default"; // 変数のカテゴリ
		std::string tooltip;			  // Tooltip

		float speed	   = 0.1f;	// Drag speed
		bool  hasRange = false; // 範囲指定あり
		float min	   = 0.0f;	// 範囲最小値
		float max	   = 0.0f;	// 範囲最大値

		bool readOnly = false; // 表示のみ
		bool hidden	  = false; // Inspector非表示
	};

	/* =========================================================================
	   変数カテゴリノード
	   ========================================================================= */
	struct VariableCategoryNode {
		std::string											  name;
		std::vector<const SerializableField*>				  fields;
		std::unordered_map<std::string, VariableCategoryNode> children;
	};

	/* =========================================================================
	   ValuePtr <-> Json
	   ========================================================================= */
	inline void WriteValue(Json& out, const ValuePtr& ptr) {
		std::visit([&](auto* p) {
			using T = std::remove_pointer_t<decltype(p)>;
			if constexpr(std::is_same_v<T, CalyxEngine::Vector2> || std::is_same_v<T, const CalyxEngine::Vector2>) {
				out = Json::array({p->x, p->y});
			} else if constexpr(std::is_same_v<T, CalyxEngine::Vector3> || std::is_same_v<T, const CalyxEngine::Vector3>) {
				out = Json::array({p->x, p->y, p->z});
			} else if constexpr(std::is_same_v<T, CalyxEngine::Vector4> || std::is_same_v<T, const CalyxEngine::Vector4>) {
				out = Json::array({p->x, p->y, p->z, p->w});
			} else if constexpr(std::is_same_v<T, CalyxEngine::Quaternion> || std::is_same_v<T, const CalyxEngine::Quaternion>) {
				out = Json::array({p->x, p->y, p->z, p->w});
			} else if constexpr(std::is_same_v<T, Guid> || std::is_same_v<T, const Guid>) {
				out = p->ToString();
			} else {
				out = *p;
			}
		},
				   ptr);
	}

	inline bool ReadValue(const Json& in, ValuePtr& ptr) {
		return std::visit([&](auto* p) -> bool {
			using T = std::remove_pointer_t<decltype(p)>;
			if constexpr(std::is_const_v<T>) {
				return false; // constな値は読み込めない
			} else {
				try {
					if constexpr(std::is_same_v<T, int32_t>) {
						if(!in.is_number_integer()) return false;
						*p = in.get<int32_t>();
						return true;
					} else if constexpr(std::is_same_v<T, size_t>) {
						if(!in.is_number()) return false;
						*p = in.get<size_t>();
						return true;
					} else if constexpr(std::is_same_v<T, float>) {
						if(!in.is_number()) return false;
						*p = in.get<float>();
						return true;
					} else if constexpr(std::is_same_v<T, bool>) {
						if(!in.is_boolean()) return false;
						*p = in.get<bool>();
						return true;
					} else if constexpr(std::is_same_v<T, CalyxEngine::Vector2>) {
						if(!in.is_array() || in.size() != 2) return false;
						p->x = in.at(0).get<float>();
						p->y = in.at(1).get<float>();
						return true;
					} else if constexpr(std::is_same_v<T, CalyxEngine::Vector3>) {
						if(!in.is_array() || in.size() != 3) return false;
						p->x = in.at(0).get<float>();
						p->y = in.at(1).get<float>();
						p->z = in.at(2).get<float>();
						return true;
					} else if constexpr(std::is_same_v<T, CalyxEngine::Vector4>) {
						if(!in.is_array() || in.size() != 4) return false;
						p->x = in.at(0).get<float>();
						p->y = in.at(1).get<float>();
						p->z = in.at(2).get<float>();
						p->w = in.at(3).get<float>();
						return true;
					} else if constexpr(std::is_same_v<T, CalyxEngine::Quaternion>) {
						if(!in.is_array() || in.size() != 4) return false;
						p->x = in.at(0).get<float>();
						p->y = in.at(1).get<float>();
						p->z = in.at(2).get<float>();
						p->w = in.at(3).get<float>();
						return true;
					} else if constexpr(std::is_same_v<T, Guid>) {
						if(!in.is_string()) return false;
						*p = Guid::FromString(in.get<std::string>());
						return true;
					} else {
						return false;
					}
				} catch(...) {
					return false;
				}
			}
		},
						  ptr);
	}

} // namespace CalyxEngine
