#pragma once

//< engine
#include <Engine/Foundation/Math/Vector3.h>

//< c++
#include <vector>
#include <string>

struct  ShootRequest{
	CxMath::Vector3 origin{};
	CxMath::Vector3 dirN{};
	float speed;
	std::string tag;
};

struct ShootBatch{
	std::vector<ShootRequest> shots;
};