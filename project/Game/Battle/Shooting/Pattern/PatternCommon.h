#pragma once

#include <Engine/Foundation/Math/Vector3.h>

namespace Cx {
	namespace GameUtil {
		CxMath::Vector3 RotateAroundAxis(const CxMath::Vector3& v, const CxMath::Vector3& axisN, float rad);
		void MakeOrthoBasis(const CxMath::Vector3& forwardN, CxMath::Vector3& rightN, CxMath::Vector3& upN);
		std::vector<CxMath::Vector3> GenFanDirs( const CxMath::Vector3& forwardN, float centerDeg, float spreadDeg, int nWay);
	}
}
