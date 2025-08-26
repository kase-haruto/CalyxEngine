#pragma once
#include <Engine/Foundation/Math/Vector3.h>
#include <vector>
#include <algorithm>

struct SplinePoint {
	Vector3 pos{};
};

class SplineData {
public:
	// 基本データ
	std::vector<SplinePoint> points;
	bool closed = false;

	// 補間API（Catmull–Rom）
	int  SegmentCount() const { int n = (int)points.size(); return closed ? n : (std::max)(0, n - 1); }
	Vector3 Evaluate(float t) const;

	// 編集ユーティリティ
	void InsertPoint(int index, const Vector3& p) {
		index = std::clamp(index, 0, (int)points.size());
		points.insert(points.begin() + index, SplinePoint{ p });
	}
	void RemovePoint(int index) {
		if (index >= 0 && index < (int)points.size()) points.erase(points.begin() + index);
	}
};
