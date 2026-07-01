#pragma once
/////////////////////////////////////////////////////////////////////////////////////////
//	include space
/////////////////////////////////////////////////////////////////////////////////////////

// engine
#include <Engine/Assets/DataAsset/MaterialAsset.h>
#include <Engine/Graphics/MaterialGraph/ShaderGraphExpressionCompiler.h>
#include <Engine/Graphics/MaterialGraph/ShaderGraphHlslEmitter.h>

namespace CalyxEngine {
	/////////////////////////////////////////////////////////////////////////////////////////
	//		Shader Graphコード生成処理のFacade
	/////////////////////////////////////////////////////////////////////////////////////////
	class ShaderGraphCodeGenerator {
	public:
		static constexpr int32_t kMaxGraphTextures = 8;

		/////////////////////////////////////////////////////////////////////////////////////////
		//		マテリアル評価関数を生成
		/////////////////////////////////////////////////////////////////////////////////////////
		static GeneratedShaderGraphCode GenerateObject3DMaterialFunction(const MaterialAsset& material) {
			// Shader GraphをHLSL式の中間表現へ変換する。
			const ShaderGraphExpressionIR expressionIR = ShaderGraphExpressionCompiler::Compile(material);

			// 中間表現からマテリアル評価関数を生成する。
			return ShaderGraphHlslEmitter::GenerateMaterialFunction(expressionIR);
		}

		/////////////////////////////////////////////////////////////////////////////////////////
		//		プレビュー用Pixel Shaderを生成
		/////////////////////////////////////////////////////////////////////////////////////////
		static GeneratedShaderGraphCode GeneratePreviewPixelShaderSource(const MaterialAsset& material) {
			// プレビューではMaterial定数バッファに依存しない式IRを構築する。
			const ShaderGraphExpressionIR expressionIR = ShaderGraphExpressionCompiler::Compile(material, false);

			// マテリアル評価関数だけを先に生成する。
			const GeneratedShaderGraphCode materialFunction = ShaderGraphHlslEmitter::GenerateMaterialFunction(expressionIR);

			// 評価関数をプレビュー用テンプレートへ組み込む。
			return ShaderGraphHlslEmitter::GeneratePreviewPixelShader(materialFunction);
		}

		/////////////////////////////////////////////////////////////////////////////////////////
		//		Object3Dランタイム用Pixel Shaderを生成
		/////////////////////////////////////////////////////////////////////////////////////////
		static GeneratedShaderGraphCode GenerateObject3DRuntimePixelShaderSource(const MaterialAsset& material) {
			// ランタイムでは未接続入力をMaterial定数バッファから取得する。
			const ShaderGraphExpressionIR expressionIR = ShaderGraphExpressionCompiler::Compile(material, true);

			// マテリアル評価関数だけを先に生成する。
			const GeneratedShaderGraphCode materialFunction = ShaderGraphHlslEmitter::GenerateMaterialFunction(expressionIR);

			// 評価関数をObject3D用テンプレートへ組み込む。
			return ShaderGraphHlslEmitter::GenerateRuntimePixelShader(materialFunction);
		}
	};
}
