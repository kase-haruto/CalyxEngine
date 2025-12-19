#pragma once

#include <Engine/Foundation/Math/Vector3.h>

namespace Cx {
	namespace GameUtil {
		CalyxMath::Vector3 RotateAroundAxis(const CalyxMath::Vector3& v, const CalyxMath::Vector3& axisN, float rad);
		void MakeOrthoBasis(const CalyxMath::Vector3& forwardN, CalyxMath::Vector3& rightN, CalyxMath::Vector3& upN);
		std::vector<CalyxMath::Vector3> GenFanDirs( const CalyxMath::Vector3& forwardN, float centerDeg, float spreadDeg, int nWay);
	}
}
