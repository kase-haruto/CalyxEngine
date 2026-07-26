#pragma once

#include <Engine/Application/UI/EngineUI/IEngineUI.h>
#include <Engine/Editor/NodeEditor/NodeEditorCanvas.h>

#include <array>
#include <filesystem>
#include <vector>

namespace CalyxEngine {
	/*-----------------------------------------------------------------------------------------
	 * PostEffectNodeEditorPanel
	 * - PostEffect Graphの構築とPreset編集を行うEditor専用パネルクラス
	 * - Node編集、実行順抽出、保存読込、Runtime Graphへの反映を担当する
	 * - PostEffect用GPUリソースの生成と所有は各Passへ委譲する
	 *---------------------------------------------------------------------------------------*/
	class PostEffectNodeEditorPanel : public IEngineUI {
	public:
		PostEffectNodeEditorPanel();
		void Render() override;

	private:
		void DrawToolbar();
		bool DrawContextMenu(const NodeEditorCanvas::ContextMenu& menu);
		bool DrawAddNodeMenu(Vector2 position);
		bool DrawNodeBody(Node& node);
		bool DrawNodeInspector(Node& node);
		void DrawInspector();
		bool DrawBlendInputControls(Node& node);
		void ExecuteGraphCommand(const char* name, const NodeGraph& before, const NodeGraph& after);
		void AddInputNode(Vector2 position);
		void AddOutputNode(Vector2 position);
		void AddEffectNode(const std::string& type, Vector2 position);
		void EnsureIoNodes();
		void Save();
		void Load();
		void Apply();
		nlohmann::json BuildPresetJson() const;
		void LoadPresetJson(const nlohmann::json& root);
		std::vector<const Node*> GetExecutionNodes() const;

	private:
		NodeGraph graph_;
		NodeEditorCanvas canvas_;
		std::array<char, 256> pathBuffer_{};
		bool outlineEnabled_ = true;
		bool nodeEditCommandActive_ = false;
		NodeGraph nodeEditBefore_;
	};
}
