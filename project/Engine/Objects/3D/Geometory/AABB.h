#pragma once 

/* engine */
#include <Engine/Foundation/Math/Vector3.h>

/* c++ */
#include<stdint.h>
#include<string>

struct CalyxEngine::Vector3;

/* ========================================================================
/*		aabb
/* ===================================================================== */
class AABB{
public:
	AABB(const CalyxEngine::Vector3& min, const CalyxEngine::Vector3& max, uint32_t color = 0xFFFFFFFF);
	AABB() = default;
	~AABB() = default;

	void Initialize(const CalyxEngine::Vector3& min, const CalyxEngine::Vector3& max);
	void Update();
	void UpdateUI(std::string lavel);

	//--------- accessor -----------------------------------------------------
	CalyxEngine::Vector3 GetMin()const;
	CalyxEngine::Vector3 GetMax()const;
	CalyxEngine::Vector3 GetCenter() const;
	AABB Transform(const CalyxEngine::Matrix4x4& mat) const;

public:
	CalyxEngine::Vector3 min_;
	CalyxEngine::Vector3 max_;
	uint32_t color;
};

