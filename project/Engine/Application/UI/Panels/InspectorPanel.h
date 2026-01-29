#pragma once

// engine
#include <Engine/Application/UI/EngineUI/IEngineUI.h>
#include <Engine/Objects/3D/Actor/SceneObject.h>

// std
#include <memory>
#include <string>


namespace CalyxEditor {

	class BaseEditor;
	class SceneObjectEditor;

	/*------------------------------------------------
	 * ParamFilterSection
	 * - パラメータフィルターセクション列挙型
	 *----------------------------------------------*/
	enum class ParamFilterSection {
		All,           //< すべて
		Object,        //< オブジェクト
		Material,      //< マテリアル
		ParameterData, //< パラメータデータ
		Collider,      //< コライダー

		ParticleEmit,   //< エミット
		ParticleModule, //< モジュール

		Unknown //< 不明
	};

	/*-----------------------------------------------------------------------------------------
	 * InspectorPanel
	 * - インスペクターパネルクラス
	 * - 選択されたオブジェクト・エディタのプロパティ調整を表示
	 *---------------------------------------------------------------------------------------*/
	class InspectorPanel
		: public IEngineUI {
	public:
		InspectorPanel();
		~InspectorPanel() override = default;

		/**
		 * @imgui描画
		 */
		void Render() override;
		/**
		 * パネル名取得
		 * @return
		 */
		const std::string& GetPanelName() const override { return panelName_; }
		/**
		 * 調整先のエディタを設定
		 * @param editor
		 */
		void SetSelectedEditor(BaseEditor* editor);
		/**
		 * 調整先のオブジェクトを設定
		 * @param obj
		 */
		void SetSelectedObject(std::weak_ptr<SceneObject> obj);
		/**
		 * @調整先のエディタをセット
		 * @param editor
		 */
		void SetSceneObjectEditor(SceneObjectEditor* editor) { sceneObjectEditor_ = editor; }

	private:
		// Tabs
		struct InspectorTab {
			std::string        iconPath;          // Texture path
			ParamFilterSection filterSection;     // フィルターセクション
			void*              iconTex = nullptr; // Runtime texture ID (D3D12_GPU_DESCRIPTOR_HANDLE::ptr)
		};

		void RenderSidebar();
		void RenderContent();

		BaseEditor*                selectedEditor_ = nullptr;
		std::weak_ptr<SceneObject> selectedObject_;
		SceneObjectEditor*         sceneObjectEditor_ = nullptr;

		int                       currentTabIndex_ = 0;
		std::vector<InspectorTab> tabs_;
		std::vector<InspectorTab> allTabs_;

		std::string rootPath_  = "UI/Tool/Inspector/";
	};

}