#pragma once

#include "Engine/Foundation/Serialization/SerializableObject.h"
#include "Engine/Foundation/Math/Vector2.h"
#include "Engine/Foundation/Utility/Ease/CxEase.h"
#include <cstdint>

namespace Calyx2D {

	/* ----------------------------------------------------
	 * HudTransformMotionConfig
	 * - HUDの共通モーション設定（保存可能：プリミティブのみ）
	 * - すべてのHUD Configの基底として使う想定
	 * ---------------------------------------------------*/
	class HudTransformMotionConfig : public CalyxEngine::SerializableObject {
	public:
		HudTransformMotionConfig();

		//=========================================================
		// Position (Vector2)
		//=========================================================
		bool               posEnabled  = true;
		CalyxMath::Vector2 posStart    = {};
		CalyxMath::Vector2 posEnd      = {};
		float              posDuration = 0.5f;
		int32_t            posEaseInt  = static_cast<int32_t>(CalyxEase::EaseType::EaseOutSine);

		//=========================================================
		// Scale (Vector2)
		//=========================================================
		bool               scaleEnabled  = false;
		CalyxMath::Vector2 scaleStart    = {1.0f,1.0f};
		CalyxMath::Vector2 scaleEnd      = {1.0f,1.0f};
		float              scaleDuration = 0.5f;
		int32_t            scaleEaseInt  = static_cast<int32_t>(CalyxEase::EaseType::Linear);

		//=========================================================
		// Rotation (float)
		//=========================================================
		bool    rotEnabled  = false;
		float   rotStart    = 0.0f;
		float   rotEnd      = 0.0f;
		float   rotDuration = 0.5f;
		int32_t rotEaseInt  = static_cast<int32_t>(CalyxEase::EaseType::Linear);

		//=========================================================
		// Alpha (float)
		//=========================================================
		bool    alphaEnabled  = false;
		float   alphaStart    = 1.0f;
		float   alphaEnd      = 1.0f;
		float   alphaDuration = 0.5f;
		int32_t alphaEaseInt  = static_cast<int32_t>(CalyxEase::EaseType::Linear);

		// NOTE:
		// GetParamPath() は派生クラスで実装する（HUDごとに保存先を分ける）
	};

} // namespace Calyx2D