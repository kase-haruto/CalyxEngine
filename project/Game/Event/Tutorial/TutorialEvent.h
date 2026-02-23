#pragma once
/* ========================================================================
/*      include space
/* ===================================================================== */
#include "Engine/Objects/2D/Hud/BaseHud.h"

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
	enum class State {
		None,
		LockOnPhase,
		AttackPhase,
		WaitingForDeath,
		Complete
	};

	//===================================================================*/
	//			public methods
	//===================================================================*/
	TutorialEvent();
	TutorialEvent(const std::string& name);
	~TutorialEvent() override;

	void Initialize() override;
	void AlwaysUpdate(float dt) override;
	void DrawUISprite(class SpriteRenderer* spriteRenderer) const;

	// gui
	void ShowGui() override;

	// collision
	void OnCollisionEnter(Collider* other) override;
	void OnCollisionStay(Collider* other) override;
	void OnCollisionExit(Collider* other) override;

	std::string_view GetTypeName() const override { return "TutorialEvent"; };

private:
	//===================================================================*/
	//			private methods
	//===================================================================*/
	void ShowTutorialMsg();
	void ShowDebugState(size_t enemyCount);
	void UpdateTimeScaleEasing(float alwaysDt);

	State state_		  = State::None;
	bool  isPlayerInside_ = false;

	class Player* player_ = nullptr;

	// タイムスケールイージング用
	float startTimeScale_		 = 1.0f;
	float targetTimeScale_		 = 1.0f;
	float currentTimeScale_		 = 1.0f;
	float timeScaleEaseTimer_	 = 0.0f;
	float timeScaleEaseDuration_ = 0.5f;

	std::unique_ptr<Calyx2D::SpriteObject2d> aimTutorialSprite_;
};
