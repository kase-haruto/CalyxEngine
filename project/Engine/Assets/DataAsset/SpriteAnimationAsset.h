#pragma once

#include "DataAsset.h"

#include <Engine\Foundation\Math\Vector2.h>

#include <cstdint>
#include <string>
#include <vector>

namespace CalyxEngine {

	struct SpriteAnimationClip {
		std::string name = "New Clip";
		int32_t startFrame = 0;
		int32_t frameCount = 1;
		float frameDuration = 0.1f;
		bool loop = true;
	};

	class SpriteAnimationAsset : public DataAsset {
	public:
		SpriteAnimationAsset();
		~SpriteAnimationAsset() override = default;

		std::string GetAssetTypeName() const override { return "SpriteAnimationAsset"; }

		Guid textureGuid;
		std::string texturePath;
		Vector2 division = {1.0f, 1.0f};
		std::vector<SpriteAnimationClip> clips;

		const SpriteAnimationClip* FindClip(const std::string& name) const;
		int32_t GetDivisionX() const;
		int32_t GetDivisionY() const;
		int32_t GetFrameCapacity() const;

	private:
		void RegisterFields();
	};

} // namespace CalyxEngine
