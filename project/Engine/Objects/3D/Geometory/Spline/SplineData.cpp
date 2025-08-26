#include "SplineData.h"
#include <cmath>

static inline Vector3 Catmull(const Vector3& p0, const Vector3& p1,
							  const Vector3& p2, const Vector3& p3, float t) {
	float t2 = t * t, t3 = t2 * t;
	return 0.5f * ((2.0f * p1) +
				   (p0 * -1 + p2) * t +
				   (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2 +
				   (p0 * -1 + 3.0f * p1 - 3.0f * p2 + p3) * t3);
}

static inline int Wrap(int i, int n) { int r = i % n; return r < 0 ? r + n : r; }

Vector3 SplineData::Evaluate(float t) const {
	if (points.empty())  return {};
	if (points.size() == 1) return points[0].pos;

	int segs = SegmentCount();
	if (segs <= 0) return points.back().pos;

	t = std::clamp(t, 0.0f, 1.0f);
	float f = t * segs;
	int   i = std::min(segs - 1, (int)std::floor(f));
	float lt = f - i; // local t

	auto idx = [&](int k) {
		int n = (int)points.size();
		return closed ? Wrap(k, n) : std::clamp(k, 0, n - 1);
	};

	int i0 = idx(i - 1), i1 = idx(i), i2 = idx(i + 1), i3 = idx(i + 2);
	return Catmull(points[i0].pos, points[i1].pos, points[i2].pos, points[i3].pos, lt);
}
