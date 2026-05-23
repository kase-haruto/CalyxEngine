#pragma once
#include "Engine/Graphics/Camera/3d/Camera3d.h"
#include "Engine/Objects/3D/Geometory/AABB.h"

namespace CalyxEngine {

	/*--------------------------------------------------------------------------
	 * ShadowBounds
	 * -シャドウマップの範囲管理クラス
	 *-------------------------------------------------------------------------*/
	class ShadowBounds {
	public:
		//===================================================================*/
		//					public methods
		//===================================================================*/

		/**
		 * \brief カメラからシャドウ範囲を更新
		 * \param camera
		 * \param shadowFar
		 * \param expandMargin
		 */
		void UpdateFromCamera(
			const Camera3d& camera,
			float           shadowFar,
			float           expandMargin = 5.0f
			);

		const AABB& GetBounds() const { return bounds_; }

	private:
		//===================================================================*/
		//					private members
		//===================================================================*/
		AABB bounds_{}; //< 範囲aabb
	};


}