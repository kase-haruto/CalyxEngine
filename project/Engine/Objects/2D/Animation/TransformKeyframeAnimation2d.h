#pragma once

#include <Engine/Foundation/Math/Vector3.h>
#include <Engine/Foundation/Utility/Ease/CxEase.h>
#include <externals/nlohmann/json.hpp>

#include <cstdint>
#include <vector>

class WorldTransform;

namespace CalyxEngine {

	enum TransformKeyframe2dChannel : uint32_t {
		TransformKeyframe2dChannel_PositionX = 1u << 0,
		TransformKeyframe2dChannel_PositionY = 1u << 1,
		TransformKeyframe2dChannel_ScaleX = 1u << 2,
		TransformKeyframe2dChannel_ScaleY = 1u << 3,
		TransformKeyframe2dChannel_RotationZ = 1u << 4,
		TransformKeyframe2dChannel_All =
			TransformKeyframe2dChannel_PositionX |
			TransformKeyframe2dChannel_PositionY |
			TransformKeyframe2dChannel_ScaleX |
			TransformKeyframe2dChannel_ScaleY |
			TransformKeyframe2dChannel_RotationZ,
	};

	struct TransformKeyframe2d {
		float time = 0.0f;
		Vector3 translation{};
		Vector3 scale{1.0f, 1.0f, 1.0f};
		float rotationZ = 0.0f;
		EaseType ease = EaseType::Linear;
		uint32_t channels = TransformKeyframe2dChannel_All;
	};

	class TransformKeyframeAnimation2d {
	public:
		void Update(WorldTransform& target, float dt);
		void ApplyAt(WorldTransform& target, float time) const;
		void CaptureKey(WorldTransform& target, float time);
		void CaptureKey(WorldTransform& target, float time, uint32_t channels);
		bool RemoveKeyChannel(float time, uint32_t channels, float epsilon = 0.0001f);
		void SortKeys();
		void Clear();

		void Play();
		void Stop();
		void SetCurrentTime(float time) { currentTime_ = time; }
		float GetCurrentTime() const { return currentTime_; }
		float GetDuration() const { return duration_; }
		bool IsPlaying() const { return playing_; }
		bool IsLoop() const { return loop_; }
		bool IsAutoPlay() const { return autoPlay_; }
		void SetAutoPlay(bool autoPlay) { autoPlay_ = autoPlay; }
		bool IsEmpty() const { return keys_.empty(); }
		const std::vector<TransformKeyframe2d>& GetKeys() const { return keys_; }

		bool ShowGui(WorldTransform& target);

		void ApplyConfigFromJson(const nlohmann::json& j);
		void ExtractConfigToJson(nlohmann::json& j) const;

	private:
		const TransformKeyframe2d* FindPrevious(float time) const;
		const TransformKeyframe2d* FindNext(float time) const;
		void RecalculateDuration();

	private:
		std::vector<TransformKeyframe2d> keys_;
		float currentTime_ = 0.0f;
		float duration_ = 1.0f;
		bool loop_ = true;
		bool autoPlay_ = true;
		bool playing_ = false;
		int32_t selectedKeyIndex_ = -1;
	};

	void to_json(nlohmann::json& j, const TransformKeyframe2d& key);
	void from_json(const nlohmann::json& j, TransformKeyframe2d& key);

} // namespace CalyxEngine
