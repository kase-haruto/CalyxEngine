#pragma once

#include <Engine/Editor/NodeEditor/NodeTypeRegistry.h>

#include <string>
#include <vector>

namespace CalyxEngine {
	/////////////////////////////////////////////////////////////////////////////////////////
	//		Shader Graph固有のピン型ID
	/////////////////////////////////////////////////////////////////////////////////////////
	namespace ShaderGraphPinTypes {
		inline constexpr std::string_view Material = "shader.material";
		inline constexpr std::string_view Texture2D = "shader.texture2d";
	}

	using ShaderGraphPinSchema = NodePinDefinition;
	using ShaderGraphNodeSchema = NodeTypeDefinition;
	using ShaderGraphSchema = NodeTypeRegistry;

	/////////////////////////////////////////////////////////////////////////////////////////
	//		Shader Graphで使用可能なNode型Registry
	/////////////////////////////////////////////////////////////////////////////////////////
	class ShaderGraphNodeRegistry {
	public:
		/////////////////////////////////////////////////////////////////////////////////////////
		//		Object3D Material用Registryを取得
		/////////////////////////////////////////////////////////////////////////////////////////
		static const ShaderGraphSchema& Get() {
			static const ShaderGraphSchema registry = [] {
				ShaderGraphSchema result;
				std::vector<ShaderGraphNodeSchema> definitions = {
				{"Output",
				 "Output",
				 {{"Surface", ShaderGraphPinTypes::Material, NodePinKind::Input}}},
				{"ToonMaster",
				 "Toon Master",
				 {{"Base Color", NodePinTypes::Color, NodePinKind::Input},
				  {"Emissive", NodePinTypes::Color, NodePinKind::Input},
				  {"Emissive Intensity", NodePinTypes::Float, NodePinKind::Input},
				  {"Highlight", NodePinTypes::Color, NodePinKind::Input},
				  {"1st Shade", NodePinTypes::Color, NodePinKind::Input},
				  {"2nd Shade", NodePinTypes::Color, NodePinKind::Input},
				  {"Base Step", NodePinTypes::Float, NodePinKind::Input},
				  {"Base Feather", NodePinTypes::Float, NodePinKind::Input},
				  {"Shade Step", NodePinTypes::Float, NodePinKind::Input},
				  {"Shade Feather", NodePinTypes::Float, NodePinKind::Input},
				  {"Spec Threshold", NodePinTypes::Float, NodePinKind::Input},
				  {"Spec Softness", NodePinTypes::Float, NodePinKind::Input},
				  {"Spec Intensity", NodePinTypes::Float, NodePinKind::Input},
				  {"Normal Map", NodePinTypes::Color, NodePinKind::Input},
				  {"Normal Strength", NodePinTypes::Float, NodePinKind::Input},
				  {"Surface", ShaderGraphPinTypes::Material, NodePinKind::Output}}},
				{"LitMaster",
				 "Lit Master",
				 {{"Base Color", NodePinTypes::Color, NodePinKind::Input},
				  {"Emissive", NodePinTypes::Color, NodePinKind::Input},
				  {"Emissive Intensity", NodePinTypes::Float, NodePinKind::Input},
				  {"Shininess", NodePinTypes::Float, NodePinKind::Input},
				  {"Roughness", NodePinTypes::Float, NodePinKind::Input},
				  {"Reflect", NodePinTypes::Bool, NodePinKind::Input},
				  {"Normal Map", NodePinTypes::Color, NodePinKind::Input},
				  {"Normal Strength", NodePinTypes::Float, NodePinKind::Input},
				  {"Surface", ShaderGraphPinTypes::Material, NodePinKind::Output}}},
				{"UnlitMaster",
				 "Unlit Master",
				 {{"Base Color", NodePinTypes::Color, NodePinKind::Input},
				  {"Emissive", NodePinTypes::Color, NodePinKind::Input},
				  {"Emissive Intensity", NodePinTypes::Float, NodePinKind::Input},
				  {"Surface", ShaderGraphPinTypes::Material, NodePinKind::Output}}},
				{"ObjectTexture",
				 "Object Texture",
				 {{"Texture", ShaderGraphPinTypes::Texture2D, NodePinKind::Output}}},
				{"Texture2D",
				 "Texture2D",
				 {{"Texture", ShaderGraphPinTypes::Texture2D, NodePinKind::Output}}},
				{"TextureSample",
				 "Texture Sample",
				 {{"Texture", ShaderGraphPinTypes::Texture2D, NodePinKind::Input},
				  {"UV", NodePinTypes::Float2, NodePinKind::Input},
				  {"Color", NodePinTypes::Color, NodePinKind::Output},
				  {"Value", NodePinTypes::Float, NodePinKind::Output}}},
				{"NoiseTexture",
				 "Noise Texture",
				 {{"UV", NodePinTypes::Float2, NodePinKind::Input},
				  {"Scale", NodePinTypes::Float, NodePinKind::Input},
				  {"Color", NodePinTypes::Color, NodePinKind::Output},
				  {"Value", NodePinTypes::Float, NodePinKind::Output}}},
				{"Color", "Color", {{"Color", NodePinTypes::Color, NodePinKind::Output}}},
				{"Float", "Float", {{"Value", NodePinTypes::Float, NodePinKind::Output}}},
				{"Shininess", "Shininess", {{"Value", NodePinTypes::Float, NodePinKind::Output}}},
				{"Roughness", "Roughness", {{"Value", NodePinTypes::Float, NodePinKind::Output}}},
				{"Reflect", "Reflect", {{"Value", NodePinTypes::Bool, NodePinKind::Output}}},
				{"LightingMode", "Lighting Mode", {{"Value", NodePinTypes::Int, NodePinKind::Input}, {"Mode", NodePinTypes::Int, NodePinKind::Output}}},
				{"HalfLambertLighting", "Half-Lambert", {{"Lighting", NodePinTypes::Int, NodePinKind::Output}}},
				{"LambertLighting", "Lambert", {{"Lighting", NodePinTypes::Int, NodePinKind::Output}}},
				{"ToonLighting", "Toon Lighting", {{"Lighting", NodePinTypes::Int, NodePinKind::Output}}},
				{"NoLighting", "No Lighting", {{"Lighting", NodePinTypes::Int, NodePinKind::Output}}},
				{"UnlitColorLighting", "Unlit Color", {{"Lighting", NodePinTypes::Int, NodePinKind::Output}}},
				{"AddFloat", "Add Float", {{"A", NodePinTypes::Float, NodePinKind::Input}, {"B", NodePinTypes::Float, NodePinKind::Input}, {"Result", NodePinTypes::Float, NodePinKind::Output}}},
				{"SubtractFloat", "Subtract Float", {{"A", NodePinTypes::Float, NodePinKind::Input}, {"B", NodePinTypes::Float, NodePinKind::Input}, {"Result", NodePinTypes::Float, NodePinKind::Output}}},
				{"MultiplyFloat", "Multiply Float", {{"A", NodePinTypes::Float, NodePinKind::Input}, {"B", NodePinTypes::Float, NodePinKind::Input}, {"Result", NodePinTypes::Float, NodePinKind::Output}}},
				{"DivideFloat", "Divide Float", {{"A", NodePinTypes::Float, NodePinKind::Input}, {"B", NodePinTypes::Float, NodePinKind::Input}, {"Result", NodePinTypes::Float, NodePinKind::Output}}},
				{"PowerFloat", "Power Float", {{"A", NodePinTypes::Float, NodePinKind::Input}, {"B", NodePinTypes::Float, NodePinKind::Input}, {"Result", NodePinTypes::Float, NodePinKind::Output}}},
				{"MinFloat", "Min Float", {{"A", NodePinTypes::Float, NodePinKind::Input}, {"B", NodePinTypes::Float, NodePinKind::Input}, {"Result", NodePinTypes::Float, NodePinKind::Output}}},
				{"MaxFloat", "Max Float", {{"A", NodePinTypes::Float, NodePinKind::Input}, {"B", NodePinTypes::Float, NodePinKind::Input}, {"Result", NodePinTypes::Float, NodePinKind::Output}}},
				{"MultiplyColor", "Multiply Color", {{"A", NodePinTypes::Color, NodePinKind::Input}, {"B", NodePinTypes::Color, NodePinKind::Input}, {"Result", NodePinTypes::Color, NodePinKind::Output}}},
				{"LerpColor", "Lerp Color", {{"A", NodePinTypes::Color, NodePinKind::Input}, {"B", NodePinTypes::Color, NodePinKind::Input}, {"T", NodePinTypes::Float, NodePinKind::Input}, {"Result", NodePinTypes::Color, NodePinKind::Output}}},
				{"LerpFloat", "Lerp Float", {{"A", NodePinTypes::Float, NodePinKind::Input}, {"B", NodePinTypes::Float, NodePinKind::Input}, {"T", NodePinTypes::Float, NodePinKind::Input}, {"Result", NodePinTypes::Float, NodePinKind::Output}}},
				{"SaturateFloat", "Saturate Float", {{"Value", NodePinTypes::Float, NodePinKind::Input}, {"Result", NodePinTypes::Float, NodePinKind::Output}}},
				{"OneMinusFloat", "One Minus Float", {{"Value", NodePinTypes::Float, NodePinKind::Input}, {"Result", NodePinTypes::Float, NodePinKind::Output}}},
				{"Float2", "Float2", {{"Value", NodePinTypes::Float2, NodePinKind::Output}}},
				{"CombineFloat2",
				 "Combine Float2",
				 {{"X", NodePinTypes::Float, NodePinKind::Input},
				  {"Y", NodePinTypes::Float, NodePinKind::Input},
				  {"Value", NodePinTypes::Float2, NodePinKind::Output}}},
				{"AddFloat2",
				 "Add Float2",
				 {{"A", NodePinTypes::Float2, NodePinKind::Input},
				  {"B", NodePinTypes::Float2, NodePinKind::Input},
				  {"Result", NodePinTypes::Float2, NodePinKind::Output}}},
				{"SubtractFloat2",
				 "Subtract Float2",
				 {{"A", NodePinTypes::Float2, NodePinKind::Input},
				  {"B", NodePinTypes::Float2, NodePinKind::Input},
				  {"Result", NodePinTypes::Float2, NodePinKind::Output}}},
				{"MultiplyFloat2",
				 "Multiply Float2",
				 {{"A", NodePinTypes::Float2, NodePinKind::Input},
				  {"B", NodePinTypes::Float2, NodePinKind::Input},
				  {"Result", NodePinTypes::Float2, NodePinKind::Output}}},
				{"DivideFloat2",
				 "Divide Float2",
				 {{"A", NodePinTypes::Float2, NodePinKind::Input},
				  {"B", NodePinTypes::Float2, NodePinKind::Input},
				  {"Result", NodePinTypes::Float2, NodePinKind::Output}}},
				{"SplitFloat2",
				 "Split Float2",
				 {{"Value", NodePinTypes::Float2, NodePinKind::Input},
				  {"X", NodePinTypes::Float, NodePinKind::Output},
				  {"Y", NodePinTypes::Float, NodePinKind::Output}}},
				{"FracFloat",
				 "Frac Float",
				 {{"Value", NodePinTypes::Float, NodePinKind::Input},
				  {"Result", NodePinTypes::Float, NodePinKind::Output}}},
				{"StepFloat",
				 "Step Float",
				 {{"Edge", NodePinTypes::Float, NodePinKind::Input},
				  {"Value", NodePinTypes::Float, NodePinKind::Input},
				  {"Result", NodePinTypes::Float, NodePinKind::Output}}},
				{"SmoothstepFloat",
				 "Smoothstep Float",
				 {{"Edge 0", NodePinTypes::Float, NodePinKind::Input},
				  {"Edge 1", NodePinTypes::Float, NodePinKind::Input},
				  {"Value", NodePinTypes::Float, NodePinKind::Input},
				  {"Result", NodePinTypes::Float, NodePinKind::Output}}},
				{"UV", "UV", {{"Value", NodePinTypes::Float2, NodePinKind::Output}}},
				{"UVTransform",
				 "UV Transform",
				 {{"UV", NodePinTypes::Float2, NodePinKind::Input},
				  {"Scale", NodePinTypes::Float2, NodePinKind::Input},
				  {"Offset", NodePinTypes::Float2, NodePinKind::Input},
				  {"UV", NodePinTypes::Float2, NodePinKind::Output}}},
				{"UVX", "UV X", {{"Value", NodePinTypes::Float, NodePinKind::Output}}},
				{"UVY", "UV Y", {{"Value", NodePinTypes::Float, NodePinKind::Output}}},
				{"Time", "Time", {{"Value", NodePinTypes::Float, NodePinKind::Output}}},
				{"WorldPositionX", "World Position X", {{"Value", NodePinTypes::Float, NodePinKind::Output}}},
				{"WorldPositionY", "World Position Y", {{"Value", NodePinTypes::Float, NodePinKind::Output}}},
				{"WorldPositionZ", "World Position Z", {{"Value", NodePinTypes::Float, NodePinKind::Output}}},
				{"WorldNormalX", "World Normal X", {{"Value", NodePinTypes::Float, NodePinKind::Output}}},
				{"WorldNormalY", "World Normal Y", {{"Value", NodePinTypes::Float, NodePinKind::Output}}},
				{"WorldNormalZ", "World Normal Z", {{"Value", NodePinTypes::Float, NodePinKind::Output}}},
				{"ViewDirectionX", "View Direction X", {{"Value", NodePinTypes::Float, NodePinKind::Output}}},
				{"ViewDirectionY", "View Direction Y", {{"Value", NodePinTypes::Float, NodePinKind::Output}}},
				{"ViewDirectionZ", "View Direction Z", {{"Value", NodePinTypes::Float, NodePinKind::Output}}},
				};

				// Editor表示とNode生成に必要なMetadataを設定して登録する。
				for(ShaderGraphNodeSchema& definition : definitions) {
					ApplyMetadata(definition);
					result.Register(std::move(definition));
				}
				return result;
			}();
			return registry;
		}

	private:
		/////////////////////////////////////////////////////////////////////////////////////////
		//		Node定義へCategory、外観、既定値を設定
		/////////////////////////////////////////////////////////////////////////////////////////
		static void ApplyMetadata(ShaderGraphNodeSchema& definition) {
			definition.category = CategoryFor(definition.type);
			if(definition.category == "Master") definition.headerColor = {0.30f, 0.24f, 0.40f, 1.0f};
			if(definition.category == "Material Parameters") definition.headerColor = {0.38f, 0.30f, 0.13f, 1.0f};
			if(definition.category == "Textures") definition.headerColor = {0.20f, 0.32f, 0.36f, 1.0f};
			if(definition.category == "Lighting") definition.headerColor = {0.17f, 0.34f, 0.52f, 1.0f};
			if(definition.type == "Color") definition.defaultProperties["value"] = {1.0f, 1.0f, 1.0f, 1.0f};
			if(definition.type == "Float" || definition.type == "Shininess" || definition.type == "Roughness") definition.defaultProperties["value"] = 0.0f;
			if(definition.type == "Float2") definition.defaultProperties = {{"x", 0.0f}, {"y", 0.0f}};
			if(definition.type == "Reflect") definition.defaultProperties["value"] = false;
			if(definition.type == "LightingMode") definition.defaultProperties["value"] = 0;
			if(definition.type == "NoiseTexture") definition.defaultProperties["scale"] = 8.0f;
		}

		/////////////////////////////////////////////////////////////////////////////////////////
		//		Node型からEditor表示Categoryを取得
		/////////////////////////////////////////////////////////////////////////////////////////
		static std::string CategoryFor(const std::string& type) {
			if(type == "Output" || type.ends_with("Master")) return "Master";
			if(type == "ObjectTexture" || type == "Texture2D" || type == "TextureSample" || type == "NoiseTexture") return "Textures";
			if(type == "Color" || type == "Float" || type == "Float2" || type == "Shininess" || type == "Roughness" || type == "Reflect") return "Material Parameters";
			if(type == "LightingMode" || type.ends_with("Lighting")) return "Lighting";
			if(type == "UV" || type == "UVTransform" || type == "UVX" || type == "UVY" || type == "Time" || type.starts_with("World") || type.starts_with("View")) return "Inputs";
			return "Operators";
		}
	};
}
