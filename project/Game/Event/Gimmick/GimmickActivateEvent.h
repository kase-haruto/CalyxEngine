#pragma once

/// ===================================================================== */
///  include space
/// ===================================================================== */
// stl
#include <string>
// engine
#include <Engine/Objects/Event/BaseEventObject.h>
#include <Engine/Foundation/Serialization/SerializableObject.h>

/*-----------------------------------------------------------------------------------------
 * GimmickActivateEvent
 * - ギミック起動イベントのクラス
 * - ギミックの起動に関連する情報や処理を管理する
 *---------------------------------------------------------------------------------------*/
class GimmickActivateEvent final
	: public BaseEventObject {
public:
	//====================================================================*/
	//			public methods
	//====================================================================*/
	GimmickActivateEvent();
	GimmickActivateEvent(const std::string& name);
	~GimmickActivateEvent() override;

	void ShowGui() override;

	// collision
	void OnCollisionEnter(Collider* other) override;

	std::string_view GetTypeName() const override { return "GimmickActivateEvent"; };

private:
	//===================================================================*/
	//			private methods
	//===================================================================*/
	struct GimmickActivateEventData:
		public CalyxEngine::SerializableObject {
		GimmickActivateEventData();
		CalyxEngine::ParamPath GetParamPath() const override;

		// param
		CalyxMath::Vector4 color = {1.0f, 0.0f, 0.0f, 0.5f};
	}param_;
};
