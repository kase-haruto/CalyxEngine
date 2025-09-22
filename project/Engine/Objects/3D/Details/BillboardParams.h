#pragma once
#include <cstdint>

enum class BillboardMode : uint32_t {
	None  = 0,
	Full  = 1,
	AxisY = 2,
};

struct GpuBillboardParams {
	uint32_t mode = static_cast<uint32_t>(BillboardMode::None);
	uint32_t pad[3] = {0,0,0};
};
static_assert(sizeof(GpuBillboardParams) == 16, "Billboard params must be 16 bytes.");
