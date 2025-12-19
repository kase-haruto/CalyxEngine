#pragma once 

/* engine */
#include <Engine/Foundation/Math/Vector3.h>

/* c++ */
#include<stdint.h>
#include<string>

struct CalyxMath::Vector3;

/* ========================================================================
/*		aabb
/* ===================================================================== */
class AABB{
public:
	AABB(const CalyxMath::Vector3& min, const CalyxMath::Vector3& max, uint32_t color = 0xFFFFFFFF);
	AABB() = default;
	~AABB() = default;

	void Initialize(const CalyxMath::Vector3& min, const CalyxMath::Vector3& max);
	void Update();
	void UpdateUI(std::string lavel);

	//--------- accessor -----------------------------------------------------
	CalyxMath::Vector3 GetMin()const;
	CalyxMath::Vector3 GetMax()const;
	CalyxMath::Vector3 GetCenter() const;
	AABB Transform(const CalyxMath::Matrix4x4& mat) const;

public:
	CalyxMath::Vector3 min_;
	CalyxMath::Vector3 max_;
	uint32_t color;
};

