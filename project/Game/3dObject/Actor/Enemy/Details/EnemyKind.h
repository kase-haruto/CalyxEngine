#pragma once
#include <cstdint>

/*---------------------------------------------------
 * EnemyKind enum class
 * - 敵の種別
 *-------------------------------------------------*/
enum class EnemyKind : uint8_t {
	Normal = 0, //< 通常型
	Boss,		//< ボス型
};