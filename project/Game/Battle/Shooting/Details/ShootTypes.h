#pragma once

//< engine
#include <Engine/Foundation/Math/Vector3.h>

//< c++
#include <vector>
#include <string>

struct  ShootRequest{
	Vector3 origin{};
	Vector3 dirN{};
	float speed;
	std::string tag;
};

struct ShootBatch{
	std::vector<ShootRequest> shots;
};