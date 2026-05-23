#pragma once

#include <Engine/Application/UI/EngineUI/IEngineUI.h>

#include <functional>
#include <memory>
#include <vector>

class SceneObject;

namespace CalyxEngine {

	class KeyframePanel : public IEngineUI {
	public:
		KeyframePanel();
		void Render() override;

		void SetSelectionProvider(std::function<std::vector<std::shared_ptr<SceneObject>>()> provider);
		void AddObject(const std::shared_ptr<SceneObject>& object);

	private:
		void DrawToolbar();
		void DrawDropTarget();
		void DrawTargetList();
		void DrawTracks();
		void DrawTimeline();
		void RemoveExpiredTargets();
		bool HasTarget(const SceneObject* object) const;

	private:
		std::vector<std::weak_ptr<SceneObject>> targets_;
		std::function<std::vector<std::shared_ptr<SceneObject>>()> selectionProvider_;
		float timelineTime_ = 0.0f;
		float timelineDuration_ = 3.0f;
		int samples_ = 30;
		int selectedTrackTab_ = 0;
	};

} // namespace CalyxEngine
