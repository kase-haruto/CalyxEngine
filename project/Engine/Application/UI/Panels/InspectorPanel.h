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

	/**
	 * 選択されたオブジェクト・エディタのプロパティ調整パネル
	 */
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
		BaseEditor*				   selectedEditor_ = nullptr;
		std::weak_ptr<SceneObject> selectedObject_;
		SceneObjectEditor*		   sceneObjectEditor_ = nullptr;
	};

}
