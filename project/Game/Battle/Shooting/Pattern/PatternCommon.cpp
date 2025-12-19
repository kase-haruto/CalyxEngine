#include "PatternCommon.h"

#include <Engine/Foundation/Utility/Func/CxUtils.h>

namespace Cx {
	namespace GameUtil {

		
		CalyxMath::Vector3 RotateAroundAxis(const CalyxMath::Vector3& v, const CalyxMath::Vector3& axisN, float rad) {
			float c = std::cos(rad), s = std::sin(rad);
			return v * c + CalyxMath::Vector3::Cross(axisN, v) * s + axisN * (CalyxMath::Vector3::Dot(axisN, v) * (1.0f - c));
		}

		void MakeOrthoBasis(const CalyxMath::Vector3& forwardN, CalyxMath::Vector3& rightN, CalyxMath::Vector3& upN){
			const CalyxMath::Vector3 worldUp = {0,1,0};
			rightN = CalyxMath::Vector3::Cross(worldUp, forwardN);
			if (rightN.LengthSquared() < 1e-8f){
				rightN = CalyxMath::Vector3::Cross({1,0,0}, forwardN);
			}
			rightN = rightN.Normalize();
			upN    = CalyxMath::Vector3::Cross(forwardN, rightN).Normalize();
		}

		std::vector<CalyxMath::Vector3> GenFanDirs(const CalyxMath::Vector3& forwardN, float centerDeg, float spreadDeg, int nWay) {
			std::vector<CalyxMath::Vector3> out;
			if (nWay <= 0) return out;

			CalyxMath::Vector3 rightN, upN;
			MakeOrthoBasis(forwardN, rightN, upN);

			float center = CalyxMath::ToRadians(centerDeg);
			float spread = CalyxMath::ToRadians(spreadDeg);

			for (int i=0; i<nWay; ++i){
				float t = (nWay==1) ? 0.5f : (float)i/(nWay-1);
				float ang = (t-0.5f)*spread + center;
				CalyxMath::Vector3 d = (forwardN*std::cos(ang)) + (rightN*std::sin(ang));
				out.push_back(d.Normalize());
			}
			return out;
		}
	}
}

