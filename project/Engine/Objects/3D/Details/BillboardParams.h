#pragma once
#include <cstdint>

enum class BillboardMode : uint32_t {
	None  = 0,
	Full  = 1,
	AxisY = 2,
};

struct GpuBillboardParams {
	uint32_t mode; // 0=None, 1=Full, 2=AxisY
	float	 pad[3];
};
static_assert(sizeof(GpuBillboardParams) == 16, "GpuBillboardParams must be 16 bytes");

struct GpuFadeParams {
	float fadeNear = 0.0f;
	float fadeFar  = 20.0f;
	float pad[2];
};
static_assert(sizeof(GpuFadeParams) == 16, "GpuFadeParams must be 16 bytes");
