#include "SimpleAnimator.h"
void CalyxEngine::SimpleAnimator::Reset() {
	// float
	for(auto& [_, ch] : floatAnims_) {
		ch->Reset();
	}

	// Vector2
	for(auto& [_, ch] : vec2Anims_) {
		ch->Reset();
	}

	// Vector3
	for(auto& [_, ch] : vec3Anims_) {
		ch->Reset();
	}

	// Vector4
	for(auto& [_, ch] : vec4Anims_) {
		ch->Reset();
	}
}