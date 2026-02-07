#include "Guid.h"

Guid::Guid(const std::string& s) {
	FromString(s);
}

Guid Guid::New(){
	Guid g;
	for (auto& b : g.bytes){
		b = Random::Generate<std::uint8_t>(0, 255);
	}
	// RFC 4122 版数・バリアント
	g.bytes[6] = (g.bytes[6] & 0x0F) | 0x40;         // version 4
	g.bytes[8] = (g.bytes[8] & 0x3F) | 0x80;         // variant 1
	return g;
}

Guid Guid::Empty(){
	return Guid();
}

Guid Guid::FromString(std::string_view s){
	Guid g; std::size_t i = 0;
	for (char c : s){
		if (c == '-') continue;
		if (i >= 32) break;
		std::uint8_t v = static_cast< std::uint8_t >(
			c <= '9' ? c - '0' : (c | 32) - 'a' + 10);
		g.bytes[i / 2] |= (i & 1) ? v : v << 4;
		++i;
	}
	return g;
}

std::string Guid::ToString() const{
	std::ostringstream oss;
	oss << std::hex << std::setfill('0');
	for (std::size_t i = 0; i < 16; ++i){
		oss << std::setw(2) << static_cast< int >(bytes[i]);
		if (i == 3 || i == 5 || i == 7 || i == 9) oss << '-';
	}
	return oss.str();
}

bool Guid::isValid() const noexcept{
	for (auto b : bytes) if (b) return true;
	return false;
}
