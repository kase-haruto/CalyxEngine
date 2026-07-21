#include "MaterialAsset.h"

namespace CalyxEngine {

	MaterialAsset::MaterialAsset() {
		// 新規マテリアル作成時のデフォルトの名前
		name_ = "New Material";
		// シリアライズ可能なフィールド群の初期登録
		RegisterFields();
	}

	void MaterialAsset::RegisterFields() {
		// SerializableObject (基底クラス) の AddField を用いて各パラメータをシリアライズ対象としてマッピング
		// これにより、DataAssetManager による JSON 形式での自動保存およびロードが可能になります。
		AddField("color", color);
		AddField("lightingMode", lightingMode);
		AddField("shininess", shininess);
		AddField("isReflect", isReflect);
		AddField("envirometCoefficient", envirometCoefficient);
		AddField("roughness", roughness);
		
		// トゥーンシェーディング用パラメータ
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
		
		// エミッシブ（自己発光）パラメータ
		AddField("emissiveColor", emissiveColor);
		AddField("emissiveIntensity", emissiveIntensity);
		AddField("rimColor", rimColor);
		AddField("rimIntensity", rimIntensity);
		AddField("rimPower", rimPower);
		
		// テクスチャ参照および法線マップ用パラメータ
		AddField("objectTextureGuid", objectTextureGuid);
		AddField("normalMapGuid", normalMapGuid);
		AddField("useNormalMap", useNormalMap);
		AddField("normalMapStrength", normalMapStrength);
		AddField("normalMapFlipY", normalMapFlipY);
		
		// ※ uvTransform (マトリクス) はカスタム処理またはノードグラフ経由で更新・シリアライズするためここでの単純登録は除外
	}

}
