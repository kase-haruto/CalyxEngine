#pragma once

namespace CalyxMath {
	struct Matrix4x4;

	/*-----------------------------------------------------------
	 *	Matrix3x4
	 *	- 3x4行列
	 *----------------------------------------------------------*/
	struct Matrix3x4 final {
		float m[3][4];

		static CalyxMath::Matrix3x4 ToMatrix3x4(const CalyxMath::Matrix4x4& m) ;
	};

}