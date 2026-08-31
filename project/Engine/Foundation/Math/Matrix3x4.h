#pragma once

namespace CalyxEngine {
	struct Matrix4x4;

	/*-----------------------------------------------------------
	 *	Matrix3x4
	 *	- 3x4行列
	 *----------------------------------------------------------*/
	/**
	 * @brief Matrix3x4に関するデータを保持する構造体です。
	 */
	struct Matrix3x4 final {
		float m[3][4];

		static CalyxEngine::Matrix3x4 ToMatrix3x4(const CalyxEngine::Matrix4x4& m) ;
	};

}
