#include "MaterialAsset.h"

namespace CalyxEngine {

	MaterialAsset::MaterialAsset() {
		name_ = "New Material";
		RegisterFields();
	}

	void MaterialAsset::RegisterFields() {
		// SerializableObject の AddField を使用してパラメータを登録
		AddField("color", color);
		AddField("lightingMode", lightingMode);
		AddField("shininess", shininess);
		AddField("isReflect", isReflect);
		AddField("envirometCoefficient", envirometCoefficient);
		AddField("roughness", roughness);
		AddField("toonHighlightColor", toonHighlightColor);
		AddField("toonBaseColor", toonBaseColor);
		AddField("toonMidShadowColor", toonMidShadowColor);
		AddField("toonShadowColor", toonShadowColor);
		AddField("toonBaseStep", toonBaseStep);
		AddField("toonBaseFeather", toonBaseFeather);
		AddField("toonShadeStep", toonShadeStep);
		AddField("toonShadeFeather", toonShadeFeather);
		AddField("toonThreshold1", toonThreshold1);
		AddField("toonThreshold2", toonThreshold2);
		AddField("toonThreshold3", toonThreshold3);
		AddField("toonEdgeSoftness", toonEdgeSoftness);
		AddField("toonSpecularThreshold", toonSpecularThreshold);
		AddField("toonSpecularSoftness", toonSpecularSoftness);
		AddField("toonSpecularIntensity", toonSpecularIntensity);
		AddField("objectTextureGuid", objectTextureGuid);
		// uvTransform のシリアライズは必要に応じて追加
	}

}
