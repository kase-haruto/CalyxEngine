#pragma once

//< engine
#include <Engine/Foundation/Math/Vector3.h>

//< c++
#include <vector>
#include <string>

struct  ShootRequest{
	CalyxMath::Vector3 origin{};
	CalyxMath::Vector3 dirN{};
	float speed;
	std::string tag;
};

struct ShootBatch{
	std::vector<ShootRequest> shots;
};