#include "AABB.h"


/* external */
#include "externals/imgui/imgui.h"
/* c++ */
#include <cmath>


AABB::AABB(const CalyxMath::Vector3& min, const CalyxMath::Vector3& max, uint32_t color)
	: min_(min), max_(max), color(color) {}

void AABB::Initialize(const CalyxMath::Vector3& Min, const CalyxMath::Vector3& Max) {
	min_ = Min;
	max_ = Max;
}

void AABB::Update() {
	//minとmaxが入れ替わらないようにする
	min_.x = (std::min)(min_.x, max_.x);
	max_.x = (std::max)(min_.x, max_.x);

	min_.y = (std::min)(min_.y, max_.y);
	max_.y = (std::max)(min_.y, max_.y);

	min_.z = (std::min)(min_.z, max_.z);
	max_.z = (std::max)(min_.z, max_.z);
}

void AABB::UpdateUI(std::string lavel) {
	ImGui::Begin(lavel.c_str());
	ImGui::DragFloat3("min", &min_.x, 0.01f);
	ImGui::DragFloat3("max", &max_.x, 0.01f);
	ImGui::End();
}

CalyxMath::Vector3 AABB::GetMin()const { return min_; }
CalyxMath::Vector3 AABB::GetMax()const { return max_; }

CalyxMath::Vector3 AABB::GetCenter() const {
	return (min_ + max_) * 0.5f;
}

AABB AABB::Transform(const CalyxMath::Matrix4x4& mat) const{
	// AABBの8頂点を作成
	CalyxMath::Vector3 corners[8] = {
		{min_.x, min_.y, min_.z},
		{max_.x, min_.y, min_.z},
		{min_.x, max_.y, min_.z},
		{max_.x, max_.y, min_.z},
		{min_.x, min_.y, max_.z},
		{max_.x, min_.y, max_.z},
		{min_.x, max_.y, max_.z},
		{max_.x, max_.y, max_.z},
	};

	CalyxMath::Vector3 newMin(FLT_MAX, FLT_MAX, FLT_MAX);
	CalyxMath::Vector3 newMax(-FLT_MAX, -FLT_MAX, -FLT_MAX);

	for (int i = 0; i < 8; ++i){
		CalyxMath::Vector3 transformed = CalyxMath::Vector3::Transform(corners[i], mat);
		newMin = CalyxMath::Vector3::Min(newMin, transformed);
		newMax = CalyxMath::Vector3::Max(newMax, transformed);
	}

	return AABB(newMin, newMax, color);
}