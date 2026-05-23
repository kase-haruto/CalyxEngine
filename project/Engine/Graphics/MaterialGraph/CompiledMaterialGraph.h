#pragma once

#include <Engine/Foundation/Math/Vector4.h>

#include <cstdint>

namespace CalyxEngine {
	enum class CompiledSurfaceModel : int32_t {
		Legacy,
		Lit,
		Toon,
		Unlit,
	};

	struct CompiledColorInput {
		Vector4 factor = {1, 1, 1, 1};
		bool usesObjectTexture = false;
	};

	struct CompiledMaterialGraph {
		CompiledSurfaceModel surfaceModel = CompiledSurfaceModel::Legacy;
		int32_t lightingMode = 0;

		CompiledColorInput baseColor;
		float shininess = 20.0f;
		float roughness = 0.5f;
		bool isReflect = false;

		CompiledColorInput toonHighlightColor{{1.08f, 1.06f, 1.02f, 1.0f}, false};
		CompiledColorInput toonBaseColor{{1, 1, 1, 1}, false};
		CompiledColorInput toonFirstShadeColor{{0.72f, 0.76f, 0.86f, 1.0f}, false};
		CompiledColorInput toonSecondShadeColor{{0.42f, 0.46f, 0.58f, 1.0f}, false};
		float toonBaseStep = 0.25f;
		float toonBaseFeather = 0.03f;
		float toonShadeStep = -0.15f;
		float toonShadeFeather = 0.03f;
		float toonSpecularThreshold = 0.96f;
		float toonSpecularSoftness = 0.02f;
		float toonSpecularIntensity = 0.35f;
	};
}
