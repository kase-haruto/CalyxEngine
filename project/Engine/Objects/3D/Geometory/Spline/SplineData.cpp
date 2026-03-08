#include "SplineData.h"
#include <cmath>

int SplineData::WrapIndex(int i) const {
	int n = CountP();
	if(closed) {
		i %= n;
		if(i < 0) i += n;
		return i;
	} else {
		return std::clamp(i, 0, n - 1);
	}
}

void SplineData::SegmentAndLocalT(float t, int& seg, float& lt) const {
	const int segCount = SegmentCount();
	if(segCount <= 0) {
		seg = 0;
		lt	= 0.0f;
		return;
	}
	// t を [0,1] にクランプ（closed でも 0..1 のループにしておく）
	if(closed) {
		t = std::fmod(std::fmod(t, 1.0f) + 1.0f, 1.0f);
	} else {
		t = std::clamp(t, 0.0f, 1.0f);
	}

	float ft = t * segCount;
	seg		 = (int)std::floor(ft);
	if(seg >= segCount) {
		seg = segCount - 1;
	}
	lt = ft - seg; // 0..1
}

CalyxMath::Vector3 SplineData::CatmullRom(const CalyxMath::Vector3& p0, const CalyxMath::Vector3& p1, const CalyxMath::Vector3& p2, const CalyxMath::Vector3& p3, float t) {
	// 標準 Catmull–Rom (centripetal ではなくuniform)
	float t2 = t * t;
	float t3 = t2 * t;
	return 0.5f * ((2.0f * p1) +
				   (-p0 + p2) * t +
				   (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2 +
				   (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3);
}
CalyxMath::Vector3 SplineData::CatmullRomTangent(const CalyxMath::Vector3& p0, const CalyxMath::Vector3& p1, const CalyxMath::Vector3& p2, const CalyxMath::Vector3& p3, float t) {
	float t2 = t * t;
	// d/dt
	return 0.5f * ((-p0 + p2) +
				   2.0f * (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t +
				   3.0f * (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t2);
}

CalyxMath::Vector3 SplineData::Evaluate(float t) const {
	const int n = CountP();
	if(n == 0) return {};
	if(n == 1) return points[0].pos;

	int	  seg;
	float lt;
	SegmentAndLocalT(t, seg, lt);

	// seg の4点を取得
	// セグメント i は p[i] -> p[i+1]
	const int i1 = WrapIndex(seg);
	const int i2 = WrapIndex(seg + 1);
	const int i0 = WrapIndex(seg - 1);
	const int i3 = WrapIndex(seg + 2);

	return CatmullRom(points[i0].pos, points[i1].pos, points[i2].pos, points[i3].pos, lt);
}

CalyxMath::Vector3 SplineData::Tangent(float t) const {
	const int n = CountP();
	if(n <= 1) return CalyxMath::Vector3(0, 0, 1);

	int	  seg;
	float lt;
	SegmentAndLocalT(t, seg, lt);
	const int i1 = WrapIndex(seg);
	const int i2 = WrapIndex(seg + 1);
	const int i0 = WrapIndex(seg - 1);
	const int i3 = WrapIndex(seg + 2);

	CalyxMath::Vector3 tg = CatmullRomTangent(points[i0].pos, points[i1].pos, points[i2].pos, points[i3].pos, lt);
	if(tg.LengthSquared() > 1e-12f) tg = tg.Normalize();
	return tg;
}

void SplineData::BuildArcTable(int samplesPerSegment) {
	samplesPerSegment_ = std::max(4, samplesPerSegment);
	tSamples_.clear();
	sCumulative_.clear();
	totalLength_ = 0.0f;

	const int segCount = SegmentCount();
	if(segCount <= 0) return;

	const int totalSamples = segCount * samplesPerSegment_ + 1;
	tSamples_.reserve(totalSamples);
	sCumulative_.reserve(totalSamples);

	CalyxMath::Vector3 prev = Evaluate(0.0f);
	tSamples_.push_back(0.0f);
	sCumulative_.push_back(0.0f);

	for(int s = 1; s < totalSamples; ++s) {
		float			   t   = (float)s / (float)(totalSamples - 1);
		CalyxMath::Vector3 cur = Evaluate(t);
		totalLength_ += (cur - prev).Length();
		tSamples_.push_back(t);
		sCumulative_.push_back(totalLength_);
		prev = cur;
	}
}

float SplineData::DistanceToT(float distance) const {
	if(sCumulative_.empty() || totalLength_ <= 0.0f) return 0.0f;
	// 端を扱う
	float d = distance;
	// ループさせたい場合：
	d = std::fmod(d, totalLength_);
	if(d < 0.0f) d += totalLength_;
	// 非ループで止めたい場合は std::clamp(d, 0, totalLength_) にする

	// 二分探索で sCumulative_ から t を補間
	auto it = std::lower_bound(sCumulative_.begin(), sCumulative_.end(), d);
	if(it == sCumulative_.begin()) return tSamples_.front();
	if(it == sCumulative_.end()) return tSamples_.back();

	size_t idx1 = (size_t)std::distance(sCumulative_.begin(), it);
	size_t idx0 = idx1 - 1;
	float  s0	= sCumulative_[idx0];
	float  s1	= sCumulative_[idx1];
	float  u	= (d - s0) / (s1 - s0 + 1e-12f);

	float t0 = tSamples_[idx0];
	float t1 = tSamples_[idx1];
	return t0 + (t1 - t0) * u;
}

float SplineData::AdvanceTBy(float t, float distance) const {
	if(sCumulative_.empty() || totalLength_ <= 0.0f) return t;
	// 現在 t の距離 s を推定して distance だけ足して戻す
	// まず t を LUT 上で位置づけ
	float tClamped = t;
	if(!closed)
		tClamped = std::clamp(tClamped, 0.0f, 1.0f);
	else
		tClamped = std::fmod(std::fmod(tClamped, 1.0f) + 1.0f, 1.0f);

	auto   it  = std::lower_bound(tSamples_.begin(), tSamples_.end(), tClamped);
	size_t idx = (it == tSamples_.end()) ? (tSamples_.size() - 1) : (size_t)std::distance(tSamples_.begin(), it);
	if(idx == 0) idx = 1;
	// 線形に s を補間
	float t0 = tSamples_[idx - 1], t1 = tSamples_[idx];
	float s0 = sCumulative_[idx - 1], s1 = sCumulative_[idx];
	float u = (tClamped - t0) / (t1 - t0 + 1e-12f);
	float s = s0 + (s1 - s0) * u;

	// 前進
	float sNew = s + distance;

	// ループ扱い（閉じていなければ端で止めたいなら clamp に変更）
	if(closed) {
		sNew = std::fmod(sNew, totalLength_);
		if(sNew < 0.0f) sNew += totalLength_;
	} else {
		sNew = std::clamp(sNew, 0.0f, totalLength_);
	}

	return DistanceToT(sNew);
}

float SplineData::FindNearestDistance(const CalyxMath::Vector3& worldPos) const {
	if(sCumulative_.empty() || totalLength_ <= 0.0f) return 0.0f;

	float minD2	   = 1e30f;
	float nearestS = 0.0f;

	// サンプル点から最も近いものを探す（簡易実装）
	for(size_t i = 0; i < sCumulative_.size(); ++i) {
		CalyxMath::Vector3 p  = Evaluate(tSamples_[i]);
		float			   d2 = (p - worldPos).LengthSquared();
		if(d2 < minD2) {
			minD2	 = d2;
			nearestS = sCumulative_[i];
		}
	}

	return nearestS;
}