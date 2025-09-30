#pragma once

#include "ShootTypes.h"

struct IShootSink{
	virtual ~IShootSink() = default;
	virtual void Submit(const ShootBatch& batch) = 0;
};