#pragma once

#include <Engine\Assets\DataAsset\MaterialAsset.h>
#include <Engine\Graphics\MaterialGraph\CompiledMaterialGraph.h>

#include <algorithm>
#include <cmath>
#include <string>

namespace CalyxEngine {
	class MaterialGraphCompiler {
	public:
		static void Compile(MaterialAsset& material) {
			const CompiledMaterialGraph compiled = CompileToIR(material);
			ApplyCompiled(material, compiled);
		}

		static CompiledMaterialGraph CompileToIR(const MaterialAsset& material) {
			CompiledMaterialGraph compiled = MakeDefaultCompiled(material);

			const Node* output = FindOutput(material.graph);
			if(!output) return compiled;

			const NodePin* surfacePin = FindInput(*output, "Surface");
			if(surfacePin) {
				const Node* master = FindLinkedNode(material.graph, surfacePin->id);
				if(master) {
					if(master->type == "ToonMaster") {
						CompileToonMasterIR(material, *master, compiled);
						return compiled;
					}
					if(master->type == "LitMaster") {
						CompileLitMasterIR(material, *master, compiled);
						return compiled;
					}
					if(master->type == "UnlitMaster") {
						CompileUnlitMasterIR(material, *master, compiled);
						return compiled;
					}
				}
			}

			CompileLegacyOutputIR(material, *output, compiled);
			return compiled;
		}

	private:
		static CompiledMaterialGraph MakeDefaultCompiled(const MaterialAsset& material) {
			CompiledMaterialGraph compiled;
			compiled.lightingMode = material.lightingMode;
			compiled.baseColor = {material.color, false};
			compiled.emissiveColor = {material.emissiveColor, false};
			compiled.emissiveIntensity = material.emissiveIntensity;
			compiled.shininess = material.shininess;
			compiled.roughness = material.roughness;
			compiled.isReflect = material.isReflect;
			compiled.normalMap = {{0.5f, 0.5f, 1.0f, 1.0f}, false};
			compiled.normalMapStrength = material.normalMapStrength;
			compiled.toonHighlightColor = {material.toonHighlightColor, false};
			compiled.toonBaseColor = {material.toonBaseColor, false};
			compiled.toonFirstShadeColor = {material.toonMidShadowColor, false};
			compiled.toonSecondShadeColor = {material.toonShadowColor, false};
			compiled.toonBaseStep = material.toonBaseStep;
			compiled.toonBaseFeather = material.toonBaseFeather;
			compiled.toonShadeStep = material.toonShadeStep;
			compiled.toonShadeFeather = material.toonShadeFeather;
			compiled.toonSpecularThreshold = material.toonSpecularThreshold;
			compiled.toonSpecularSoftness = material.toonSpecularSoftness;
			compiled.toonSpecularIntensity = material.toonSpecularIntensity;
			return compiled;
		}

		static void ApplyCompiled(MaterialAsset& material, const CompiledMaterialGraph& compiled) {
			material.lightingMode = compiled.lightingMode;
			material.color = compiled.baseColor.factor;
			material.emissiveColor = compiled.emissiveColor.factor;
			material.emissiveIntensity = compiled.emissiveIntensity;
			material.shininess = compiled.shininess;
			material.roughness = compiled.roughness;
			material.isReflect = compiled.isReflect;
			material.normalMapStrength = compiled.normalMapStrength;
			material.toonHighlightColor = compiled.toonHighlightColor.factor;
			material.toonBaseColor = compiled.toonBaseColor.factor;
			material.toonMidShadowColor = compiled.toonFirstShadeColor.factor;
			material.toonShadowColor = compiled.toonSecondShadeColor.factor;
			material.toonBaseStep = compiled.toonBaseStep;
			material.toonBaseFeather = compiled.toonBaseFeather;
			material.toonShadeStep = compiled.toonShadeStep;
			material.toonShadeFeather = compiled.toonShadeFeather;
			material.toonSpecularThreshold = compiled.toonSpecularThreshold;
			material.toonSpecularSoftness = compiled.toonSpecularSoftness;
			material.toonSpecularIntensity = compiled.toonSpecularIntensity;
		}

		static const Node* FindOutput(const NodeGraph& graph) {
			for(const auto& node : graph.nodes) {
				if(node.type == "Output") return &node;
			}
			return nullptr;
		}

		static const NodePin* FindInput(const Node& node, const std::string& name) {
			for(const auto& pin : node.inputs) {
				if(pin.name == name) return &pin;
			}
			return nullptr;
		}

		static const Node* FindLinkedNode(const NodeGraph& graph, int32_t inputPinId) {
			for(const auto& link : graph.links) {
				if(link.toPinId != inputPinId) continue;
				const Node* fromNode = nullptr;
				graph.FindPin(link.fromPinId, &fromNode);
				return fromNode;
			}
			return nullptr;
		}

		static float GetFloatProperty(const Node& node, const char* key, float fallback) {
			if(!node.properties.contains(key)) return fallback;
			return node.properties.value(key, fallback);
		}

		static Vector4 GetColorProperty(const Node& node, const char* key, const Vector4& fallback) {
			auto it = node.properties.find(key);
			if(it == node.properties.end() || !it->is_array() || it->size() != 4) return fallback;
			return {it->at(0).get<float>(), it->at(1).get<float>(), it->at(2).get<float>(), it->at(3).get<float>()};
		}

		static Vector4 EvaluateColor(const MaterialAsset& material, int32_t inputPinId, const Vector4& fallback) {
			for(const auto& link : material.graph.links) {
				if(link.toPinId != inputPinId) continue;
				const Node* fromNode = nullptr;
				const NodePin* fromPin = material.graph.FindPin(link.fromPinId, &fromNode);
				if(!fromNode || !fromPin || fromPin->valueType != NodeValueType::Color) return fallback;
				if(fromNode->type == "Color") return fromNode->colorValue;
				if(fromNode->type == "TextureSample") {
					return {1, 1, 1, 1};
				}
				if(fromNode->type == "NoiseTexture") {
					return {0.5f, 0.5f, 0.5f, 1.0f};
				}
				if(fromNode->type == "MultiplyColor") {
					return EvaluateColor(material, fromNode->inputs[0].id, {1, 1, 1, 1}) *
						   EvaluateColor(material, fromNode->inputs[1].id, {1, 1, 1, 1});
				}
				if(fromNode->type == "LerpColor") {
					const Vector4 a = EvaluateColor(material, fromNode->inputs[0].id, fallback);
					const Vector4 b = EvaluateColor(material, fromNode->inputs[1].id, fallback);
					const float t = std::clamp(EvaluateFloat(material, fromNode->inputs[2].id, 0.0f), 0.0f, 1.0f);
					return {
						a.x + (b.x - a.x) * t,
						a.y + (b.y - a.y) * t,
						a.z + (b.z - a.z) * t,
						a.w + (b.w - a.w) * t};
				}
			}
			return fallback;
		}

		static float EvaluateFloat(const MaterialAsset& material, int32_t inputPinId, float fallback) {
			for(const auto& link : material.graph.links) {
				if(link.toPinId != inputPinId) continue;
				const Node* fromNode = nullptr;
				const NodePin* fromPin = material.graph.FindPin(link.fromPinId, &fromNode);
				if(!fromNode || !fromPin || fromPin->valueType != NodeValueType::Float) return fallback;
				if(fromNode->type == "Float" || fromNode->type == "Shininess" || fromNode->type == "Roughness") return fromNode->floatValue;
				if(fromNode->type == "TextureSample") return 0.5f;
				if(fromNode->type == "NoiseTexture") return 0.5f;
				if(fromNode->type == "UVX" || fromNode->type == "UVY" || fromNode->type == "Time" ||
				   fromNode->type == "WorldPositionX" || fromNode->type == "WorldPositionY" || fromNode->type == "WorldPositionZ" ||
				   fromNode->type == "WorldNormalX" || fromNode->type == "WorldNormalY" || fromNode->type == "WorldNormalZ" ||
				   fromNode->type == "ViewDirectionX" || fromNode->type == "ViewDirectionY" || fromNode->type == "ViewDirectionZ") {
					return fallback;
				}
				if(fromNode->type == "AddFloat") {
					return EvaluateFloat(material, fromNode->inputs[0].id, 0.0f) +
						   EvaluateFloat(material, fromNode->inputs[1].id, 0.0f);
				}
				if(fromNode->type == "SubtractFloat") {
					return EvaluateFloat(material, fromNode->inputs[0].id, 0.0f) -
						   EvaluateFloat(material, fromNode->inputs[1].id, 0.0f);
				}
				if(fromNode->type == "MultiplyFloat") {
					return EvaluateFloat(material, fromNode->inputs[0].id, 1.0f) *
						   EvaluateFloat(material, fromNode->inputs[1].id, 1.0f);
				}
				if(fromNode->type == "DivideFloat") {
					const float b = EvaluateFloat(material, fromNode->inputs[1].id, 1.0f);
					const float denominator = std::abs(b) < 0.0001f ? (b < 0.0f ? -0.0001f : 0.0001f) : b;
					return EvaluateFloat(material, fromNode->inputs[0].id, fallback) / denominator;
				}
				if(fromNode->type == "PowerFloat") {
					return std::pow(
						(std::max)(EvaluateFloat(material, fromNode->inputs[0].id, 1.0f), 0.0f),
						EvaluateFloat(material, fromNode->inputs[1].id, 1.0f));
				}
				if(fromNode->type == "MinFloat") {
					return (std::min)(
						EvaluateFloat(material, fromNode->inputs[0].id, fallback),
						EvaluateFloat(material, fromNode->inputs[1].id, fallback));
				}
				if(fromNode->type == "MaxFloat") {
					return (std::max)(
						EvaluateFloat(material, fromNode->inputs[0].id, fallback),
						EvaluateFloat(material, fromNode->inputs[1].id, fallback));
				}
				if(fromNode->type == "LerpFloat") {
					const float a = EvaluateFloat(material, fromNode->inputs[0].id, fallback);
					const float b = EvaluateFloat(material, fromNode->inputs[1].id, fallback);
					const float t = std::clamp(EvaluateFloat(material, fromNode->inputs[2].id, 0.0f), 0.0f, 1.0f);
					return a + (b - a) * t;
				}
				if(fromNode->type == "SaturateFloat") {
					return std::clamp(EvaluateFloat(material, fromNode->inputs[0].id, fallback), 0.0f, 1.0f);
				}
				if(fromNode->type == "FracFloat") {
					const float value = EvaluateFloat(material, fromNode->inputs[0].id, fallback);
					return value - std::floor(value);
				}
				if(fromNode->type == "OneMinusFloat") {
					return 1.0f - EvaluateFloat(material, fromNode->inputs[0].id, fallback);
				}
				if(fromNode->type == "StepFloat") {
					const float edge = EvaluateFloat(material, fromNode->inputs[0].id, 0.5f);
					const float value = EvaluateFloat(material, fromNode->inputs[1].id, fallback);
					return value < edge ? 0.0f : 1.0f;
				}
				if(fromNode->type == "SmoothstepFloat") {
					const float edge0 = EvaluateFloat(material, fromNode->inputs[0].id, 0.4f);
					const float edge1 = EvaluateFloat(material, fromNode->inputs[1].id, 0.6f);
					const float value = EvaluateFloat(material, fromNode->inputs[2].id, fallback);
					const float width = std::abs(edge1 - edge0) < 0.0001f ? 0.0001f : edge1 - edge0;
					const float t = std::clamp((value - edge0) / width, 0.0f, 1.0f);
					return t * t * (3.0f - 2.0f * t);
				}
			}
			return fallback;
		}

		static bool EvaluateBool(const MaterialAsset& material, int32_t inputPinId, bool fallback) {
			for(const auto& link : material.graph.links) {
				if(link.toPinId != inputPinId) continue;
				const Node* fromNode = nullptr;
				const NodePin* fromPin = material.graph.FindPin(link.fromPinId, &fromNode);
				if(!fromNode || !fromPin || fromPin->valueType != NodeValueType::Bool) return fallback;
				if(fromNode->type == "Reflect") return fromNode->boolValue;
			}
			return fallback;
		}

		static bool UsesObjectTexture(const MaterialAsset& material, int32_t inputPinId) {
			for(const auto& link : material.graph.links) {
				if(link.toPinId != inputPinId) continue;
				const Node* fromNode = nullptr;
				const NodePin* fromPin = material.graph.FindPin(link.fromPinId, &fromNode);
				if(!fromNode || !fromPin) return false;
				if(fromNode->type == "TextureSample") return true;
				if(fromNode->type == "MultiplyColor") {
					return UsesObjectTexture(material, fromNode->inputs[0].id) ||
						   UsesObjectTexture(material, fromNode->inputs[1].id);
				}
				if(fromNode->type == "LerpColor") {
					return UsesObjectTexture(material, fromNode->inputs[0].id) ||
						   UsesObjectTexture(material, fromNode->inputs[1].id);
				}
			}
			return false;
		}

		static CompiledColorInput EvaluateColorInput(const MaterialAsset& material, int32_t inputPinId, const Vector4& fallback) {
			return {EvaluateColor(material, inputPinId, fallback), UsesObjectTexture(material, inputPinId)};
		}

		static int32_t EvaluateLightingMode(MaterialAsset& material, int32_t inputPinId, int32_t fallback) {
			for(const auto& link : material.graph.links) {
				if(link.toPinId != inputPinId) continue;
				const Node* fromNode = nullptr;
				const NodePin* fromPin = material.graph.FindPin(link.fromPinId, &fromNode);
				if(!fromNode || !fromPin || fromPin->valueType != NodeValueType::Int) return fallback;
				if(fromNode->type == "LightingMode") return std::clamp(fromNode->intValue, 0, 4);
				if(fromNode->type == "HalfLambertLighting") return 0;
				if(fromNode->type == "LambertLighting") return 1;
				if(fromNode->type == "ToonLighting") {
					CompileToonLightingNode(material, *fromNode);
					return 2;
				}
				if(fromNode->type == "NoLighting") return 3;
				if(fromNode->type == "UnlitColorLighting") return 4;
			}
			return fallback;
		}

		static int32_t EvaluateLightingModeIR(const MaterialAsset& material, int32_t inputPinId, int32_t fallback, CompiledMaterialGraph& compiled) {
			for(const auto& link : material.graph.links) {
				if(link.toPinId != inputPinId) continue;
				const Node* fromNode = nullptr;
				const NodePin* fromPin = material.graph.FindPin(link.fromPinId, &fromNode);
				if(!fromNode || !fromPin || fromPin->valueType != NodeValueType::Int) return fallback;
				if(fromNode->type == "LightingMode") return std::clamp(fromNode->intValue, 0, 4);
				if(fromNode->type == "HalfLambertLighting") return 0;
				if(fromNode->type == "LambertLighting") return 1;
				if(fromNode->type == "ToonLighting") {
					CompileToonLightingNodeIR(material, *fromNode, compiled);
					return 2;
				}
				if(fromNode->type == "NoLighting") return 3;
				if(fromNode->type == "UnlitColorLighting") return 4;
			}
			return fallback;
		}

		static void CompileLegacyOutputIR(const MaterialAsset& material, const Node& output, CompiledMaterialGraph& compiled) {
			compiled.surfaceModel = CompiledSurfaceModel::Legacy;
			for(const auto& pin : output.inputs) {
				if(pin.name == "BaseColor") compiled.baseColor = EvaluateColorInput(material, pin.id, compiled.baseColor.factor);
				if(pin.name == "Emissive") compiled.emissiveColor = EvaluateColorInput(material, pin.id, compiled.emissiveColor.factor);
				if(pin.name == "Emissive Intensity") compiled.emissiveIntensity = EvaluateFloat(material, pin.id, compiled.emissiveIntensity);
				if(pin.name == "Shininess") compiled.shininess = EvaluateFloat(material, pin.id, compiled.shininess);
				if(pin.name == "Roughness") compiled.roughness = EvaluateFloat(material, pin.id, compiled.roughness);
				if(pin.name == "Reflect") compiled.isReflect = EvaluateBool(material, pin.id, compiled.isReflect);
				if(pin.name == "Lighting Mode") compiled.lightingMode = EvaluateLightingModeIR(material, pin.id, compiled.lightingMode, compiled);
			}
		}

		static void CompileLitMasterIR(const MaterialAsset& material, const Node& node, CompiledMaterialGraph& compiled) {
			compiled.surfaceModel = CompiledSurfaceModel::Lit;
			compiled.lightingMode = static_cast<int32_t>(GetFloatProperty(node, "lightingMode", 0.0f));
			if(const NodePin* pin = FindInput(node, "Base Color")) compiled.baseColor = EvaluateColorInput(material, pin->id, GetColorProperty(node, "baseColor", compiled.baseColor.factor));
			if(const NodePin* pin = FindInput(node, "Emissive")) compiled.emissiveColor = EvaluateColorInput(material, pin->id, GetColorProperty(node, "emissiveColor", compiled.emissiveColor.factor));
			if(const NodePin* pin = FindInput(node, "Emissive Intensity")) compiled.emissiveIntensity = EvaluateFloat(material, pin->id, GetFloatProperty(node, "emissiveIntensity", compiled.emissiveIntensity));
			if(const NodePin* pin = FindInput(node, "Shininess")) compiled.shininess = EvaluateFloat(material, pin->id, GetFloatProperty(node, "shininess", compiled.shininess));
			if(const NodePin* pin = FindInput(node, "Roughness")) compiled.roughness = EvaluateFloat(material, pin->id, GetFloatProperty(node, "roughness", compiled.roughness));
			if(const NodePin* pin = FindInput(node, "Reflect")) compiled.isReflect = EvaluateBool(material, pin->id, compiled.isReflect);
			if(const NodePin* pin = FindInput(node, "Normal Map")) compiled.normalMap = EvaluateColorInput(material, pin->id, compiled.normalMap.factor);
			if(const NodePin* pin = FindInput(node, "Normal Strength")) compiled.normalMapStrength = EvaluateFloat(material, pin->id, compiled.normalMapStrength);
		}

		static void CompileUnlitMasterIR(const MaterialAsset& material, const Node& node, CompiledMaterialGraph& compiled) {
			compiled.surfaceModel = CompiledSurfaceModel::Unlit;
			compiled.lightingMode = 4;
			if(const NodePin* pin = FindInput(node, "Base Color")) compiled.baseColor = EvaluateColorInput(material, pin->id, GetColorProperty(node, "baseColor", compiled.baseColor.factor));
			if(const NodePin* pin = FindInput(node, "Emissive")) compiled.emissiveColor = EvaluateColorInput(material, pin->id, GetColorProperty(node, "emissiveColor", compiled.emissiveColor.factor));
			if(const NodePin* pin = FindInput(node, "Emissive Intensity")) compiled.emissiveIntensity = EvaluateFloat(material, pin->id, GetFloatProperty(node, "emissiveIntensity", compiled.emissiveIntensity));
		}

		static void CompileToonMasterIR(const MaterialAsset& material, const Node& node, CompiledMaterialGraph& compiled) {
			compiled.surfaceModel = CompiledSurfaceModel::Toon;
			compiled.lightingMode = 2;

			const Vector4 baseColor = GetColorProperty(node, "baseColor", {1, 1, 1, 1});
			const Vector4 highlightColor = GetColorProperty(node, "highlightColor", {1.08f, 1.06f, 1.02f, 1.0f});
			const Vector4 firstShade = GetColorProperty(node, "firstShadeColor", {0.72f, 0.76f, 0.86f, 1.0f});
			const Vector4 secondShade = GetColorProperty(node, "secondShadeColor", {0.42f, 0.46f, 0.58f, 1.0f});
			const Vector4 emissiveColor = GetColorProperty(node, "emissiveColor", compiled.emissiveColor.factor);

			compiled.baseColor = {baseColor, false};
			compiled.toonBaseColor = {{1, 1, 1, 1}, false};
			compiled.toonHighlightColor = {highlightColor, false};
			compiled.toonFirstShadeColor = {firstShade, false};
			compiled.toonSecondShadeColor = {secondShade, false};

			float shadeStep = GetFloatProperty(node, "shadeStep", -0.15f);
			float baseStep = GetFloatProperty(node, "baseStep", 0.25f);
			float shadeFeather = GetFloatProperty(node, "shadeFeather", 0.03f);
			float baseFeather = GetFloatProperty(node, "baseFeather", 0.03f);
			float specularThreshold = GetFloatProperty(node, "specularThreshold", 0.96f);
			float specularSoftness = GetFloatProperty(node, "specularSoftness", 0.02f);
			float specularIntensity = GetFloatProperty(node, "specularIntensity", 0.35f);

			if(const NodePin* pin = FindInput(node, "Base Color")) compiled.baseColor = EvaluateColorInput(material, pin->id, baseColor);
			if(const NodePin* pin = FindInput(node, "Emissive")) compiled.emissiveColor = EvaluateColorInput(material, pin->id, emissiveColor);
			if(const NodePin* pin = FindInput(node, "Emissive Intensity")) compiled.emissiveIntensity = EvaluateFloat(material, pin->id, GetFloatProperty(node, "emissiveIntensity", compiled.emissiveIntensity));
			if(const NodePin* pin = FindInput(node, "Highlight")) compiled.toonHighlightColor = EvaluateColorInput(material, pin->id, highlightColor);
			if(const NodePin* pin = FindInput(node, "1st Shade")) compiled.toonFirstShadeColor = EvaluateColorInput(material, pin->id, firstShade);
			if(const NodePin* pin = FindInput(node, "2nd Shade")) compiled.toonSecondShadeColor = EvaluateColorInput(material, pin->id, secondShade);
			if(const NodePin* pin = FindInput(node, "Shade Step")) shadeStep = EvaluateFloat(material, pin->id, shadeStep);
			if(const NodePin* pin = FindInput(node, "Base Step")) baseStep = EvaluateFloat(material, pin->id, baseStep);
			if(const NodePin* pin = FindInput(node, "Shade Feather")) shadeFeather = EvaluateFloat(material, pin->id, shadeFeather);
			if(const NodePin* pin = FindInput(node, "Base Feather")) baseFeather = EvaluateFloat(material, pin->id, baseFeather);
			if(const NodePin* pin = FindInput(node, "Spec Threshold")) specularThreshold = EvaluateFloat(material, pin->id, specularThreshold);
			if(const NodePin* pin = FindInput(node, "Spec Softness")) specularSoftness = EvaluateFloat(material, pin->id, specularSoftness);
			if(const NodePin* pin = FindInput(node, "Spec Intensity")) specularIntensity = EvaluateFloat(material, pin->id, specularIntensity);
			if(const NodePin* pin = FindInput(node, "Normal Map")) compiled.normalMap = EvaluateColorInput(material, pin->id, compiled.normalMap.factor);
			if(const NodePin* pin = FindInput(node, "Normal Strength")) compiled.normalMapStrength = EvaluateFloat(material, pin->id, compiled.normalMapStrength);

			compiled.toonShadeStep = shadeStep;
			compiled.toonShadeFeather = shadeFeather;
			compiled.toonBaseStep = baseStep;
			compiled.toonBaseFeather = baseFeather;
			compiled.toonSpecularThreshold = specularThreshold;
			compiled.toonSpecularSoftness = specularSoftness;
			compiled.toonSpecularIntensity = specularIntensity;
		}

		static void CompileToonLightingNodeIR(const MaterialAsset& material, const Node& node, CompiledMaterialGraph& compiled) {
			compiled.toonHighlightColor = {GetColorProperty(node, "toonHighlightColor", material.toonHighlightColor), false};
			compiled.toonBaseColor = {GetColorProperty(node, "toonBaseColor", material.toonBaseColor), false};
			compiled.toonFirstShadeColor = {GetColorProperty(node, "toonMidShadowColor", material.toonMidShadowColor), false};
			compiled.toonSecondShadeColor = {GetColorProperty(node, "toonShadowColor", material.toonShadowColor), false};
			compiled.toonShadeStep = GetFloatProperty(node, "toonThreshold1", material.toonThreshold1);
			compiled.toonShadeFeather = GetFloatProperty(node, "toonEdgeSoftness", material.toonEdgeSoftness);
			compiled.toonBaseStep = GetFloatProperty(node, "toonThreshold2", material.toonThreshold2);
			compiled.toonBaseFeather = compiled.toonShadeFeather;
			compiled.toonSpecularThreshold = GetFloatProperty(node, "toonSpecularThreshold", material.toonSpecularThreshold);
			compiled.toonSpecularSoftness = GetFloatProperty(node, "toonSpecularSoftness", material.toonSpecularSoftness);
			compiled.toonSpecularIntensity = GetFloatProperty(node, "toonSpecularIntensity", material.toonSpecularIntensity);
		}

		static void CompileLegacyOutput(MaterialAsset& material, const Node& output) {
			for(const auto& pin : output.inputs) {
				if(pin.name == "BaseColor") material.color = EvaluateColor(material, pin.id, material.color);
				if(pin.name == "Emissive") material.emissiveColor = EvaluateColor(material, pin.id, material.emissiveColor);
				if(pin.name == "Emissive Intensity") material.emissiveIntensity = EvaluateFloat(material, pin.id, material.emissiveIntensity);
				if(pin.name == "Shininess") material.shininess = EvaluateFloat(material, pin.id, material.shininess);
				if(pin.name == "Roughness") material.roughness = EvaluateFloat(material, pin.id, material.roughness);
				if(pin.name == "Reflect") material.isReflect = EvaluateBool(material, pin.id, material.isReflect);
				if(pin.name == "Lighting Mode") material.lightingMode = EvaluateLightingMode(material, pin.id, material.lightingMode);
			}
		}

		static void CompileLitMaster(MaterialAsset& material, const Node& node) {
			material.lightingMode = static_cast<int32_t>(GetFloatProperty(node, "lightingMode", 0.0f));
			if(const NodePin* pin = FindInput(node, "Base Color")) material.color = EvaluateColor(material, pin->id, GetColorProperty(node, "baseColor", material.color));
			if(const NodePin* pin = FindInput(node, "Emissive")) material.emissiveColor = EvaluateColor(material, pin->id, GetColorProperty(node, "emissiveColor", material.emissiveColor));
			if(const NodePin* pin = FindInput(node, "Emissive Intensity")) material.emissiveIntensity = EvaluateFloat(material, pin->id, GetFloatProperty(node, "emissiveIntensity", material.emissiveIntensity));
			if(const NodePin* pin = FindInput(node, "Shininess")) material.shininess = EvaluateFloat(material, pin->id, GetFloatProperty(node, "shininess", material.shininess));
			if(const NodePin* pin = FindInput(node, "Roughness")) material.roughness = EvaluateFloat(material, pin->id, GetFloatProperty(node, "roughness", material.roughness));
			if(const NodePin* pin = FindInput(node, "Reflect")) material.isReflect = EvaluateBool(material, pin->id, material.isReflect);
		}

		static void CompileUnlitMaster(MaterialAsset& material, const Node& node) {
			material.lightingMode = 4;
			if(const NodePin* pin = FindInput(node, "Base Color")) material.color = EvaluateColor(material, pin->id, GetColorProperty(node, "baseColor", material.color));
			if(const NodePin* pin = FindInput(node, "Emissive")) material.emissiveColor = EvaluateColor(material, pin->id, GetColorProperty(node, "emissiveColor", material.emissiveColor));
			if(const NodePin* pin = FindInput(node, "Emissive Intensity")) material.emissiveIntensity = EvaluateFloat(material, pin->id, GetFloatProperty(node, "emissiveIntensity", material.emissiveIntensity));
		}

		static void CompileToonMaster(MaterialAsset& material, const Node& node) {
			material.lightingMode = 2;

			const Vector4 baseColor = GetColorProperty(node, "baseColor", {1, 1, 1, 1});
			const Vector4 highlightColor = GetColorProperty(node, "highlightColor", {1.08f, 1.06f, 1.02f, 1.0f});
			const Vector4 firstShade = GetColorProperty(node, "firstShadeColor", {0.72f, 0.76f, 0.86f, 1.0f});
			const Vector4 secondShade = GetColorProperty(node, "secondShadeColor", {0.42f, 0.46f, 0.58f, 1.0f});
			const Vector4 emissiveColor = GetColorProperty(node, "emissiveColor", material.emissiveColor);

			if(const NodePin* pin = FindInput(node, "Base Color")) material.color = EvaluateColor(material, pin->id, baseColor);
			else material.color = baseColor;
			if(const NodePin* pin = FindInput(node, "Emissive")) material.emissiveColor = EvaluateColor(material, pin->id, emissiveColor);
			if(const NodePin* pin = FindInput(node, "Emissive Intensity")) material.emissiveIntensity = EvaluateFloat(material, pin->id, GetFloatProperty(node, "emissiveIntensity", material.emissiveIntensity));

			material.toonBaseColor = {1, 1, 1, 1};
			material.toonHighlightColor = highlightColor;
			material.toonMidShadowColor = firstShade;
			material.toonShadowColor = secondShade;

			float shadeStep = GetFloatProperty(node, "shadeStep", -0.15f);
			float baseStep = GetFloatProperty(node, "baseStep", 0.25f);
			float shadeFeather = GetFloatProperty(node, "shadeFeather", 0.03f);
			float baseFeather = GetFloatProperty(node, "baseFeather", 0.03f);

			float specularThreshold = GetFloatProperty(node, "specularThreshold", 0.96f);
			float specularSoftness = GetFloatProperty(node, "specularSoftness", 0.02f);
			float specularIntensity = GetFloatProperty(node, "specularIntensity", 0.35f);

			if(const NodePin* pin = FindInput(node, "Highlight")) material.toonHighlightColor = EvaluateColor(material, pin->id, highlightColor);
			if(const NodePin* pin = FindInput(node, "1st Shade")) material.toonMidShadowColor = EvaluateColor(material, pin->id, firstShade);
			if(const NodePin* pin = FindInput(node, "2nd Shade")) material.toonShadowColor = EvaluateColor(material, pin->id, secondShade);
			if(const NodePin* pin = FindInput(node, "Shade Step")) shadeStep = EvaluateFloat(material, pin->id, shadeStep);
			if(const NodePin* pin = FindInput(node, "Base Step")) baseStep = EvaluateFloat(material, pin->id, baseStep);
			if(const NodePin* pin = FindInput(node, "Shade Feather")) shadeFeather = EvaluateFloat(material, pin->id, shadeFeather);
			if(const NodePin* pin = FindInput(node, "Base Feather")) baseFeather = EvaluateFloat(material, pin->id, baseFeather);
			if(const NodePin* pin = FindInput(node, "Spec Threshold")) specularThreshold = EvaluateFloat(material, pin->id, specularThreshold);
			if(const NodePin* pin = FindInput(node, "Spec Softness")) specularSoftness = EvaluateFloat(material, pin->id, specularSoftness);
			if(const NodePin* pin = FindInput(node, "Spec Intensity")) specularIntensity = EvaluateFloat(material, pin->id, specularIntensity);

			material.toonShadeStep = shadeStep;
			material.toonShadeFeather = shadeFeather;
			material.toonBaseStep = baseStep;
			material.toonBaseFeather = baseFeather;
			material.toonSpecularThreshold = specularThreshold;
			material.toonSpecularSoftness = specularSoftness;
			material.toonSpecularIntensity = specularIntensity;
		}

		static void CompileToonLightingNode(MaterialAsset& material, const Node& node) {
			material.toonHighlightColor = GetColorProperty(node, "toonHighlightColor", material.toonHighlightColor);
			material.toonBaseColor = GetColorProperty(node, "toonBaseColor", material.toonBaseColor);
			material.toonMidShadowColor = GetColorProperty(node, "toonMidShadowColor", material.toonMidShadowColor);
			material.toonShadowColor = GetColorProperty(node, "toonShadowColor", material.toonShadowColor);
			material.toonThreshold1 = GetFloatProperty(node, "toonThreshold1", material.toonThreshold1);
			material.toonThreshold2 = GetFloatProperty(node, "toonThreshold2", material.toonThreshold2);
			material.toonThreshold3 = GetFloatProperty(node, "toonThreshold3", material.toonThreshold3);
			material.toonEdgeSoftness = GetFloatProperty(node, "toonEdgeSoftness", material.toonEdgeSoftness);
			material.toonShadeStep = material.toonThreshold1;
			material.toonShadeFeather = material.toonEdgeSoftness;
			material.toonBaseStep = material.toonThreshold2;
			material.toonBaseFeather = material.toonEdgeSoftness;
			material.toonSpecularThreshold = GetFloatProperty(node, "toonSpecularThreshold", material.toonSpecularThreshold);
			material.toonSpecularSoftness = GetFloatProperty(node, "toonSpecularSoftness", material.toonSpecularSoftness);
			material.toonSpecularIntensity = GetFloatProperty(node, "toonSpecularIntensity", material.toonSpecularIntensity);
		}
	};
}
