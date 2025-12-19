#include "PatternCommon.h"

#include <Engine/Foundation/Utility/Func/CxUtils.h>

namespace Cx {
	namespace GameUtil {

		
		CxMath::Vector3 RotateAroundAxis(const CxMath::Vector3& v, const CxMath::Vector3& axisN, float rad) {
			float c = std::cos(rad), s = std::sin(rad);
			return v * c + CxMath::Vector3::Cross(axisN, v) * s + axisN * (CxMath::Vector3::Dot(axisN, v) * (1.0f - c));
		}

		void MakeOrthoBasis(const CxMath::Vector3& forwardN, CxMath::Vector3& rightN, CxMath::Vector3& upN){
			const CxMath::Vector3 worldUp = {0,1,0};
			rightN = CxMath::Vector3::Cross(worldUp, forwardN);
			if (rightN.LengthSquared() < 1e-8f){
				rightN = CxMath::Vector3::Cross({1,0,0}, forwardN);
			}
			rightN = rightN.Normalize();
			upN    = CxMath::Vector3::Cross(forwardN, rightN).Normalize();
		}

		std::vector<CxMath::Vector3> GenFanDirs(const CxMath::Vector3& forwardN, float centerDeg, float spreadDeg, int nWay) {
			std::vector<CxMath::Vector3> out;
			if (nWay <= 0) return out;

			CxMath::Vector3 rightN, upN;
			MakeOrthoBasis(forwardN, rightN, upN);

			float center = CxMath::ToRadians(centerDeg);
			float spread = CxMath::ToRadians(spreadDeg);

			for (int i=0; i<nWay; ++i){
				float t = (nWay==1) ? 0.5f : (float)i/(nWay-1);
				float ang = (t-0.5f)*spread + center;
				CxMath::Vector3 d = (forwardN*std::cos(ang)) + (rightN*std::sin(ang));
				out.push_back(d.Normalize());
			}
			return out;
		}
	}
}

