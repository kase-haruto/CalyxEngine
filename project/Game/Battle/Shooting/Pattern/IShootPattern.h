#pragma once

#include <Engine/Foundation/Math/Vector3.h>
#include <vector>

struct PatternInput {
	Vector3 baseDirN{};
};

struct PatternOutput {
	std::vector<Vector3> dirsN; // 発射方向（正規化）
};

struct IShootPattern {
	virtual ~IShootPattern() = default;
	virtual void Generate(const PatternInput& in, PatternOutput& out) = 0;
	virtual void Advance(float /*dt*/){}
};