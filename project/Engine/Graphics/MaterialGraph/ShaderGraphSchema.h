#pragma once

#include <Engine/Editor/NodeEditor/NodeGraph.h>

#include <string>
#include <vector>

namespace CalyxEngine {
	enum class ShaderGraphPinRole : int32_t {
		Input,
		Output,
	};

	struct ShaderGraphPinSchema {
		std::string name;
		NodeValueType valueType = NodeValueType::None;
		ShaderGraphPinRole role = ShaderGraphPinRole::Input;
	};

	struct ShaderGraphNodeSchema {
		std::string type;
		std::string title;
		std::vector<ShaderGraphPinSchema> pins;
	};

	struct ShaderGraphSchema {
		std::string name;
		std::vector<ShaderGraphNodeSchema> nodes;
	};

	class ShaderGraphSchemas {
	public:
		static ShaderGraphSchema Object3DMaterial() {
			ShaderGraphSchema schema;
			schema.name = "Object3DMaterial";
			schema.nodes = {
				{"Output",
				 "Output",
				 {{"Surface", NodeValueType::Material, ShaderGraphPinRole::Input}}},
				{"ToonMaster",
				 "Toon Master",
				 {{"Base Color", NodeValueType::Color, ShaderGraphPinRole::Input},
				  {"Highlight", NodeValueType::Color, ShaderGraphPinRole::Input},
				  {"1st Shade", NodeValueType::Color, ShaderGraphPinRole::Input},
				  {"2nd Shade", NodeValueType::Color, ShaderGraphPinRole::Input},
				  {"Base Step", NodeValueType::Float, ShaderGraphPinRole::Input},
				  {"Base Feather", NodeValueType::Float, ShaderGraphPinRole::Input},
				  {"Shade Step", NodeValueType::Float, ShaderGraphPinRole::Input},
				  {"Shade Feather", NodeValueType::Float, ShaderGraphPinRole::Input},
				  {"Spec Threshold", NodeValueType::Float, ShaderGraphPinRole::Input},
				  {"Spec Softness", NodeValueType::Float, ShaderGraphPinRole::Input},
				  {"Spec Intensity", NodeValueType::Float, ShaderGraphPinRole::Input},
				  {"Surface", NodeValueType::Material, ShaderGraphPinRole::Output}}},
				{"LitMaster",
				 "Lit Master",
				 {{"Base Color", NodeValueType::Color, ShaderGraphPinRole::Input},
				  {"Shininess", NodeValueType::Float, ShaderGraphPinRole::Input},
				  {"Roughness", NodeValueType::Float, ShaderGraphPinRole::Input},
				  {"Reflect", NodeValueType::Bool, ShaderGraphPinRole::Input},
				  {"Surface", NodeValueType::Material, ShaderGraphPinRole::Output}}},
				{"UnlitMaster",
				 "Unlit Master",
				 {{"Base Color", NodeValueType::Color, ShaderGraphPinRole::Input},
				  {"Surface", NodeValueType::Material, ShaderGraphPinRole::Output}}},
				{"ObjectTexture",
				 "Object Texture",
				 {{"Texture", NodeValueType::Texture2D, ShaderGraphPinRole::Output}}},
				{"Texture2D",
				 "Texture2D",
				 {{"Texture", NodeValueType::Texture2D, ShaderGraphPinRole::Output}}},
				{"TextureSample",
				 "Texture Sample",
				 {{"Texture", NodeValueType::Texture2D, ShaderGraphPinRole::Input},
				  {"UV", NodeValueType::Float2, ShaderGraphPinRole::Input},
				  {"Color", NodeValueType::Color, ShaderGraphPinRole::Output},
				  {"Value", NodeValueType::Float, ShaderGraphPinRole::Output}}},
				{"NoiseTexture",
				 "Noise Texture",
				 {{"UV", NodeValueType::Float2, ShaderGraphPinRole::Input},
				  {"Scale", NodeValueType::Float, ShaderGraphPinRole::Input},
				  {"Color", NodeValueType::Color, ShaderGraphPinRole::Output},
				  {"Value", NodeValueType::Float, ShaderGraphPinRole::Output}}},
				{"Float2", "Float2", {{"Value", NodeValueType::Float2, ShaderGraphPinRole::Output}}},
				{"CombineFloat2",
				 "Combine Float2",
				 {{"X", NodeValueType::Float, ShaderGraphPinRole::Input},
				  {"Y", NodeValueType::Float, ShaderGraphPinRole::Input},
				  {"Value", NodeValueType::Float2, ShaderGraphPinRole::Output}}},
				{"AddFloat2",
				 "Add Float2",
				 {{"A", NodeValueType::Float2, ShaderGraphPinRole::Input},
				  {"B", NodeValueType::Float2, ShaderGraphPinRole::Input},
				  {"Result", NodeValueType::Float2, ShaderGraphPinRole::Output}}},
				{"SubtractFloat2",
				 "Subtract Float2",
				 {{"A", NodeValueType::Float2, ShaderGraphPinRole::Input},
				  {"B", NodeValueType::Float2, ShaderGraphPinRole::Input},
				  {"Result", NodeValueType::Float2, ShaderGraphPinRole::Output}}},
				{"MultiplyFloat2",
				 "Multiply Float2",
				 {{"A", NodeValueType::Float2, ShaderGraphPinRole::Input},
				  {"B", NodeValueType::Float2, ShaderGraphPinRole::Input},
				  {"Result", NodeValueType::Float2, ShaderGraphPinRole::Output}}},
				{"DivideFloat2",
				 "Divide Float2",
				 {{"A", NodeValueType::Float2, ShaderGraphPinRole::Input},
				  {"B", NodeValueType::Float2, ShaderGraphPinRole::Input},
				  {"Result", NodeValueType::Float2, ShaderGraphPinRole::Output}}},
				{"SplitFloat2",
				 "Split Float2",
				 {{"Value", NodeValueType::Float2, ShaderGraphPinRole::Input},
				  {"X", NodeValueType::Float, ShaderGraphPinRole::Output},
				  {"Y", NodeValueType::Float, ShaderGraphPinRole::Output}}},
				{"FracFloat",
				 "Frac Float",
				 {{"Value", NodeValueType::Float, ShaderGraphPinRole::Input},
				  {"Result", NodeValueType::Float, ShaderGraphPinRole::Output}}},
				{"StepFloat",
				 "Step Float",
				 {{"Edge", NodeValueType::Float, ShaderGraphPinRole::Input},
				  {"Value", NodeValueType::Float, ShaderGraphPinRole::Input},
				  {"Result", NodeValueType::Float, ShaderGraphPinRole::Output}}},
				{"SmoothstepFloat",
				 "Smoothstep Float",
				 {{"Edge 0", NodeValueType::Float, ShaderGraphPinRole::Input},
				  {"Edge 1", NodeValueType::Float, ShaderGraphPinRole::Input},
				  {"Value", NodeValueType::Float, ShaderGraphPinRole::Input},
				  {"Result", NodeValueType::Float, ShaderGraphPinRole::Output}}},
				{"UV", "UV", {{"Value", NodeValueType::Float2, ShaderGraphPinRole::Output}}},
				{"UVTransform",
				 "UV Transform",
				 {{"UV", NodeValueType::Float2, ShaderGraphPinRole::Input},
				  {"Scale", NodeValueType::Float2, ShaderGraphPinRole::Input},
				  {"Offset", NodeValueType::Float2, ShaderGraphPinRole::Input},
				  {"UV", NodeValueType::Float2, ShaderGraphPinRole::Output}}},
				{"UVX", "UV X", {{"Value", NodeValueType::Float, ShaderGraphPinRole::Output}}},
				{"UVY", "UV Y", {{"Value", NodeValueType::Float, ShaderGraphPinRole::Output}}},
				{"Time", "Time", {{"Value", NodeValueType::Float, ShaderGraphPinRole::Output}}},
				{"WorldPositionX", "World Position X", {{"Value", NodeValueType::Float, ShaderGraphPinRole::Output}}},
				{"WorldPositionY", "World Position Y", {{"Value", NodeValueType::Float, ShaderGraphPinRole::Output}}},
				{"WorldPositionZ", "World Position Z", {{"Value", NodeValueType::Float, ShaderGraphPinRole::Output}}},
				{"WorldNormalX", "World Normal X", {{"Value", NodeValueType::Float, ShaderGraphPinRole::Output}}},
				{"WorldNormalY", "World Normal Y", {{"Value", NodeValueType::Float, ShaderGraphPinRole::Output}}},
				{"WorldNormalZ", "World Normal Z", {{"Value", NodeValueType::Float, ShaderGraphPinRole::Output}}},
				{"ViewDirectionX", "View Direction X", {{"Value", NodeValueType::Float, ShaderGraphPinRole::Output}}},
				{"ViewDirectionY", "View Direction Y", {{"Value", NodeValueType::Float, ShaderGraphPinRole::Output}}},
				{"ViewDirectionZ", "View Direction Z", {{"Value", NodeValueType::Float, ShaderGraphPinRole::Output}}},
			};
			return schema;
		}
	};
}
