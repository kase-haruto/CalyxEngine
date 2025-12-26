#pragma once

//============================================================================
//	include
//============================================================================

// c++
#include <array>
#include <cstdint>
#include <optional>
#include <string_view>
#include <type_traits>

// imgui
#include <externals/imgui/imgui.h>

// magic_enum
#include <externals/magic_enum/magic_enum.hpp>

//============================================================================
//	EnumConverter
//	enum <-> index / string 変換ユーティリティ
//============================================================================
namespace CalyxUtil {

	template <typename T>
	class EnumConverter {
		static_assert(std::is_enum_v<T>, "EnumConverter requires enum type");

	public:
		using Enum		 = T;
		using Underlying = std::underlying_type_t<T>;

		//====================================================================
		//	Enum meta
		//====================================================================
		static constexpr std::size_t Count() noexcept {
			return magic_enum::enum_count<Enum>();
		}

		//====================================================================
		//	index -> enum
		//====================================================================
		static constexpr Enum FromIndex(std::uint32_t index) noexcept {
			constexpr auto values = magic_enum::enum_values<Enum>();
			return index < values.size() ? values[index] : values.front();
		}

		//====================================================================
		//	enum -> index
		//====================================================================
		static constexpr std::uint32_t ToIndex(Enum value) noexcept {
			auto idx = magic_enum::enum_index(value);
			return idx ? static_cast<std::uint32_t>(*idx) : 0;
		}

		//====================================================================
		//	enum -> string
		//====================================================================
		static constexpr std::string_view ToString(Enum value) noexcept {
			return magic_enum::enum_name(value);
		}

		//====================================================================
		//	string -> enum
		//====================================================================
		static constexpr std::optional<Enum> FromString(std::string_view name) noexcept {
			return magic_enum::enum_cast<Enum>(name);
		}

		//====================================================================
		//	enum name list
		//====================================================================
		static constexpr auto Names() noexcept {
			constexpr auto						  names = magic_enum::enum_names<Enum>();
			std::array<const char*, names.size()> result{};
			for(std::size_t i = 0; i < names.size(); ++i) {
				result[i] = names[i].data();
			}
			return result;
		}

		//====================================================================
		//	ImGui Combo helper
		//====================================================================
		static bool Combo(const char* label, Enum& value) noexcept {
			int		   idx	 = static_cast<int>(ToIndex(value));
			const auto names = Names();

			if(ImGui::Combo(label, &idx, names.data(), static_cast<int>(names.size()))) {
				value = FromIndex(static_cast<std::uint32_t>(idx));
				return true;
			}
			return false;
		}
	};

} // namespace CalyxUtil
