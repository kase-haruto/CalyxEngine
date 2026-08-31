#pragma once
#include <Engine/Foundation/Math/Vector2.h>
#include <Engine/Foundation/Math/Vector3.h>
#include <Engine/Foundation/Math/Vector4.h>

/**
 * @brief VertexDataに関するデータを保持する構造体です。
 */
struct VertexData{
	CalyxEngine::Vector4 position;
	CalyxEngine::Vector2 texcoord;
	CalyxEngine::Vector3 normal;
};

