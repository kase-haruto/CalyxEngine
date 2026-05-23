#pragma once

#include <Engine\Application\UI\EngineUI\IEngineUI.h>
#include <Engine\Editor\NodeEditor\NodeEditorCanvas.h>
#include <Engine\Foundation\Utility\Guid\Guid.h>
#include <Engine\Graphics\Buffer\DxConstantBuffer.h>
#include <Engine\Graphics\Material.h>
#include <Engine\Graphics\MaterialGraph\MaterialGraphRuntimeShaderCache.h>
#include <Engine\Graphics\Pipeline\Pso\PipelineStateObject.h>
#include <Engine\Graphics\RenderTarget\OffscreenRT\OffscreenRenderTarget.h>

#include <array>
#include <filesystem>
#include <memory>
#include <string>

namespace CalyxEngine {
	class MaterialAsset;

	class MaterialNodeEditorPanel : public IEngineUI {
	public:
		MaterialNodeEditorPanel();
		void Render() override;

	private:
		void DrawMaterialList();
		void DrawToolbar(MaterialAsset& material);
		void DrawMaterialPreview(MaterialAsset& material, bool framed);
		bool DrawAddNodeMenu(MaterialAsset& material, Vector2 position);
		bool DrawContextMenu(MaterialAsset& material, const NodeEditorCanvas::ContextMenu& menu);
		bool DrawLightingModePopup(MaterialAsset& material);
		void ExecuteGraphCommand(MaterialAsset& material, const char* name, const NodeGraph& before, const NodeGraph& after);
		void CreateMaterialAsset();
		void BeginRenameMaterial(const Guid& guid, const std::filesystem::path& path);
		void CommitRenameMaterial();
		void CancelRenameMaterial();
		bool DrawNodeBody(MaterialAsset& material, Node& node);
		void AddColorNode(MaterialAsset& material, Vector2 position);
		void AddFloatNode(MaterialAsset& material, const char* type, const char* title, Vector2 position);
		void AddFloat2Node(MaterialAsset& material, Vector2 position);
		void AddBoolNode(MaterialAsset& material, const char* type, const char* title, Vector2 position);
		void AddLightingModeNode(MaterialAsset& material, Vector2 position);
		void AddLightingNode(MaterialAsset& material, const char* type, const char* title, int32_t mode, Vector2 position);
		void AddToonMasterNode(MaterialAsset& material, Vector2 position);
		void AddLitMasterNode(MaterialAsset& material, Vector2 position);
		void AddUnlitMasterNode(MaterialAsset& material, Vector2 position);
		void AddObjectTextureNode(MaterialAsset& material, Vector2 position);
		void AddTexture2DNode(MaterialAsset& material, Vector2 position);
		void AddTextureSampleNode(MaterialAsset& material, Vector2 position);
		void AddNoiseTextureNode(MaterialAsset& material, Vector2 position);
		void AddShaderInputFloatNode(MaterialAsset& material, const char* type, const char* title, Vector2 position);
		void AddShaderInputFloat2Node(MaterialAsset& material, const char* type, const char* title, Vector2 position);
		void AddUVTransformNode(MaterialAsset& material, Vector2 position);
		void AddCombineFloat2Node(MaterialAsset& material, Vector2 position);
		void AddSplitFloat2Node(MaterialAsset& material, Vector2 position);
		void AddBinaryNode(MaterialAsset& material, const char* type, const char* title, NodeValueType valueType, Vector2 position);
		void AddLerpNode(MaterialAsset& material, const char* type, const char* title, NodeValueType valueType, Vector2 position);
		void AddUnaryFloatNode(MaterialAsset& material, const char* type, const char* title, Vector2 position);
		void AddStepFloatNode(MaterialAsset& material, Vector2 position);
		void AddSmoothstepFloatNode(MaterialAsset& material, Vector2 position);
		void EnsureOutputNode(MaterialAsset& material);
		bool EnsureTextureSampleNodePins(MaterialAsset& material);
		Vector4 EvaluateColor(const MaterialAsset& material, int32_t inputPinId, const Vector4& fallback) const;
		float EvaluateFloat(const MaterialAsset& material, int32_t inputPinId, float fallback) const;
		bool EvaluateBool(const MaterialAsset& material, int32_t inputPinId, bool fallback) const;
		int32_t EvaluateInt(const MaterialAsset& material, int32_t inputPinId, int32_t fallback) const;
		void ApplyToonLightingNode(MaterialAsset& material, const Node& node) const;
		void Evaluate(MaterialAsset& material);
		void Save(MaterialAsset& material);
		bool EnsurePreviewResources();
		bool EnsurePreviewPipeline(const MaterialGraphRuntimeShader& shader);
		D3D12_GPU_DESCRIPTOR_HANDLE ResolvePreviewTexture(const MaterialAsset& material) const;
		D3D12_GPU_DESCRIPTOR_HANDLE ResolvePreviewTextureTable(const MaterialAsset& material);
		Material BuildPreviewMaterial(const MaterialAsset& material) const;

	private:
		Guid selectedMaterial_;
		NodeEditorCanvas canvas_;
		Guid renamingMaterial_;
		std::filesystem::path renamingPath_;
		std::array<char, 256> renameBuffer_{};
		bool focusRename_ = false;
		bool renamePopupRequested_ = false;
		bool lightingModePopupRequested_ = false;
		int32_t lightingModePopupNodeId_ = 0;
		Vector2 lightingModePopupPos_{0.0f, 0.0f};
		std::string graphStatusMessage_;
		bool graphStatusIsError_ = false;
		MaterialGraphRuntimeShaderCache runtimeShaderCache_;
		std::unique_ptr<OffscreenRenderTarget> previewTarget_;
		DescriptorHandle previewRtv_{};
		DescriptorHandle previewDsv_{};
		DescriptorHandle previewTextureTable_{};
		DxConstantBuffer<Material> previewMaterialBuffer_;
		std::unique_ptr<PipelineStateObject> previewPipeline_;
		std::size_t previewPipelineHash_ = 0;
		bool previewInitialized_ = false;
		bool nodeEditCommandActive_ = false;
		Guid nodeEditMaterial_;
		NodeGraph nodeEditBefore_;
	};
}
