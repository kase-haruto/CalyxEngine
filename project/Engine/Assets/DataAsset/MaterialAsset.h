#pragma once

#include "DataAsset.h"
#include <Engine/Editor/NodeEditor/NodeGraph.h>
#include <Engine/Foundation/Math/Vector4.h>
#include <Engine/Foundation/Math/Matrix4x4.h>
#include <Engine/Lighting/LightData.h>

namespace CalyxEngine {

	/**
	 * @brief マテリアルのパラメータを保持するデータアセット
	 */
	class MaterialAsset : public DataAsset {
	public:
		MaterialAsset();
		virtual ~MaterialAsset()override = default;

		std::string GetAssetTypeName() const override { return "MaterialAsset"; }

		// --- Parameters ---
		Vector4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
		int32_t lightingMode = 0;
		float   shininess = 20.0f;
		bool    isReflect = false;
		float   envirometCoefficient = 0.5f;
		float   roughness = 0.5f;
		Vector4 toonHighlightColor = {1.15f, 1.10f, 1.00f, 1.0f};
		Vector4 toonBaseColor = {1.0f, 1.0f, 1.0f, 1.0f};
		Vector4 toonMidShadowColor = {0.72f, 0.76f, 0.86f, 1.0f};
		Vector4 toonShadowColor = {0.42f, 0.46f, 0.58f, 1.0f};
		float   toonBaseStep = 0.25f;
		float   toonBaseFeather = 0.03f;
		float   toonShadeStep = -0.15f;
		float   toonShadeFeather = 0.03f;
		float   toonThreshold1 = -0.15f;
		float   toonThreshold2 = 0.25f;
		float   toonThreshold3 = 0.82f;
		float   toonEdgeSoftness = 0.03f;
		float   toonSpecularThreshold = 0.96f;
		float   toonSpecularSoftness = 0.02f;
		float   toonSpecularIntensity = 0.35f;
		Guid    objectTextureGuid;
		Matrix4x4 uvTransform = Matrix4x4::MakeIdentity();
		NodeGraph graph;

	private:
		void RegisterFields();
	};

}
