#include "PatternAimedNWay.h"

#include <Engine/Foundation/Utility/Func/CxUtils.h>

#include "PatternCommon.h"

// 生成
void PatternAimedNWay::Generate(const PatternInput& in, PatternOutput& out) {
	out.dirsN.clear();
	if (nWay <= 0) return;

	// 基準 forward（安全側フォールバック）
	Vector3 forward = in.baseDirN;
	if (forward.LengthSquared() < 1e-8f) {
		forward = {0,0,1};
	} else {
		forward = forward.Normalize();
	}

	// 直交基底
	Vector3 rightN, upN;
	Cx::GameUtil::MakeOrthoBasis(forward, rightN, upN);

	const float spread = Cx::Math::ToRadians(spreadDeg);
	const float center = Cx::Math::ToRadians(centerDeg);

	out.dirsN.reserve(static_cast<size_t>((std::max)(0, nWay)));
	for (int i = 0; i < nWay; ++i) {
		// [0,1] の等間隔サンプル（nWay==1 のときは中央に1本）
		const float t   = (nWay == 1) ? 0.5f : static_cast<float>(i) / static_cast<float>(nWay - 1);
		const float ang = (t - 0.5f) * spread + center; // 左(-spread/2) → 右(+spread/2)

		// 扇：上下は固定し、right 方向にのみ振る（必要なら upN も混ぜて3Dコーンに拡張可）
		Vector3 dir = forward * std::cos(ang) + rightN * std::sin(ang);
		out.dirsN.push_back(dir.Normalize());
	}
}