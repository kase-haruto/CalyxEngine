#pragma once
#include <Engine/Foundation/Math/Vector3.h>
#include <algorithm>
#include <numeric>
#include <vector>


struct SplinePoint {
	CalyxMath::Vector3 pos{};
};

class SplineData {
public:
	// 基本データ
	std::vector<SplinePoint> points;
	bool					 closed = false;

	// 事前計算（弧長LUT）
	void BuildArcTable(int samplesPerSegment = 32);

	// 補間API（Catmull–Rom）
	int SegmentCount() const {
		int n = (int)points.size();
		return closed ? n : (std::max)(0, n - 1);
	}
	CalyxMath::Vector3 Evaluate(float t) const; // 0..1 の正規化パラメータ
	CalyxMath::Vector3 Tangent(float t) const;	// 進行方向ベクトル（正規化推奨）

	// 弧長 → t 逆写像（距離で進める用）
	float DistanceToT(float distance) const;		 // 0..totalLength_ の距離を 0..1 の t に
	float AdvanceTBy(float t, float distance) const; // 今の t からdistanceだけ前進して新しい t を返す

	/**
	 * \brief 指定座標から最も近いスプライン上の距離（弧長）を取得
	 * \param worldPos 検索対象の座標
	 * \return 弧長（0〜TotalLength）
	 */
	float FindNearestDistance(const CalyxMath::Vector3& worldPos) const;

	// 編集ユーティリティ
	void InsertPoint(int index, const CalyxMath::Vector3& p) {
		index = std::clamp(index, 0, (int)points.size());
		points.insert(points.begin() + index, SplinePoint{p});
	}
	void RemovePoint(int index) {
		if(index >= 0 && index < (int)points.size()) points.erase(points.begin() + index);
	}

	float TotalLength() const { return totalLength_; }

private:
	// 内部ユーティリティ
	inline int				  CountP() const { return (int)points.size(); }
	int						  WrapIndex(int i) const;
	void					  SegmentAndLocalT(float t, int& seg, float& lt) const;
	static CalyxMath::Vector3 CatmullRom(const CalyxMath::Vector3& p0, const CalyxMath::Vector3& p1, const CalyxMath::Vector3& p2, const CalyxMath::Vector3& p3, float t);
	static CalyxMath::Vector3 CatmullRomTangent(const CalyxMath::Vector3& p0, const CalyxMath::Vector3& p1, const CalyxMath::Vector3& p2, const CalyxMath::Vector3& p3, float t);

private:
	// 弧長LUT（正規化 t に対する累積距離）
	// samples_.size() = segments * samplesPerSeg + 1 （最後は 1.0）
	std::vector<float> tSamples_;
	std::vector<float> sCumulative_;
	float			   totalLength_		  = 0.0f;
	int				   samplesPerSegment_ = 0;
};