#pragma once
#include "AssetType.h"
#include <Engine/Foundation/Utility/Guid/Guid.h>

struct AssetDragPayload {
	AssetType type;
	Guid guid;
};
