#pragma once
#include <Engine/Foundation/Math/Vector4.h>
#include <Engine/Graphics/RenderTarget/Detail/RenderTargetDetail.h>
#include <Engine/Objects/2D/Sprite/SpriteAsset.h>
#include <Engine/System/Component/IComponent.h>

namespace Calyx2D {
	
	class SpriteComponent final
		: public IComponent {
	public:
		//===================================================================*/
		//			public methods
		//===================================================================*/
		SpriteAsset*	   asset = nullptr;				//< スプライトアセット
		CalyxMath::Vector4 color{1, 1, 1, 1};	//< 色
		float			   fillAmount = 1.0f;			//< 塗りつぶし量
		bool			   visible	  = true;			//< 表示フラグ

		RenderTargetType targetRT = RenderTargetType::BackBuffer;
	};

} // namespace Calyx2D
