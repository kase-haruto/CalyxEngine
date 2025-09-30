#include "PatternCircleRing.h"

// void PatternCircleRing::Generate(const PatternInput& in, PatternOutput& out){
// 	out.dirsN.clear();
// 	if (count <= 0) return;
//
// 	Vector3 r, u; MakeOrthoBasis(in.baseDirN, r, u);
// 	for (int i = 0; i < count; ++i){
// 		const float t   = (float)i / count;
// 		const float ang = t * 2.0f * 3.14159265358979323846f;
// 		Vector3 d = r * std::cos(ang) + u * std::sin(ang);
// 		out.dirsN.push_back(d.Normalize());
// 	}
// }