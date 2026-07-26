#pragma once

#include <cstdint>
#include <memory>
#include <unordered_map>

class SceneContext;
class SceneObject;

namespace CalyxEngine {

	class FxObject;

	/**
	 * @brief ParticlePreviewSessionの機能を提供するクラスです。
	 */
	class ParticlePreviewSession {
	public:
		void Ensure();
		void Update(float dt);

		SceneContext* Context() const { return context_.get(); }
		std::shared_ptr<FxObject> Object() const;

	private:
		void EnsureDefaultObject();
		void UpdateEmitterPlayback();

		std::unique_ptr<SceneContext> context_;
		std::shared_ptr<FxObject> fx_;
		std::unordered_map<SceneObject*, uint64_t> playedEmitterRevisions_;
	};

} // namespace CalyxEngine
