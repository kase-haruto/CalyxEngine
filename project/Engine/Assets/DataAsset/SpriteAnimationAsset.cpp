#include "SpriteAnimationAsset.h"

#include <algorithm>

namespace CalyxEngine {

	SpriteAnimationAsset::SpriteAnimationAsset() {
		name_ = "New Sprite Animation";
		clips.push_back({});
		RegisterFields();
	}

	void SpriteAnimationAsset::RegisterFields() {
		AddField("textureGuid", textureGuid);
		AddField("division", division);
	}

	const SpriteAnimationClip* SpriteAnimationAsset::FindClip(const std::string& name) const {
		auto it = std::find_if(clips.begin(), clips.end(), [&name](const SpriteAnimationClip& clip) {
			return clip.name == name;
		});
		return it == clips.end() ? nullptr : &*it;
	}

	int32_t SpriteAnimationAsset::GetDivisionX() const {
		return std::max(1, static_cast<int32_t>(division.x));
	}

	int32_t SpriteAnimationAsset::GetDivisionY() const {
		return std::max(1, static_cast<int32_t>(division.y));
	}

	int32_t SpriteAnimationAsset::GetFrameCapacity() const {
		return GetDivisionX() * GetDivisionY();
	}

} // namespace CalyxEngine
