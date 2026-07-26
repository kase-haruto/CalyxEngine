#pragma once
#include "AssetType.h"
#include <Engine/Foundation/Utility/Guid/Guid.h>

/*-----------------------------------------------------------------------------------------
 * AssetDragPayload
 * - Editor内のDrag and DropでAssetを識別するデータ構造
 * - Asset種別と安定GUIDだけを受け渡し、Asset本体は所有しない
 *---------------------------------------------------------------------------------------*/
struct AssetDragPayload {
	AssetType type;
	Guid guid;
};
