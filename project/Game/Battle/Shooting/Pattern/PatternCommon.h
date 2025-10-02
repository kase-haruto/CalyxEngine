#pragma once

#include <Engine/Foundation/Math/Vector3.h>

namespace Cx {
	namespace GameUtil {
		Vector3 RotateAroundAxis(const Vector3& v, const Vector3& axisN, float rad);
		void MakeOrthoBasis(const Vector3& forwardN, Vector3& rightN, Vector3& upN);
		std::vector<Vector3> GenFanDirs( const Vector3& forwardN, float centerDeg, float spreadDeg, int nWay);
	}
}
