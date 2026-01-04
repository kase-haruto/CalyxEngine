#pragma once
#include <string>
#include <memory>
#include <vector>
#include <Engine/PostProcess/Interface/IPostEffectPass.h>

struct PostEffectSlot {
	std::string name;
	bool enabled = true;
	IPostEffectPass* pass = nullptr;
};
