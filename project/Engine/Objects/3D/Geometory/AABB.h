#pragma once 

/* engine */
#include <Engine/Foundation/Math/Vector3.h>

/* c++ */
#include<stdint.h>
#include<string>

struct Vector3;

/* ========================================================================
/*		aabb
/* ===================================================================== */
class AABB{
public:
	AABB(const Vector3& min, const Vector3& max, uint32_t color = 0xFFFFFFFF);
	AABB() = default;
	~AABB() = default;

	void Initialize(const Vector3& min, const Vector3& max);
	void Update();
	void UpdateUI(std::string lavel);

	//--------- accessor -----------------------------------------------------
	Vector3 GetMin()const;
	Vector3 GetMax()const;
	Vector3 GetCenter() const;
	AABB Transform(const Matrix4x4& mat) const;

public:
	Vector3 min_;
	Vector3 max_;
	uint32_t color;
};

