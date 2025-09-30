#include "PatternCommon.h"

#include <Engine/Foundation/Utility/Func/CxUtils.h>

namespace Cx {
	namespace GameUtil {

		
		Vector3 RotateAroundAxis(const Vector3& v, const Vector3& axisN, float rad) {
			float c = std::cos(rad), s = std::sin(rad);
			return v * c + Vector3::Cross(axisN, v) * s + axisN * (Vector3::Dot(axisN, v) * (1.0f - c));
		}

		void MakeOrthoBasis(const Vector3& forwardN, Vector3& rightN, Vector3& upN){
			const Vector3 worldUp = {0,1,0};
			rightN = Vector3::Cross(worldUp, forwardN);
			if (rightN.LengthSquared() < 1e-8f){
				rightN = Vector3::Cross({1,0,0}, forwardN);
			}
			rightN = rightN.Normalize();
			upN    = Vector3::Cross(forwardN, rightN).Normalize();
		}

		std::vector<Vector3> GenFanDirs(const Vector3& forwardN, float centerDeg, float spreadDeg, int nWay) {
			std::vector<Vector3> out;
			if (nWay <= 0) return out;

			Vector3 rightN, upN;
			MakeOrthoBasis(forwardN, rightN, upN);

			float center = Cx::Math::ToRadians(centerDeg);
			float spread = Cx::Math::ToRadians(spreadDeg);

			for (int i=0; i<nWay; ++i){
				float t = (nWay==1) ? 0.5f : (float)i/(nWay-1);
				float ang = (t-0.5f)*spread + center;
				Vector3 d = (forwardN*std::cos(ang)) + (rightN*std::sin(ang));
				out.push_back(d.Normalize());
			}
			return out;
		}
	}
}

