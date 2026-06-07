#pragma once

#include "DataAsset.h"
#include <Engine/Editor/NodeEditor/NodeGraph.h>
#include <Engine/Foundation/Math/Vector4.h>
#include <Engine/Foundation/Math/Matrix4x4.h>
#include <Engine/Lighting/LightData.h>

namespace CalyxEngine {

	/**
	 * @brief マテリアルの各種描画パラメータおよびシェーダーノードグラフ情報を保持するデータアセットクラス
	 */
	class MaterialAsset : public DataAsset {
	public:
		/**
		 * @brief コンストラクタ。デフォルトパラメータ値の設定とフィールド登録を行います。
		 */
		MaterialAsset();

		/**
		 * @brief デストラクタ
		 */
		virtual ~MaterialAsset() override = default;

		/**
		 * @brief アセットタイプ名を取得
		 * @return "MaterialAsset"
		 */
		std::string GetAssetTypeName() const override { return "MaterialAsset"; }

		// --- 描画・マテリアルパラメータ群 ---
		Vector4 color = { 1.0f, 1.0f, 1.0f, 1.0f };             //< 基本色 (RGBA)
		int32_t lightingMode = 0;                                //< ライティングモード (例: 0=Phong, 1=Toon)
		float   shininess = 20.0f;                               //< スペキュラ反射の光沢度（鋭さ）
		bool    isReflect = false;                               //< 環境マッピング反射の有効化フラグ
		float   envirometCoefficient = 0.5f;                     //< 環境マッピングの反射強度係数
		float   roughness = 0.5f;                                //< マテリアルの粗さ（PBRシェーダー等で使用）
		
		// --- トゥーン（セル）シェーダー関連パラメータ ---
		Vector4 toonHighlightColor = {1.15f, 1.10f, 1.00f, 1.0f};//< トゥーンハイライトの色
		Vector4 toonBaseColor = {1.0f, 1.0f, 1.0f, 1.0f};        //< トゥーン基本色
		Vector4 toonMidShadowColor = {0.72f, 0.76f, 0.86f, 1.0f};//< トゥーン中間影（1階調目の影）の色
		Vector4 toonShadowColor = {0.42f, 0.46f, 0.58f, 1.0f};   //< トゥーン暗部影（2階調目の影）の色
		float   toonBaseStep = 0.25f;                            //< トゥーンしきい値（基本色境界）
		float   toonBaseFeather = 0.03f;                         //< トゥーン境界のぼかし幅（基本色境界）
		float   toonShadeStep = -0.15f;                          //< トゥーンしきい値（影境界）
		float   toonShadeFeather = 0.03f;                        //< トゥーン境界のぼかし幅（影境界）
		float   toonThreshold1 = -0.15f;                         //< トゥーン影境界閾値 1
		float   toonThreshold2 = 0.25f;                          //< トゥーン影境界閾値 2
		float   toonThreshold3 = 0.82f;                          //< トゥーン影境界閾値 3
		float   toonEdgeSoftness = 0.03f;                        //< トゥーン境界全体のにじみ・滑らかさ
		float   toonSpecularThreshold = 0.96f;                   //< トゥーンスペキュラ（光沢）しきい値
		float   toonSpecularSoftness = 0.02f;                    //< トゥーンスペキュラ境界の滑らかさ
		float   toonSpecularIntensity = 0.35f;                   //< トゥーンスペキュラ反射強度

		// --- 自己発光 (Emissive) パラメータ ---
		Vector4 emissiveColor = {0.0f, 0.0f, 0.0f, 1.0f};        //< 発光色 (RGB)
		float   emissiveIntensity = 0.0f;                        //< 発光強度係数

		// --- テクスチャリソースの参照 ---
		Guid    objectTextureGuid;                               //< アルベド（ディフューズ）テクスチャアセットのGUID
		Guid    normalMapGuid;                                   //< 法線マップ（ノーマルマップ）テクスチャアセットのGUID
		bool    useNormalMap = false;                            //< 法線マップの適用フラグ
		float   normalMapStrength = 1.0f;                        //< 法線マップの適用強度係数
		bool    normalMapFlipY = false;                          //< 法線マップのY軸（Greenチャンネル）反転フラグ

		// --- UV制御 ---
		Matrix4x4 uvTransform = Matrix4x4::MakeIdentity();       //< テクスチャ座標（UV）のトランスフォーム行列

		// --- シェーダーグラフ ---
		NodeGraph graph;                                         //< マテリアルエディタで編集可能なノードグラフ

	private:
		/**
		 * @brief フィールド登録関数。シリアライズ対象として各メンバ変数を親クラスに紐付けます。
		 */
		void RegisterFields();
	};

}
