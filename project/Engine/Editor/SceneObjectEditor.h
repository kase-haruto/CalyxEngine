#pragma once
/* ========================================================================
/*  include space
/* ===================================================================== */
#include <Engine/Editor/BaseEditor.h>
#include <Engine/Application/UI/EngineUI/Manipulator.h>

#include <memory>

class SceneObject;
class SceneContext;
namespace CalyxEditor {

	/*-----------------------------------------------------------------------------------------
	 * SceneObjectEditor
	 * - シーンオブジェクト編集クラス
	 * - 選択されたSceneObjectのトランスフォーム編集・ギズモ操作を担当
	 *---------------------------------------------------------------------------------------*/
	class SceneObjectEditor 
		: public BaseEditor {
	public:
		//===================================================================*/
		//                   public functions
		//===================================================================*/
		SceneObjectEditor(const std::string& name);
		SceneObjectEditor();
		~SceneObjectEditor() override = default;

		void Update();
		void ShowImGuiInterface() override;
		void SetSceneObject(SceneObject* sceneObject) { sceneObject_ = sceneObject; }
		void BindRemovalCallback(SceneContext* ctx);
		void ClearSelection(); // ← 明示クリアも使える

		void					  SetTarget(SceneObject* object);
		Manipulator* GetManipulator() const { return manipulator_.get(); }

	private:
		//===================================================================*/
		//                   private variables
		//===================================================================*/
		std::unique_ptr<Manipulator> manipulator_ = nullptr; // マニピュレーター
		SceneObject*							  sceneObject_ = nullptr; // 編集対象のSceneObject
	};

}
