#pragma once

// engine
#include <Engine/Foundation/Utility/Random/Random.h>

// external
#include <externals/nlohmann/json.hpp>

// std
#include <array>
#include <cstdint>
#include <string>

/* --------------------------------------------------------------------
 *		識別id
 *		- 16バイトのユニークIDを表す構造体
 * =----------------------------------------------------------------- */
struct Guid {
	Guid() = default;
	Guid(const std::string& s);
	std::array<std::uint8_t, 16> bytes{};

	static Guid New();
	static Guid Empty();

	/* string ----------------------------------------------------*/
	static Guid FromString(std::string_view s);
	std::string ToString() const;

	/* ユーティリティ ------------------------------------------------------*/
	bool isValid() const noexcept;
	bool operator==(const Guid&) const noexcept	 = default;
	auto operator<=>(const Guid&) const noexcept = default;

	/* JSON 変換 -----------------------------------------------------------*/
	friend void to_json(nlohmann::json& j, const Guid& g) { j = g.ToString(); }
	friend void from_json(const nlohmann::json& j, Guid& g) { g = Guid::FromString(j.get<std::string>()); }
};

/* unordered_map 用ハッシュ -----------------------------------------------*/
template <>
struct std::hash<Guid> {
	std::size_t operator()(const Guid& g) const noexcept {
		std::size_t h = 0;
		for(auto b : g.bytes) h = (h * 131) ^ b;
		return h;
	}
};