#pragma once

#include <Engine/Foundation/Math/Vector3.h>
#include <externals/nlohmann/json.hpp>

#include <cstdint>
#include <string>
#include <type_traits>
#include <variant>

namespace CalyxEngine {

	using Json = nlohmann::json;

	/* =========================================================================
	   対応型（必要になったら追加）
	   ========================================================================= */
	using ValuePtr = std::variant<
		int32_t*,
		float*,
		bool*,
		CalyxMath::Vector3*,
		CalyxMath::Vector4*>;

	struct SerializableField {
		std::string key;
		ValuePtr	ptr;
	};

	/* =========================================================================
	   ValuePtr <-> Json
	   ========================================================================= */
	inline void WriteValue(Json& out, const ValuePtr& ptr) {
		std::visit([&](auto* p) {
			using T = std::remove_pointer_t<decltype(p)>;
			if constexpr(std::is_same_v<T, CalyxMath::Vector3>) {
				out = Json::array({p->x, p->y, p->z});
			} else {
				out = *p;
			}
		},
				   ptr);
	}

	inline bool ReadValue(const Json& in, ValuePtr& ptr) {
		return std::visit([&](auto* p) -> bool {
			using T = std::remove_pointer_t<decltype(p)>;
			try {
				if constexpr(std::is_same_v<T, int32_t>) {
					if(!in.is_number_integer()) return false;
					*p = in.get<int32_t>();
					return true;
				} else if constexpr(std::is_same_v<T, float>) {
					if(!in.is_number()) return false;
					*p = in.get<float>();
					return true;
				} else if constexpr(std::is_same_v<T, bool>) {
					if(!in.is_boolean()) return false;
					*p = in.get<bool>();
					return true;
				} else if constexpr(std::is_same_v<T, CalyxMath::Vector3>) {
					if(!in.is_array() || in.size() != 3) return false;
					p->x = in.at(0).get<float>();
					p->y = in.at(1).get<float>();
					p->z = in.at(2).get<float>();
					return true;
				} else if constexpr(std::is_same_v<T, CalyxMath::Vector4>) {
					if(!in.is_array() || in.size() != 4) return false;
					p->x = in.at(0).get<float>();
					p->y = in.at(1).get<float>();
					p->z = in.at(2).get<float>();
					p->z = in.at(3).get<float>();
					return true;
				} else {
					return false;
				}
			} catch(...) {
				return false;
			}
		},
						  ptr);
	}

} // namespace CalyxEngine
