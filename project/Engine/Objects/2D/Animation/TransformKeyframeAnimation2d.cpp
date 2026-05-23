#include "TransformKeyframeAnimation2d.h"

#include <Engine/Foundation/Math/Quaternion.h>
#include <Engine/Objects/Transform/Transform.h>

#include <externals/imgui/imgui.h>

#include <algorithm>
#include <cstddef>
#include <cmath>

namespace CalyxEngine {
	namespace {
		float Clamp01(float value) {
			return (std::max)(0.0f, (std::min)(1.0f, value));
		}

		TransformKeyframe2d CaptureTransform(const WorldTransform& target, float time) {
			TransformKeyframe2d key;
			key.time = time;
			key.translation = target.translation;
			key.scale = target.scale;
			key.rotationZ = target.eulerRotation.z;
			key.ease = EaseType::Linear;
			return key;
		}

		bool HasChannel(const TransformKeyframe2d& key, uint32_t channel) {
			return (key.channels & channel) != 0;
		}

		const TransformKeyframe2d* FindPreviousChannel(
			const std::vector<TransformKeyframe2d>& keys,
			float time,
			uint32_t channel) {
			const TransformKeyframe2d* result = nullptr;
			for(const auto& key : keys) {
				if(key.time <= time && HasChannel(key, channel)) result = &key;
				if(key.time > time) break;
			}
			return result;
		}

		const TransformKeyframe2d* FindNextChannel(
			const std::vector<TransformKeyframe2d>& keys,
			float time,
			uint32_t channel) {
			for(const auto& key : keys) {
				if(key.time >= time && HasChannel(key, channel)) return &key;
			}
			return nullptr;
		}

		float InterpolateChannel(
			const std::vector<TransformKeyframe2d>& keys,
			float time,
			uint32_t channel,
			float currentValue,
			float TransformKeyframe2d::*member) {
			const TransformKeyframe2d* prev = FindPreviousChannel(keys, time, channel);
			const TransformKeyframe2d* next = FindNextChannel(keys, time, channel);
			if(!prev && !next) return currentValue;
			if(!prev) return next->*member;
			if(!next || prev == next) return prev->*member;

			const float span = (std::max)(0.0001f, next->time - prev->time);
			const float t = Clamp01((time - prev->time) / span);
			const float eased = ApplyEase(prev->ease, t);
			return (prev->*member) + ((next->*member) - (prev->*member)) * eased;
		}

		float InterpolateVectorChannel(
			const std::vector<TransformKeyframe2d>& keys,
			float time,
			uint32_t channel,
			float currentValue,
			Vector3 TransformKeyframe2d::*vectorMember,
			float Vector3::*component) {
			const TransformKeyframe2d* prev = FindPreviousChannel(keys, time, channel);
			const TransformKeyframe2d* next = FindNextChannel(keys, time, channel);
			if(!prev && !next) return currentValue;
			if(!prev) return (next->*vectorMember).*component;
			if(!next || prev == next) return (prev->*vectorMember).*component;

			const float span = (std::max)(0.0001f, next->time - prev->time);
			const float t = Clamp01((time - prev->time) / span);
			const float eased = ApplyEase(prev->ease, t);
			const float from = (prev->*vectorMember).*component;
			const float to = (next->*vectorMember).*component;
			return from + (to - from) * eased;
		}
	}

	void TransformKeyframeAnimation2d::Update(WorldTransform& target, float dt) {
		if(!playing_ || keys_.empty()) return;
		currentTime_ += dt;
		if(duration_ > 0.0f && currentTime_ > duration_) {
			if(loop_) {
				currentTime_ = std::fmod(currentTime_, duration_);
			} else {
				currentTime_ = duration_;
				playing_ = false;
			}
		}
		ApplyAt(target, currentTime_);
	}

	void TransformKeyframeAnimation2d::ApplyAt(WorldTransform& target, float time) const {
		if(keys_.empty()) return;
		target.translation.x = InterpolateVectorChannel(keys_, time, TransformKeyframe2dChannel_PositionX, target.translation.x, &TransformKeyframe2d::translation, &Vector3::x);
		target.translation.y = InterpolateVectorChannel(keys_, time, TransformKeyframe2dChannel_PositionY, target.translation.y, &TransformKeyframe2d::translation, &Vector3::y);
		target.scale.x = InterpolateVectorChannel(keys_, time, TransformKeyframe2dChannel_ScaleX, target.scale.x, &TransformKeyframe2d::scale, &Vector3::x);
		target.scale.y = InterpolateVectorChannel(keys_, time, TransformKeyframe2dChannel_ScaleY, target.scale.y, &TransformKeyframe2d::scale, &Vector3::y);
		target.eulerRotation = {0.0f, 0.0f, InterpolateChannel(keys_, time, TransformKeyframe2dChannel_RotationZ, target.eulerRotation.z, &TransformKeyframe2d::rotationZ)};
		target.rotationSource = RotationSource::Euler;
		target.Update();
	}

	void TransformKeyframeAnimation2d::CaptureKey(WorldTransform& target, float time) {
		CaptureKey(target, time, TransformKeyframe2dChannel_All);
	}

	void TransformKeyframeAnimation2d::CaptureKey(WorldTransform& target, float time, uint32_t channels) {
		for(auto& key : keys_) {
			if(std::abs(key.time - time) <= 0.0001f) {
				const TransformKeyframe2d captured = CaptureTransform(target, time);
				if((channels & TransformKeyframe2dChannel_PositionX) != 0) key.translation.x = captured.translation.x;
				if((channels & TransformKeyframe2dChannel_PositionY) != 0) key.translation.y = captured.translation.y;
				if((channels & TransformKeyframe2dChannel_ScaleX) != 0) key.scale.x = captured.scale.x;
				if((channels & TransformKeyframe2dChannel_ScaleY) != 0) key.scale.y = captured.scale.y;
				if((channels & TransformKeyframe2dChannel_RotationZ) != 0) key.rotationZ = captured.rotationZ;
				key.channels |= channels;
				SortKeys();
				RecalculateDuration();
				return;
			}
		}
		auto key = CaptureTransform(target, time);
		key.channels = channels;
		keys_.push_back(key);
		SortKeys();
		RecalculateDuration();
		selectedKeyIndex_ = static_cast<int32_t>(keys_.size()) - 1;
	}

	bool TransformKeyframeAnimation2d::RemoveKeyChannel(float time, uint32_t channels, float epsilon) {
		for(size_t i = 0; i < keys_.size(); ++i) {
			auto& key = keys_[i];
			if(std::abs(key.time - time) > epsilon) continue;

			key.channels &= ~channels;
			if(key.channels == 0) {
				keys_.erase(keys_.begin() + static_cast<std::ptrdiff_t>(i));
				if(selectedKeyIndex_ == static_cast<int32_t>(i)) {
					selectedKeyIndex_ = -1;
				} else if(selectedKeyIndex_ > static_cast<int32_t>(i)) {
					--selectedKeyIndex_;
				}
			}
			RecalculateDuration();
			return true;
		}
		return false;
	}

	void TransformKeyframeAnimation2d::SortKeys() {
		std::sort(keys_.begin(), keys_.end(), [](const auto& lhs, const auto& rhs) {
			return lhs.time < rhs.time;
		});
	}

	void TransformKeyframeAnimation2d::Clear() {
		keys_.clear();
		currentTime_ = 0.0f;
		duration_ = 1.0f;
		playing_ = false;
		selectedKeyIndex_ = -1;
	}

	void TransformKeyframeAnimation2d::Play() {
		if(keys_.empty()) return;
		currentTime_ = 0.0f;
		playing_ = true;
	}

	void TransformKeyframeAnimation2d::Stop() {
		playing_ = false;
	}

	bool TransformKeyframeAnimation2d::ShowGui(WorldTransform& target) {
		bool changed = false;
		if(ImGui::TreeNodeEx("2D Transform Animation", ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DefaultOpen)) {
			changed |= ImGui::Checkbox("Auto Play", &autoPlay_);
			ImGui::SameLine();
			changed |= ImGui::Checkbox("Loop", &loop_);
			ImGui::SameLine();
			if(ImGui::Button(playing_ ? "Stop" : "Play", ImVec2(64.0f, 0.0f))) {
				if(playing_) Stop();
				else Play();
			}
			ImGui::SameLine();
			if(ImGui::Button("Apply")) {
				ApplyAt(target, currentTime_);
			}

			ImGui::SetNextItemWidth(160.0f);
			if(ImGui::DragFloat("Time", &currentTime_, 0.01f, 0.0f, (std::max)(duration_, 0.01f), "%.2f")) {
				ApplyAt(target, currentTime_);
			}
			ImGui::SameLine();
			if(ImGui::Button("Add Key")) {
				CaptureKey(target, currentTime_);
				changed = true;
			}
			ImGui::SameLine();
			if(ImGui::Button("Clear")) {
				Clear();
				changed = true;
			}

			if(ImGui::BeginTable("TransformKeyframe2dTable", 5, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
				ImGui::TableSetupColumn("Time", ImGuiTableColumnFlags_WidthFixed, 54.0f);
				ImGui::TableSetupColumn("Pos");
				ImGui::TableSetupColumn("Scale");
				ImGui::TableSetupColumn("RotZ", ImGuiTableColumnFlags_WidthFixed, 58.0f);
				ImGui::TableSetupColumn("Ease", ImGuiTableColumnFlags_WidthFixed, 116.0f);
				ImGui::TableHeadersRow();

				for(int32_t i = 0; i < static_cast<int32_t>(keys_.size()); ++i) {
					auto& key = keys_[static_cast<size_t>(i)];
					ImGui::PushID(i);
					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					if(ImGui::Selectable("##select", selectedKeyIndex_ == i, ImGuiSelectableFlags_SpanAllColumns)) {
						selectedKeyIndex_ = i;
						currentTime_ = key.time;
						ApplyAt(target, currentTime_);
					}
					ImGui::SameLine();
					if(ImGui::DragFloat("##time", &key.time, 0.01f, 0.0f, 999.0f, "%.2f")) {
						changed = true;
					}
					ImGui::TableNextColumn();
					changed |= ImGui::DragFloat3("##pos", &key.translation.x, 0.1f);
					ImGui::TableNextColumn();
					changed |= ImGui::DragFloat3("##scale", &key.scale.x, 0.1f, 0.0f, 9999.0f);
					ImGui::TableNextColumn();
					changed |= ImGui::DragFloat("##rot", &key.rotationZ, 0.01f);
					ImGui::TableNextColumn();
					int32_t ease = static_cast<int32_t>(key.ease);
					if(SelectEaseInt("##ease", ease)) {
						key.ease = static_cast<EaseType>(ease);
						changed = true;
					}
					ImGui::PopID();
				}
				ImGui::EndTable();
			}

			if(selectedKeyIndex_ >= 0 && selectedKeyIndex_ < static_cast<int32_t>(keys_.size())) {
				if(ImGui::Button("Capture Selected")) {
					keys_[static_cast<size_t>(selectedKeyIndex_)] = CaptureTransform(target, keys_[static_cast<size_t>(selectedKeyIndex_)].time);
					changed = true;
				}
				ImGui::SameLine();
				if(ImGui::Button("Delete Selected")) {
					keys_.erase(keys_.begin() + selectedKeyIndex_);
					selectedKeyIndex_ = -1;
					changed = true;
				}
			}

			if(changed) {
				SortKeys();
				RecalculateDuration();
			}
			ImGui::TreePop();
		}
		return changed;
	}

	void TransformKeyframeAnimation2d::ApplyConfigFromJson(const nlohmann::json& j) {
		keys_.clear();
		loop_ = j.value("loop", loop_);
		autoPlay_ = j.value("autoPlay", autoPlay_);
		currentTime_ = j.value("currentTime", 0.0f);
		if(j.contains("keys") && j.at("keys").is_array()) {
			keys_ = j.at("keys").get<std::vector<TransformKeyframe2d>>();
		}
		SortKeys();
		RecalculateDuration();
		playing_ = autoPlay_ && !keys_.empty();
		if(playing_) {
			currentTime_ = 0.0f;
		}
		selectedKeyIndex_ = -1;
	}

	void TransformKeyframeAnimation2d::ExtractConfigToJson(nlohmann::json& j) const {
		j["loop"] = loop_;
		j["autoPlay"] = autoPlay_;
		j["currentTime"] = currentTime_;
		j["keys"] = keys_;
	}

	const TransformKeyframe2d* TransformKeyframeAnimation2d::FindPrevious(float time) const {
		const TransformKeyframe2d* result = nullptr;
		for(const auto& key : keys_) {
			if(key.time <= time) result = &key;
			else break;
		}
		return result;
	}

	const TransformKeyframe2d* TransformKeyframeAnimation2d::FindNext(float time) const {
		for(const auto& key : keys_) {
			if(key.time >= time) return &key;
		}
		return keys_.empty() ? nullptr : &keys_.back();
	}

	void TransformKeyframeAnimation2d::RecalculateDuration() {
		duration_ = keys_.empty() ? 1.0f : (std::max)(0.01f, keys_.back().time);
	}

	void to_json(nlohmann::json& j, const TransformKeyframe2d& key) {
		j = nlohmann::json{
			{"time", key.time},
			{"translation", key.translation},
			{"scale", key.scale},
			{"rotationZ", key.rotationZ},
			{"ease", static_cast<int32_t>(key.ease)},
			{"channels", key.channels}};
	}

	void from_json(const nlohmann::json& j, TransformKeyframe2d& key) {
		key.time = j.value("time", 0.0f);
		key.translation = j.value("translation", key.translation);
		key.scale = j.value("scale", key.scale);
		key.rotationZ = j.value("rotationZ", 0.0f);
		key.channels = j.value("channels", TransformKeyframe2dChannel_All);
		const int32_t ease = j.value("ease", static_cast<int32_t>(EaseType::Linear));
		key.ease = static_cast<EaseType>((std::max)(0, (std::min)(ease, static_cast<int32_t>(EaseType::Count) - 1)));
	}

} // namespace CalyxEngine
