#pragma once
/* ========================================================================
/*      include space
/* ===================================================================== */
#include <Engine/Objects/Event/BaseEventObject.h>

/*-----------------------------------------------------------------------------------------
 * TutorialEvent
 * - チュートリアルイベントの基本クラス
 * - チュートリアルイベントの共通インターフェースや基本機能を提供
 * - 具体的なチュートリアルイベントはこのクラスを継承して実装する
 *---------------------------------------------------------------------------------------*/
class TutorialEvent final
	: public BaseEventObject {
public:
	//===================================================================*/
	//			public methods
	//===================================================================*/
	TutorialEvent();
	TutorialEvent(const std::string& name);
	~TutorialEvent() override;

	void Initialize() override;
	void AlwaysUpdate(float dt) override;

	// gui
	void ShowGui() override;

	// collision
	void OnCollisionEnter(Collider* other) override;
	void OnCollisionStay(Collider* other) override;
	void OnCollisionExit(Collider* other) override;

	std::string_view GetTypeName() const override{return "TutorialEvent";};

private:
	//===================================================================*/
	//			private methods
	//===================================================================*/
	std::unique_ptr<Collider> collider_;
};
