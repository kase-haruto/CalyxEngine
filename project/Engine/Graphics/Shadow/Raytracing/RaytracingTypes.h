#pragma once

#include <d3d12.h>

namespace CalyxEngine {

	/*----------------------------------------------------------------
	 *	Raytracing Mode
	 *	- raytracingの有効/無効設定
	 *---------------------------------------------------------------*/
	enum class RaytracingMode {
		Disabled,		 //< raytracing無効
		InlineRaytracing //< Inline raytracing
	};

} // namespace CalyxEngine