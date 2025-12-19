#pragma once 

/* engine */
#include <Engine/Foundation/Math/Vector3.h>

/* c++ */
#include<stdint.h>
#include<string>

struct CxMath::Vector3;

/* ========================================================================
/*		aabb
/* ===================================================================== */
class AABB{
public:
	AABB(const CxMath::Vector3& min, const CxMath::Vector3& max, uint32_t color = 0xFFFFFFFF);
	AABB() = default;
	~AABB() = default;

	void Initialize(const CxMath::Vector3& min, const CxMath::Vector3& max);
	void Update();
	void UpdateUI(std::string lavel);

	//--------- accessor -----------------------------------------------------
	CxMath::Vector3 GetMin()const;
	CxMath::Vector3 GetMax()const;
	CxMath::Vector3 GetCenter() const;
	AABB Transform(const CxMath::Matrix4x4& mat) const;

public:
	CxMath::Vector3 min_;
	CxMath::Vector3 max_;
	uint32_t color;
};

