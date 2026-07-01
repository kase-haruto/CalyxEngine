#pragma once
/////////////////////////////////////////////////////////////////////////////////////////
//	include space
/////////////////////////////////////////////////////////////////////////////////////////

// engine
#include <Engine/Assets/DataAsset/MaterialAsset.h>

// c++
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <string>

namespace CalyxEngine {
	/////////////////////////////////////////////////////////////////////////////////////////
	//		Shader Graph内の単一HLSL式
	/////////////////////////////////////////////////////////////////////////////////////////
	struct ShaderGraphExpression {
		std::string hlsl;
		bool usesObjectTexture = false;
		bool usesNoiseTexture = false;
	};

	/////////////////////////////////////////////////////////////////////////////////////////
	//		Surface全体を保持するShader Graph式IR
	/////////////////////////////////////////////////////////////////////////////////////////
	struct ShaderGraphExpressionIR {
		int32_t lightingMode = 0;
		int32_t textureSlotCount = 0;
		ShaderGraphExpression baseColor;
		ShaderGraphExpression emissiveColor;
		ShaderGraphExpression emissiveIntensity;
		ShaderGraphExpression shininess;
		ShaderGraphExpression roughness;
		bool isReflect = false;
		ShaderGraphExpression normalMap;
		ShaderGraphExpression normalMapStrength;
		ShaderGraphExpression toonHighlightColor;
		ShaderGraphExpression toonBaseColor;
		ShaderGraphExpression toonFirstShadeColor;
		ShaderGraphExpression toonSecondShadeColor;
		ShaderGraphExpression toonBaseStep;
		ShaderGraphExpression toonBaseFeather;
		ShaderGraphExpression toonShadeStep;
		ShaderGraphExpression toonShadeFeather;
		ShaderGraphExpression toonSpecularThreshold;
		ShaderGraphExpression toonSpecularSoftness;
		ShaderGraphExpression toonSpecularIntensity;
	};

	/////////////////////////////////////////////////////////////////////////////////////////
	//		Shader GraphをHLSL式の中間表現へ変換するコンパイラ
	/////////////////////////////////////////////////////////////////////////////////////////
	class ShaderGraphExpressionCompiler {
	private:
		// Shader Graphへ割り当て可能なテクスチャ数。
		static constexpr int32_t kMaxGraphTextures = 8;

		/////////////////////////////////////////////////////////////////////////////////////////
		//		浮動小数点値をHLSLリテラルへ変換
		/////////////////////////////////////////////////////////////////////////////////////////
		static std::string FloatExpr(float value) {
			std::ostringstream out;
			out << std::fixed << std::setprecision(6) << value << "f";
			return out.str();
		}

		/////////////////////////////////////////////////////////////////////////////////////////
		//		Vector4をHLSLのfloat4式へ変換
		/////////////////////////////////////////////////////////////////////////////////////////
		static std::string Float4Expr(const Vector4& value) {
			return "float4(" +
				   FloatExpr(value.x) + ", " +
				   FloatExpr(value.y) + ", " +
				   FloatExpr(value.z) + ", " +
				   FloatExpr(value.w) + ")";
		}

		/////////////////////////////////////////////////////////////////////////////////////////
		//		Float式を生成
		/////////////////////////////////////////////////////////////////////////////////////////
		static ShaderGraphExpression FloatExpression(float value) {
			return {FloatExpr(value), false};
		}

		/////////////////////////////////////////////////////////////////////////////////////////
		//		Color式を生成
		/////////////////////////////////////////////////////////////////////////////////////////
		static ShaderGraphExpression ColorExpression(const Vector4& value) {
			return {Float4Expr(value), false};
		}

		/////////////////////////////////////////////////////////////////////////////////////////
		//		複数式の利用リソース情報を統合
		/////////////////////////////////////////////////////////////////////////////////////////
		static ShaderGraphExpression Combine(std::string hlsl, const ShaderGraphExpression& a, const ShaderGraphExpression& b) {
			return {std::move(hlsl), a.usesObjectTexture || b.usesObjectTexture, a.usesNoiseTexture || b.usesNoiseTexture};
		}

		/////////////////////////////////////////////////////////////////////////////////////////
		//		Noise Textureノードの式を生成
		/////////////////////////////////////////////////////////////////////////////////////////
		static ShaderGraphExpression NoiseValueExpression(const NodeGraph& graph, const Node& node, int32_t depth) {
			ShaderGraphExpression noiseUv = {"uv", false};
			if(const NodePin* uvPin = FindInput(node, "UV")) {
				noiseUv = Float2ExpressionFromInput(graph, uvPin->id, noiseUv, depth + 1);
			}
			ShaderGraphExpression scale = FloatExpression(GetFloatProperty(node, "scale", 8.0f));
			if(const NodePin* scalePin = FindInput(node, "Scale")) {
				scale = FloatExpressionFromInput(graph, scalePin->id, scale, depth + 1);
			}
			return {
				"GeneratedValueNoise(" + noiseUv.hlsl + " * max(" + scale.hlsl + ", 0.0001f))",
				noiseUv.usesObjectTexture || scale.usesObjectTexture,
				true};
		}

		/////////////////////////////////////////////////////////////////////////////////////////
		//		Outputノードを検索
		/////////////////////////////////////////////////////////////////////////////////////////
		static const Node* FindOutput(const NodeGraph& graph) {
			for(const Node& node : graph.nodes) {
				if(node.type == "Output") return &node;
			}
			return nullptr;
		}

		/////////////////////////////////////////////////////////////////////////////////////////
		//		指定名の入力ピンを検索
		/////////////////////////////////////////////////////////////////////////////////////////
		static const NodePin* FindInput(const Node& node, const std::string& name) {
			for(const NodePin& pin : node.inputs) {
				if(pin.name == name) return &pin;
			}
			return nullptr;
		}

		/////////////////////////////////////////////////////////////////////////////////////////
		//		入力ピンへ接続されたノードを検索
		/////////////////////////////////////////////////////////////////////////////////////////
		static const Node* FindLinkedNode(const NodeGraph& graph, int32_t inputPinId) {
			for(const NodeLink& link : graph.links) {
				if(link.toPinId != inputPinId) continue;
				const Node* fromNode = nullptr;
				graph.FindPin(link.fromPinId, &fromNode);
				return fromNode;
			}
			return nullptr;
		}

		/////////////////////////////////////////////////////////////////////////////////////////
		//		グラフで使用するテクスチャ数を取得
		/////////////////////////////////////////////////////////////////////////////////////////
		static int32_t TextureSlotCount(const NodeGraph& graph) {
			int32_t count = 0;
			for(const Node& node : graph.nodes) {
				if(IsTextureSourceNode(node)) ++count;
			}
			return count;
		}

		/////////////////////////////////////////////////////////////////////////////////////////
		//		テクスチャ供給ノードか判定
		/////////////////////////////////////////////////////////////////////////////////////////
		static bool IsTextureSourceNode(const Node& node) {
			return node.type == "ObjectTexture" || node.type == "Texture2D";
		}

		/////////////////////////////////////////////////////////////////////////////////////////
		//		テクスチャ供給ノードのスロットを取得
		/////////////////////////////////////////////////////////////////////////////////////////
		static int32_t TextureSlotForTextureSourceNode(const NodeGraph& graph, const Node& textureNode) {
			int32_t slot = 0;
			for(const Node& node : graph.nodes) {
				if(!IsTextureSourceNode(node)) continue;
				if(node.id == textureNode.id) return slot;
				++slot;
			}
			return 0;
		}

		/////////////////////////////////////////////////////////////////////////////////////////
		//		Texture Sampleが参照するスロットを取得
		/////////////////////////////////////////////////////////////////////////////////////////
		static int32_t TextureSlotForSampleNode(const NodeGraph& graph, const Node& sampleNode) {
			const NodePin* texturePin = FindInput(sampleNode, "Texture");
			const Node* textureNode = texturePin ? FindLinkedNode(graph, texturePin->id) : nullptr;
			if(!textureNode || !IsTextureSourceNode(*textureNode)) return 0;
			return (std::min)(TextureSlotForTextureSourceNode(graph, *textureNode), kMaxGraphTextures - 1);
		}

		/////////////////////////////////////////////////////////////////////////////////////////
		//		入力ピンの接続状態を確認
		/////////////////////////////////////////////////////////////////////////////////////////
		static bool HasInputLink(const NodeGraph& graph, int32_t inputPinId) {
			return std::any_of(graph.links.begin(), graph.links.end(), [inputPinId](const NodeLink& link) {
				return link.toPinId == inputPinId;
			});
		}

		/////////////////////////////////////////////////////////////////////////////////////////
		//		Floatプロパティを安全に取得
		/////////////////////////////////////////////////////////////////////////////////////////
		static float GetFloatProperty(const Node& node, const char* key, float fallback) {
			if(!node.properties.contains(key)) return fallback;
			return node.properties.value(key, fallback);
		}

		/////////////////////////////////////////////////////////////////////////////////////////
		//		Colorプロパティを安全に取得
		/////////////////////////////////////////////////////////////////////////////////////////
		static Vector4 GetColorProperty(const Node& node, const char* key, const Vector4& fallback) {
			auto it = node.properties.find(key);
			if(it == node.properties.end() || !it->is_array() || it->size() != 4) return fallback;
			return {it->at(0).get<float>(), it->at(1).get<float>(), it->at(2).get<float>(), it->at(3).get<float>()};
		}

		/////////////////////////////////////////////////////////////////////////////////////////
		//		Color入力から式を生成
		/////////////////////////////////////////////////////////////////////////////////////////
		static ShaderGraphExpression ColorInputExpression(const MaterialAsset& material, const Node& node, const char* inputName, const Vector4& fallback) {
			return ColorInputExpression(material, node, inputName, ColorExpression(fallback));
		}

		/////////////////////////////////////////////////////////////////////////////////////////
		//		既定式を指定してColor入力から式を生成
		/////////////////////////////////////////////////////////////////////////////////////////
		static ShaderGraphExpression ColorInputExpression(const MaterialAsset& material, const Node& node, const char* inputName, const ShaderGraphExpression& fallback) {
			const NodePin* pin = FindInput(node, inputName);
			if(!pin || !HasInputLink(material.graph, pin->id)) return fallback;
			return ColorExpressionFromInput(material.graph, pin->id, fallback, 0);
		}

		/////////////////////////////////////////////////////////////////////////////////////////
		//		Float入力から式を生成
		/////////////////////////////////////////////////////////////////////////////////////////
		static ShaderGraphExpression FloatInputExpression(const MaterialAsset& material, const Node& node, const char* inputName, float fallback) {
			return FloatInputExpression(material, node, inputName, FloatExpression(fallback));
		}

		/////////////////////////////////////////////////////////////////////////////////////////
		//		既定式を指定してFloat入力から式を生成
		/////////////////////////////////////////////////////////////////////////////////////////
		static ShaderGraphExpression FloatInputExpression(const MaterialAsset& material, const Node& node, const char* inputName, const ShaderGraphExpression& fallback) {
			const NodePin* pin = FindInput(node, inputName);
			if(!pin || !HasInputLink(material.graph, pin->id)) return fallback;
			return FloatExpressionFromInput(material.graph, pin->id, fallback, 0);
		}

		/////////////////////////////////////////////////////////////////////////////////////////
		//		Float2式を生成
		/////////////////////////////////////////////////////////////////////////////////////////
		static ShaderGraphExpression Float2Expression(float x, float y) {
			return {"float2(" + FloatExpr(x) + ", " + FloatExpr(y) + ")", false};
		}

		/////////////////////////////////////////////////////////////////////////////////////////
		//		Float2ノードツリーを再帰的にコンパイル
		/////////////////////////////////////////////////////////////////////////////////////////
		static ShaderGraphExpression Float2ExpressionFromInput(
			const NodeGraph& graph,
			int32_t inputPinId,
			const ShaderGraphExpression& fallback,
			int32_t depth) {
			if(depth > 32) return fallback;
			const Node* fromNode = FindLinkedNode(graph, inputPinId);
			if(!fromNode) return fallback;

			if(fromNode->type == "UV") return {"uv", false};
			if(fromNode->type == "Float2") {
				return Float2Expression(
					GetFloatProperty(*fromNode, "x", 0.0f),
					GetFloatProperty(*fromNode, "y", 0.0f));
			}
			if(fromNode->type == "UVTransform") {
				ShaderGraphExpression sourceUv = {"uv", false};
				ShaderGraphExpression scale = Float2Expression(
					GetFloatProperty(*fromNode, "scaleX", 1.0f),
					GetFloatProperty(*fromNode, "scaleY", 1.0f));
				ShaderGraphExpression offset = Float2Expression(
					GetFloatProperty(*fromNode, "offsetX", 0.0f),
					GetFloatProperty(*fromNode, "offsetY", 0.0f));
				if(const NodePin* uvPin = FindInput(*fromNode, "UV")) {
					sourceUv = Float2ExpressionFromInput(graph, uvPin->id, sourceUv, depth + 1);
				}
				if(const NodePin* scalePin = FindInput(*fromNode, "Scale")) {
					scale = Float2ExpressionFromInput(graph, scalePin->id, scale, depth + 1);
				}
				if(const NodePin* offsetPin = FindInput(*fromNode, "Offset")) {
					offset = Float2ExpressionFromInput(graph, offsetPin->id, offset, depth + 1);
				}
				return {
					"((" + sourceUv.hlsl + " * " + scale.hlsl + ") + " + offset.hlsl + ")",
					sourceUv.usesObjectTexture || scale.usesObjectTexture || offset.usesObjectTexture,
					sourceUv.usesNoiseTexture || scale.usesNoiseTexture || offset.usesNoiseTexture};
			}
			if(fromNode->type == "CombineFloat2" && fromNode->inputs.size() >= 2) {
				const ShaderGraphExpression x = FloatExpressionFromInput(graph, fromNode->inputs[0].id, FloatExpression(0.0f), depth + 1);
				const ShaderGraphExpression y = FloatExpressionFromInput(graph, fromNode->inputs[1].id, FloatExpression(0.0f), depth + 1);
				return {"float2(" + x.hlsl + ", " + y.hlsl + ")", x.usesObjectTexture || y.usesObjectTexture, x.usesNoiseTexture || y.usesNoiseTexture};
			}
			if(fromNode->type == "AddFloat2" && fromNode->inputs.size() >= 2) {
				const ShaderGraphExpression a = Float2ExpressionFromInput(graph, fromNode->inputs[0].id, fallback, depth + 1);
				const ShaderGraphExpression b = Float2ExpressionFromInput(graph, fromNode->inputs[1].id, {"float2(0.0f, 0.0f)", false}, depth + 1);
				return Combine("(" + a.hlsl + " + " + b.hlsl + ")", a, b);
			}
			if(fromNode->type == "SubtractFloat2" && fromNode->inputs.size() >= 2) {
				const ShaderGraphExpression a = Float2ExpressionFromInput(graph, fromNode->inputs[0].id, fallback, depth + 1);
				const ShaderGraphExpression b = Float2ExpressionFromInput(graph, fromNode->inputs[1].id, {"float2(0.0f, 0.0f)", false}, depth + 1);
				return Combine("(" + a.hlsl + " - " + b.hlsl + ")", a, b);
			}
			if(fromNode->type == "MultiplyFloat2" && fromNode->inputs.size() >= 2) {
				const ShaderGraphExpression a = Float2ExpressionFromInput(graph, fromNode->inputs[0].id, fallback, depth + 1);
				const ShaderGraphExpression b = Float2ExpressionFromInput(graph, fromNode->inputs[1].id, {"float2(1.0f, 1.0f)", false}, depth + 1);
				return Combine("(" + a.hlsl + " * " + b.hlsl + ")", a, b);
			}
			if(fromNode->type == "DivideFloat2" && fromNode->inputs.size() >= 2) {
				const ShaderGraphExpression a = Float2ExpressionFromInput(graph, fromNode->inputs[0].id, fallback, depth + 1);
				const ShaderGraphExpression b = Float2ExpressionFromInput(graph, fromNode->inputs[1].id, {"float2(1.0f, 1.0f)", false}, depth + 1);
				return Combine("(" + a.hlsl + " / max(abs(" + b.hlsl + "), float2(0.0001f, 0.0001f)))", a, b);
			}
			return fallback;
		}

		/////////////////////////////////////////////////////////////////////////////////////////
		//		Colorノードツリーを再帰的にコンパイル
		/////////////////////////////////////////////////////////////////////////////////////////
		static ShaderGraphExpression ColorExpressionFromInput(
			const NodeGraph& graph,
			int32_t inputPinId,
			const ShaderGraphExpression& fallback,
			int32_t depth) {
			// 異常なグラフによる無限再帰を防止する。
			if(depth > 32) return fallback;

			// 入力ピンへ接続された出力元ノードを取得する。
			const Node* fromNode = FindLinkedNode(graph, inputPinId);
			if(!fromNode) return fallback;

			// 定数、テクスチャ、演算ノードの順にColor式へ変換する。
			if(fromNode->type == "Color") return ColorExpression(GetColorProperty(*fromNode, "value", {1, 1, 1, 1}));
			if(fromNode->type == "TextureSample") {
				ShaderGraphExpression sampleUv = {"uv", false};
				if(const NodePin* uvPin = FindInput(*fromNode, "UV")) {
					sampleUv = Float2ExpressionFromInput(graph, uvPin->id, sampleUv, depth + 1);
				}
				const int32_t textureSlot = TextureSlotForSampleNode(graph, *fromNode);
				return {"gGraphTextures[" + std::to_string(textureSlot) + "].Sample(gSampler, " + sampleUv.hlsl + ")", true};
			}
			if(fromNode->type == "NoiseTexture") {
				const ShaderGraphExpression value = NoiseValueExpression(graph, *fromNode, depth);
				return {"float4(" + value.hlsl + ", " + value.hlsl + ", " + value.hlsl + ", 1.0f)", value.usesObjectTexture, value.usesNoiseTexture};
			}
			if(fromNode->type == "MultiplyColor" && fromNode->inputs.size() >= 2) {
				const ShaderGraphExpression a = ColorExpressionFromInput(graph, fromNode->inputs[0].id, ColorExpression({1, 1, 1, 1}), depth + 1);
				const ShaderGraphExpression b = ColorExpressionFromInput(graph, fromNode->inputs[1].id, ColorExpression({1, 1, 1, 1}), depth + 1);
				return Combine("(" + a.hlsl + " * " + b.hlsl + ")", a, b);
			}
			if(fromNode->type == "LerpColor" && fromNode->inputs.size() >= 3) {
				const ShaderGraphExpression a = ColorExpressionFromInput(graph, fromNode->inputs[0].id, fallback, depth + 1);
				const ShaderGraphExpression b = ColorExpressionFromInput(graph, fromNode->inputs[1].id, fallback, depth + 1);
				const ShaderGraphExpression t = FloatExpressionFromInput(graph, fromNode->inputs[2].id, FloatExpression(0.0f), depth + 1);
				return {
					("lerp(" + a.hlsl + ", " + b.hlsl + ", saturate(" + t.hlsl + "))"),
					a.usesObjectTexture || b.usesObjectTexture || t.usesObjectTexture,
					a.usesNoiseTexture || b.usesNoiseTexture || t.usesNoiseTexture};
			}
			return fallback;
		}

		/////////////////////////////////////////////////////////////////////////////////////////
		//		Floatノードツリーを再帰的にコンパイル
		/////////////////////////////////////////////////////////////////////////////////////////
		static ShaderGraphExpression FloatExpressionFromInput(
			const NodeGraph& graph,
			int32_t inputPinId,
			const ShaderGraphExpression& fallback,
			int32_t depth) {
			// 異常なグラフによる無限再帰を防止する。
			if(depth > 32) return fallback;

			// 入力ピンへ接続された出力元ノードを取得する。
			const Node* fromNode = FindLinkedNode(graph, inputPinId);
			if(!fromNode) return fallback;

			// 複数出力ノードを識別するため、接続元ピンも取得する。
			const NodePin* fromPin = nullptr;
			for(const NodeLink& link : graph.links) {
				if(link.toPinId == inputPinId) {
					fromPin = graph.FindPin(link.fromPinId);
					break;
				}
			}

			// 定数、テクスチャ、組み込み入力、演算ノードの順にFloat式へ変換する。
			if(fromNode->type == "Float" || fromNode->type == "Shininess" || fromNode->type == "Roughness") return FloatExpression(fromNode->GetProperty<float>("value", 0.0f));
			if(fromNode->type == "TextureSample") {
				ShaderGraphExpression sampleUv = {"uv", false};
				if(const NodePin* uvPin = FindInput(*fromNode, "UV")) {
					sampleUv = Float2ExpressionFromInput(graph, uvPin->id, sampleUv, depth + 1);
				}
				const int32_t textureSlot = TextureSlotForSampleNode(graph, *fromNode);
				const std::string sample = "gGraphTextures[" + std::to_string(textureSlot) + "].Sample(gSampler, " + sampleUv.hlsl + ")";
				if(fromPin && fromPin->name == "Value") {
					return {"dot((" + sample + ").rgb, float3(0.299f, 0.587f, 0.114f))", true};
				}
				return fallback;
			}
			if(fromNode->type == "NoiseTexture") return NoiseValueExpression(graph, *fromNode, depth);
			if(fromNode->type == "SplitFloat2" && fromPin && !fromNode->inputs.empty()) {
				const ShaderGraphExpression value = Float2ExpressionFromInput(graph, fromNode->inputs[0].id, {"float2(0.0f, 0.0f)", false}, depth + 1);
				if(fromPin->name == "X") return {value.hlsl + ".x", value.usesObjectTexture, value.usesNoiseTexture};
				if(fromPin->name == "Y") return {value.hlsl + ".y", value.usesObjectTexture, value.usesNoiseTexture};
			}
			if(fromNode->type == "UVX") return {"uv.x", false};
			if(fromNode->type == "UVY") return {"uv.y", false};
			if(fromNode->type == "Time") return {"gMaterial.pad3", false};
			if(fromNode->type == "WorldPositionX") return {"worldPosition.x", false};
			if(fromNode->type == "WorldPositionY") return {"worldPosition.y", false};
			if(fromNode->type == "WorldPositionZ") return {"worldPosition.z", false};
			if(fromNode->type == "WorldNormalX") return {"normal.x", false};
			if(fromNode->type == "WorldNormalY") return {"normal.y", false};
			if(fromNode->type == "WorldNormalZ") return {"normal.z", false};
			if(fromNode->type == "ViewDirectionX") return {"viewDirection.x", false};
			if(fromNode->type == "ViewDirectionY") return {"viewDirection.y", false};
			if(fromNode->type == "ViewDirectionZ") return {"viewDirection.z", false};
			if(fromNode->type == "AddFloat" && fromNode->inputs.size() >= 2) {
				const ShaderGraphExpression a = FloatExpressionFromInput(graph, fromNode->inputs[0].id, FloatExpression(0.0f), depth + 1);
				const ShaderGraphExpression b = FloatExpressionFromInput(graph, fromNode->inputs[1].id, FloatExpression(0.0f), depth + 1);
				return Combine("(" + a.hlsl + " + " + b.hlsl + ")", a, b);
			}
			if(fromNode->type == "SubtractFloat" && fromNode->inputs.size() >= 2) {
				const ShaderGraphExpression a = FloatExpressionFromInput(graph, fromNode->inputs[0].id, FloatExpression(0.0f), depth + 1);
				const ShaderGraphExpression b = FloatExpressionFromInput(graph, fromNode->inputs[1].id, FloatExpression(0.0f), depth + 1);
				return Combine("(" + a.hlsl + " - " + b.hlsl + ")", a, b);
			}
			if(fromNode->type == "MultiplyFloat" && fromNode->inputs.size() >= 2) {
				const ShaderGraphExpression a = FloatExpressionFromInput(graph, fromNode->inputs[0].id, FloatExpression(1.0f), depth + 1);
				const ShaderGraphExpression b = FloatExpressionFromInput(graph, fromNode->inputs[1].id, FloatExpression(1.0f), depth + 1);
				return Combine("(" + a.hlsl + " * " + b.hlsl + ")", a, b);
			}
			if(fromNode->type == "DivideFloat" && fromNode->inputs.size() >= 2) {
				const ShaderGraphExpression a = FloatExpressionFromInput(graph, fromNode->inputs[0].id, fallback, depth + 1);
				const ShaderGraphExpression b = FloatExpressionFromInput(graph, fromNode->inputs[1].id, FloatExpression(1.0f), depth + 1);
				return Combine("(" + a.hlsl + " / ((" + b.hlsl + ") < 0.0f ? -max(abs(" + b.hlsl + "), 0.0001f) : max(abs(" + b.hlsl + "), 0.0001f)))", a, b);
			}
			if(fromNode->type == "PowerFloat" && fromNode->inputs.size() >= 2) {
				const ShaderGraphExpression a = FloatExpressionFromInput(graph, fromNode->inputs[0].id, FloatExpression(1.0f), depth + 1);
				const ShaderGraphExpression b = FloatExpressionFromInput(graph, fromNode->inputs[1].id, FloatExpression(1.0f), depth + 1);
				return Combine("pow(max(" + a.hlsl + ", 0.0f), " + b.hlsl + ")", a, b);
			}
			if(fromNode->type == "MinFloat" && fromNode->inputs.size() >= 2) {
				const ShaderGraphExpression a = FloatExpressionFromInput(graph, fromNode->inputs[0].id, fallback, depth + 1);
				const ShaderGraphExpression b = FloatExpressionFromInput(graph, fromNode->inputs[1].id, fallback, depth + 1);
				return Combine("min(" + a.hlsl + ", " + b.hlsl + ")", a, b);
			}
			if(fromNode->type == "MaxFloat" && fromNode->inputs.size() >= 2) {
				const ShaderGraphExpression a = FloatExpressionFromInput(graph, fromNode->inputs[0].id, fallback, depth + 1);
				const ShaderGraphExpression b = FloatExpressionFromInput(graph, fromNode->inputs[1].id, fallback, depth + 1);
				return Combine("max(" + a.hlsl + ", " + b.hlsl + ")", a, b);
			}
			if(fromNode->type == "LerpFloat" && fromNode->inputs.size() >= 3) {
				const ShaderGraphExpression a = FloatExpressionFromInput(graph, fromNode->inputs[0].id, fallback, depth + 1);
				const ShaderGraphExpression b = FloatExpressionFromInput(graph, fromNode->inputs[1].id, fallback, depth + 1);
				const ShaderGraphExpression t = FloatExpressionFromInput(graph, fromNode->inputs[2].id, FloatExpression(0.0f), depth + 1);
				return {
					("lerp(" + a.hlsl + ", " + b.hlsl + ", saturate(" + t.hlsl + "))"),
					a.usesObjectTexture || b.usesObjectTexture || t.usesObjectTexture,
					a.usesNoiseTexture || b.usesNoiseTexture || t.usesNoiseTexture};
			}
			if(fromNode->type == "SaturateFloat" && !fromNode->inputs.empty()) {
				const ShaderGraphExpression value = FloatExpressionFromInput(graph, fromNode->inputs[0].id, fallback, depth + 1);
				return {"saturate(" + value.hlsl + ")", value.usesObjectTexture, value.usesNoiseTexture};
			}
			if(fromNode->type == "FracFloat" && !fromNode->inputs.empty()) {
				const ShaderGraphExpression value = FloatExpressionFromInput(graph, fromNode->inputs[0].id, fallback, depth + 1);
				return {"frac(" + value.hlsl + ")", value.usesObjectTexture, value.usesNoiseTexture};
			}
			if(fromNode->type == "OneMinusFloat" && !fromNode->inputs.empty()) {
				const ShaderGraphExpression value = FloatExpressionFromInput(graph, fromNode->inputs[0].id, fallback, depth + 1);
				return {"(1.0f - " + value.hlsl + ")", value.usesObjectTexture, value.usesNoiseTexture};
			}
			if(fromNode->type == "StepFloat" && fromNode->inputs.size() >= 2) {
				const ShaderGraphExpression edge = FloatExpressionFromInput(graph, fromNode->inputs[0].id, FloatExpression(0.5f), depth + 1);
				const ShaderGraphExpression value = FloatExpressionFromInput(graph, fromNode->inputs[1].id, fallback, depth + 1);
				return {
					"step(" + edge.hlsl + ", " + value.hlsl + ")",
					edge.usesObjectTexture || value.usesObjectTexture,
					edge.usesNoiseTexture || value.usesNoiseTexture};
			}
			if(fromNode->type == "SmoothstepFloat" && fromNode->inputs.size() >= 3) {
				const ShaderGraphExpression edge0 = FloatExpressionFromInput(graph, fromNode->inputs[0].id, FloatExpression(0.4f), depth + 1);
				const ShaderGraphExpression edge1 = FloatExpressionFromInput(graph, fromNode->inputs[1].id, FloatExpression(0.6f), depth + 1);
				const ShaderGraphExpression value = FloatExpressionFromInput(graph, fromNode->inputs[2].id, fallback, depth + 1);
				return {
					"smoothstep(" + edge0.hlsl + ", " + edge1.hlsl + ", " + value.hlsl + ")",
					edge0.usesObjectTexture || edge1.usesObjectTexture || value.usesObjectTexture,
					edge0.usesNoiseTexture || edge1.usesNoiseTexture || value.usesNoiseTexture};
			}
			return fallback;
		}

		/////////////////////////////////////////////////////////////////////////////////////////
		//		マテリアル既定値から式IRを初期化
		/////////////////////////////////////////////////////////////////////////////////////////
		static ShaderGraphExpressionIR MakeDefaultExpressionIR(const MaterialAsset& material, bool useMaterialCBufferDefaults) {
			ShaderGraphExpressionIR surface;
			surface.textureSlotCount = (std::min)(TextureSlotCount(material.graph), kMaxGraphTextures);
			surface.lightingMode = material.lightingMode;
			surface.baseColor = useMaterialCBufferDefaults ? ShaderGraphExpression{"gMaterial.color", false} : ColorExpression(material.color);
			surface.emissiveColor = useMaterialCBufferDefaults ? ShaderGraphExpression{"gMaterial.emissiveColor", false} : ColorExpression(material.emissiveColor);
			surface.emissiveIntensity = useMaterialCBufferDefaults ? ShaderGraphExpression{"gMaterial.emissiveIntensity", false} : FloatExpression(material.emissiveIntensity);
			surface.shininess = useMaterialCBufferDefaults ? ShaderGraphExpression{"gMaterial.shiniess", false} : FloatExpression(material.shininess);
			surface.roughness = useMaterialCBufferDefaults ? ShaderGraphExpression{"gMaterial.roughness", false} : FloatExpression(material.roughness);
			surface.isReflect = material.isReflect;
			surface.normalMap = useMaterialCBufferDefaults
				? ShaderGraphExpression{"(gMaterial.useNormalMap != 0 ? gNormalMap.Sample(gSampler, uv) : float4(0.5f, 0.5f, 1.0f, 1.0f))", false}
				: ColorExpression({0.5f, 0.5f, 1.0f, 1.0f});
			surface.normalMapStrength = useMaterialCBufferDefaults ? ShaderGraphExpression{"gMaterial.normalMapStrength", false} : FloatExpression(material.normalMapStrength);
			surface.toonHighlightColor = useMaterialCBufferDefaults ? ShaderGraphExpression{"gMaterial.toonHighlightColor", false} : ColorExpression(material.toonHighlightColor);
			surface.toonBaseColor = useMaterialCBufferDefaults ? ShaderGraphExpression{"gMaterial.toonBaseColor", false} : ColorExpression(material.toonBaseColor);
			surface.toonFirstShadeColor = useMaterialCBufferDefaults ? ShaderGraphExpression{"gMaterial.toonMidShadowColor", false} : ColorExpression(material.toonMidShadowColor);
			surface.toonSecondShadeColor = useMaterialCBufferDefaults ? ShaderGraphExpression{"gMaterial.toonShadowColor", false} : ColorExpression(material.toonShadowColor);
			surface.toonBaseStep = useMaterialCBufferDefaults ? ShaderGraphExpression{"gMaterial.toonBaseStep", false} : FloatExpression(material.toonBaseStep);
			surface.toonBaseFeather = useMaterialCBufferDefaults ? ShaderGraphExpression{"gMaterial.toonBaseFeather", false} : FloatExpression(material.toonBaseFeather);
			surface.toonShadeStep = useMaterialCBufferDefaults ? ShaderGraphExpression{"gMaterial.toonShadeStep", false} : FloatExpression(material.toonShadeStep);
			surface.toonShadeFeather = useMaterialCBufferDefaults ? ShaderGraphExpression{"gMaterial.toonShadeFeather", false} : FloatExpression(material.toonShadeFeather);
			surface.toonSpecularThreshold = useMaterialCBufferDefaults ? ShaderGraphExpression{"gMaterial.toonSpecularThreshold", false} : FloatExpression(material.toonSpecularThreshold);
			surface.toonSpecularSoftness = useMaterialCBufferDefaults ? ShaderGraphExpression{"gMaterial.toonSpecularSoftness", false} : FloatExpression(material.toonSpecularSoftness);
			surface.toonSpecularIntensity = useMaterialCBufferDefaults ? ShaderGraphExpression{"gMaterial.toonSpecularIntensity", false} : FloatExpression(material.toonSpecularIntensity);
			return surface;
		}

		/////////////////////////////////////////////////////////////////////////////////////////
		//		旧Output形式を式IRへ変換
		/////////////////////////////////////////////////////////////////////////////////////////
		static void ApplyLegacyOutputExpressions(const MaterialAsset& material, const Node& output, ShaderGraphExpressionIR& surface) {
			for(const NodePin& pin : output.inputs) {
				if(pin.name == "BaseColor") surface.baseColor = ColorExpressionFromInput(material.graph, pin.id, surface.baseColor, 0);
				if(pin.name == "Emissive") surface.emissiveColor = ColorExpressionFromInput(material.graph, pin.id, surface.emissiveColor, 0);
				if(pin.name == "Emissive Intensity") surface.emissiveIntensity = FloatExpressionFromInput(material.graph, pin.id, surface.emissiveIntensity, 0);
				if(pin.name == "Shininess") surface.shininess = FloatExpressionFromInput(material.graph, pin.id, surface.shininess, 0);
				if(pin.name == "Roughness") surface.roughness = FloatExpressionFromInput(material.graph, pin.id, surface.roughness, 0);
				if(pin.name == "Lighting Mode") {
					if(const Node* linked = FindLinkedNode(material.graph, pin.id)) {
						if(linked->type == "LambertLighting") surface.lightingMode = 1;
						else if(linked->type == "ToonLighting") surface.lightingMode = 2;
						else if(linked->type == "NoLighting") surface.lightingMode = 3;
						else if(linked->type == "UnlitColorLighting") surface.lightingMode = 4;
						else surface.lightingMode = 0;
					}
				}
			}
		}

	public:
		/////////////////////////////////////////////////////////////////////////////////////////
		//		Shader Graphを型付きの式IRへ変換
		/////////////////////////////////////////////////////////////////////////////////////////
		static ShaderGraphExpressionIR Compile(const MaterialAsset& material, bool useMaterialCBufferDefaults = false) {
			// 未接続入力へ使用する既定式をマテリアルから構築する。
			ShaderGraphExpressionIR surface = MakeDefaultExpressionIR(material, useMaterialCBufferDefaults);

			// グラフの終端となるOutputノードを取得する。
			const Node* output = FindOutput(material.graph);
			if(!output) return surface;

			// Output.Surfaceへ接続されたMasterノードを取得する。
			const NodePin* surfacePin = FindInput(*output, "Surface");
			const Node* master = surfacePin ? FindLinkedNode(material.graph, surfacePin->id) : nullptr;
			if(!master) {
				// Masterノードがない既存アセットは旧Output形式として変換する。
				ApplyLegacyOutputExpressions(material, *output, surface);
				return surface;
			}

			if(master->type == "ToonMaster") {
				// Toon固有の色、境界値、法線入力を式IRへ設定する。
				surface.lightingMode = 2;
				const Vector4 baseColor = GetColorProperty(*master, "baseColor", {1, 1, 1, 1});
				const Vector4 emissiveColor = GetColorProperty(*master, "emissiveColor", material.emissiveColor);
				const Vector4 highlightColor = GetColorProperty(*master, "highlightColor", {1.08f, 1.06f, 1.02f, 1.0f});
				const Vector4 firstShade = GetColorProperty(*master, "firstShadeColor", {0.72f, 0.76f, 0.86f, 1.0f});
				const Vector4 secondShade = GetColorProperty(*master, "secondShadeColor", {0.42f, 0.46f, 0.58f, 1.0f});
				surface.baseColor = ColorInputExpression(material, *master, "Base Color", useMaterialCBufferDefaults ? surface.baseColor : ColorExpression(baseColor));
				surface.emissiveColor = ColorInputExpression(material, *master, "Emissive", useMaterialCBufferDefaults ? surface.emissiveColor : ColorExpression(emissiveColor));
				surface.emissiveIntensity = FloatInputExpression(material, *master, "Emissive Intensity", useMaterialCBufferDefaults ? surface.emissiveIntensity : FloatExpression(GetFloatProperty(*master, "emissiveIntensity", material.emissiveIntensity)));
				surface.toonBaseColor = useMaterialCBufferDefaults ? surface.toonBaseColor : ColorExpression({1, 1, 1, 1});
				surface.toonHighlightColor = ColorInputExpression(material, *master, "Highlight", useMaterialCBufferDefaults ? surface.toonHighlightColor : ColorExpression(highlightColor));
				surface.toonFirstShadeColor = ColorInputExpression(material, *master, "1st Shade", useMaterialCBufferDefaults ? surface.toonFirstShadeColor : ColorExpression(firstShade));
				surface.toonSecondShadeColor = ColorInputExpression(material, *master, "2nd Shade", useMaterialCBufferDefaults ? surface.toonSecondShadeColor : ColorExpression(secondShade));
				surface.toonBaseStep = FloatInputExpression(material, *master, "Base Step", useMaterialCBufferDefaults ? surface.toonBaseStep : FloatExpression(GetFloatProperty(*master, "baseStep", 0.25f)));
				surface.toonBaseFeather = FloatInputExpression(material, *master, "Base Feather", useMaterialCBufferDefaults ? surface.toonBaseFeather : FloatExpression(GetFloatProperty(*master, "baseFeather", 0.03f)));
				surface.toonShadeStep = FloatInputExpression(material, *master, "Shade Step", useMaterialCBufferDefaults ? surface.toonShadeStep : FloatExpression(GetFloatProperty(*master, "shadeStep", -0.15f)));
				surface.toonShadeFeather = FloatInputExpression(material, *master, "Shade Feather", useMaterialCBufferDefaults ? surface.toonShadeFeather : FloatExpression(GetFloatProperty(*master, "shadeFeather", 0.03f)));
				surface.toonSpecularThreshold = FloatInputExpression(material, *master, "Spec Threshold", useMaterialCBufferDefaults ? surface.toonSpecularThreshold : FloatExpression(GetFloatProperty(*master, "specularThreshold", 0.96f)));
				surface.toonSpecularSoftness = FloatInputExpression(material, *master, "Spec Softness", useMaterialCBufferDefaults ? surface.toonSpecularSoftness : FloatExpression(GetFloatProperty(*master, "specularSoftness", 0.02f)));
				surface.toonSpecularIntensity = FloatInputExpression(material, *master, "Spec Intensity", useMaterialCBufferDefaults ? surface.toonSpecularIntensity : FloatExpression(GetFloatProperty(*master, "specularIntensity", 0.35f)));
				surface.normalMap = ColorInputExpression(material, *master, "Normal Map", surface.normalMap);
				surface.normalMapStrength = FloatInputExpression(material, *master, "Normal Strength", surface.normalMapStrength);
				return surface;
			}

			if(master->type == "LitMaster") {
				// 標準ライティングに必要な入力を式IRへ設定する。
				surface.lightingMode = static_cast<int32_t>(GetFloatProperty(*master, "lightingMode", 0.0f));
				surface.baseColor = ColorInputExpression(material, *master, "Base Color", useMaterialCBufferDefaults ? surface.baseColor : ColorExpression(material.color));
				surface.emissiveColor = ColorInputExpression(material, *master, "Emissive", useMaterialCBufferDefaults ? surface.emissiveColor : ColorExpression(GetColorProperty(*master, "emissiveColor", material.emissiveColor)));
				surface.emissiveIntensity = FloatInputExpression(material, *master, "Emissive Intensity", useMaterialCBufferDefaults ? surface.emissiveIntensity : FloatExpression(GetFloatProperty(*master, "emissiveIntensity", material.emissiveIntensity)));
				surface.shininess = FloatInputExpression(material, *master, "Shininess", useMaterialCBufferDefaults ? surface.shininess : FloatExpression(GetFloatProperty(*master, "shininess", material.shininess)));
				surface.roughness = FloatInputExpression(material, *master, "Roughness", useMaterialCBufferDefaults ? surface.roughness : FloatExpression(GetFloatProperty(*master, "roughness", material.roughness)));
				surface.normalMap = ColorInputExpression(material, *master, "Normal Map", surface.normalMap);
				surface.normalMapStrength = FloatInputExpression(material, *master, "Normal Strength", surface.normalMapStrength);
				return surface;
			}

			if(master->type == "UnlitMaster") {
				// Unlitに必要な色とEmissive入力だけを式IRへ設定する。
				surface.lightingMode = 4;
				surface.baseColor = ColorInputExpression(material, *master, "Base Color", useMaterialCBufferDefaults ? surface.baseColor : ColorExpression(material.color));
				surface.emissiveColor = ColorInputExpression(material, *master, "Emissive", useMaterialCBufferDefaults ? surface.emissiveColor : ColorExpression(GetColorProperty(*master, "emissiveColor", material.emissiveColor)));
				surface.emissiveIntensity = FloatInputExpression(material, *master, "Emissive Intensity", useMaterialCBufferDefaults ? surface.emissiveIntensity : FloatExpression(GetFloatProperty(*master, "emissiveIntensity", material.emissiveIntensity)));
				return surface;
			}

			// 未知のMasterは旧形式へフォールバックし、既存アセットを維持する。
			ApplyLegacyOutputExpressions(material, *output, surface);
			return surface;
		}
	};
}
