#include "PatternSpiral.h"

#include <Engine/Foundation/Utility/Func/CxUtils.h>
#include "PatternCommon.h"

void PatternSpiral::Advance(float ) {}

void PatternSpiral::Generate(const PatternInput& in, PatternOutput& out){
	out.dirsN.clear();

	CalyxMath::Vector3 r, u; Cx::GameUtil::MakeOrthoBasis(in.baseDirN, r, u);

	const float a   = CalyxMath::ToRadians(angleDeg);
	const float tip = CalyxMath::ToRadians(tipDeg);

	// リング上の接線方向（r/u 平面）
	CalyxMath::Vector3 ring = r * std::cos(a) + u * std::sin(a);
	// すこし前に倒す
	CalyxMath::Vector3 d = in.baseDirN * std::cos(tip) + ring * std::sin(tip);

	out.dirsN.push_back(d.Normalize());
}
