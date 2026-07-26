#include "KeyframePanel.h"

#include <Engine/Objects/3D/Actor/SceneObject.h>

#include <externals/imgui/imgui.h>

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstdio>
#include <utility>

namespace CalyxEngine {
	namespace {
		const char* Object2dDropPayloadName = "SceneObjectPtr";

		bool IsKeyframeTarget(const SceneObject* object) {
			return object && object->GetObjectType() == ObjectType::Object2D;
		}

		bool HasChannel(const TransformKeyframe2d& key, uint32_t channel) {
			return (key.channels & channel) != 0;
		}

		void DrawDiamond(ImDrawList* drawList, ImVec2 center, float radius, ImU32 color) {
			const ImVec2 points[] = {
				{center.x, center.y - radius},
				{center.x + radius, center.y},
				{center.x, center.y + radius},
				{center.x - radius, center.y},
			};
			drawList->AddConvexPolyFilled(points, 4, color);
			drawList->AddPolyline(points, 4, IM_COL32(20, 20, 20, 255), ImDrawFlags_Closed, 1.0f);
		}
	}

	KeyframePanel::KeyframePanel()
		: IEngineUI("Keyframe") {}

	void KeyframePanel::SetSelectionProvider(std::function<std::vector<std::shared_ptr<SceneObject>>()> provider) {
		selectionProvider_ = std::move(provider);
	}

	void KeyframePanel::AddObject(const std::shared_ptr<SceneObject>& object) {
		if(!IsKeyframeTarget(object.get()) || HasTarget(object.get())) {
			return;
		}
		targets_.push_back(object);
	}

	void KeyframePanel::Render() {
		if(!isShow_) return;

		// Scene切替やObject削除で失効したweak_ptrを描画前に除去する。
		RemoveExpiredTargets();

		if(ImGui::Begin(panelName_.c_str(), &isShow_)) {
			DrawToolbar();
			ImGui::Separator();
			DrawDropTarget();
			ImGui::Separator();
			DrawTimeline();
		}
		ImGui::End();
	}

	void KeyframePanel::DrawToolbar() {
		ImGui::SetNextItemWidth(110.0f);
		// Toolbar Scrubを全Targetへ適用し、複数ObjectのPreview時刻を揃える。
		if(ImGui::DragFloat("Time", &timelineTime_, 0.01f, 0.0f, 999.0f, "%.2f")) {
			for(auto& weak : targets_) {
				if(auto object = weak.lock()) {
					object->Get2DTransformAnimation().SetCurrentTime(timelineTime_);
					object->Get2DTransformAnimation().ApplyAt(object->GetWorldTransform(), timelineTime_);
				}
			}
		}

		ImGui::SameLine();
		ImGui::SetNextItemWidth(90.0f);
		ImGui::DragFloat("Duration", &timelineDuration_, 0.1f, 0.1f, 120.0f, "%.1f");

		ImGui::SameLine();
		ImGui::SetNextItemWidth(80.0f);
		ImGui::DragInt("Samples", &samples_, 1.0f, 1, 240);

		ImGui::SameLine();
		// 現在のTransformを同一時刻へ記録し、複数TargetのAnimation開始点を同期する。
		if(ImGui::Button("Add Key To All")) {
			for(auto& weak : targets_) {
				if(auto object = weak.lock()) {
					object->Get2DTransformAnimation().CaptureKey(object->GetWorldTransform(), timelineTime_);
				}
			}
		}

		ImGui::SameLine();
		if(ImGui::Button("Apply Time")) {
			for(auto& weak : targets_) {
				if(auto object = weak.lock()) {
					object->Get2DTransformAnimation().SetCurrentTime(timelineTime_);
					object->Get2DTransformAnimation().ApplyAt(object->GetWorldTransform(), timelineTime_);
				}
			}
		}

		ImGui::SameLine();
		if(ImGui::Button("Play All")) {
			for(auto& weak : targets_) {
				if(auto object = weak.lock()) {
					object->Get2DTransformAnimation().Play();
				}
			}
		}

		ImGui::SameLine();
		if(ImGui::Button("Stop All")) {
			for(auto& weak : targets_) {
				if(auto object = weak.lock()) {
					object->Get2DTransformAnimation().Stop();
				}
			}
		}

		ImGui::SameLine();
		if(ImGui::Button("Auto Play All")) {
			for(auto& weak : targets_) {
				if(auto object = weak.lock()) {
					object->Get2DTransformAnimation().SetAutoPlay(true);
				}
			}
		}

		ImGui::SameLine();
		if(ImGui::Button("Manual All")) {
			for(auto& weak : targets_) {
				if(auto object = weak.lock()) {
					object->Get2DTransformAnimation().SetAutoPlay(false);
				}
			}
		}

		ImGui::SameLine();
		if(selectionProvider_ && ImGui::Button("Add Selection")) {
			for(const auto& object : selectionProvider_()) {
				AddObject(object);
			}
		}
	}

	void KeyframePanel::DrawDropTarget() {
		ImGui::BeginChild("KeyframeDropTarget", ImVec2(-FLT_MIN, 32.0f), true);
		ImGui::TextUnformatted("Object2D Tracks");

		// Object2D専用Payloadだけを受理し、3D Objectを誤ってTrackへ登録しない。
		if(ImGui::BeginDragDropTarget()) {
			if(const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(Object2dDropPayloadName)) {
				if(payload->DataSize == sizeof(SceneObject*)) {
					SceneObject* object = *static_cast<SceneObject**>(payload->Data);
					if(IsKeyframeTarget(object)) {
						AddObject(object->shared_from_this());
					}
				}
			}
			ImGui::EndDragDropTarget();
		}
		ImGui::EndChild();
	}

	void KeyframePanel::DrawTargetList() {
		ImGui::TextUnformatted("Targets");
		ImGui::Separator();

		for(size_t i = 0; i < targets_.size();) {
			auto object = targets_[i].lock();
			if(!object) {
				targets_.erase(targets_.begin() + static_cast<std::ptrdiff_t>(i));
				continue;
			}

			ImGui::PushID(static_cast<int>(i));
			const std::string displayName = object->GetDisplayName();
			ImGui::TextUnformatted(displayName.c_str());
			ImGui::SameLine();
			if(ImGui::SmallButton("Remove")) {
				targets_.erase(targets_.begin() + static_cast<std::ptrdiff_t>(i));
				ImGui::PopID();
				continue;
			}
			ImGui::PopID();
			++i;
		}

		if(targets_.empty()) {
			ImGui::TextDisabled("No Object2D targets.");
		}
	}

	void KeyframePanel::DrawTracks() {
		for(size_t i = 0; i < targets_.size(); ++i) {
			auto object = targets_[i].lock();
			if(!object) continue;

			ImGui::PushID(static_cast<int>(i));
			const std::string label = object->GetDisplayName() + "##track";
			if(ImGui::CollapsingHeader(label.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
				object->Get2DTransformAnimation().ShowGui(object->GetWorldTransform());
			}
			ImGui::PopID();
		}
	}

	void KeyframePanel::DrawTimeline() {
		struct TrackRow {
			const char* label;
			uint32_t channel;
		};

		const TrackRow allRows[] = {
			{"Position.x", TransformKeyframe2dChannel_PositionX},
			{"Position.y", TransformKeyframe2dChannel_PositionY},
			{"Scale.x", TransformKeyframe2dChannel_ScaleX},
			{"Scale.y", TransformKeyframe2dChannel_ScaleY},
			{"Rotation.z", TransformKeyframe2dChannel_RotationZ},
		};
		const TrackRow positionRows[] = {
			{"X", TransformKeyframe2dChannel_PositionX},
			{"Y", TransformKeyframe2dChannel_PositionY},
		};
		const TrackRow scaleRows[] = {
			{"X", TransformKeyframe2dChannel_ScaleX},
			{"Y", TransformKeyframe2dChannel_ScaleY},
		};
		const TrackRow rotationRows[] = {
			{"Z", TransformKeyframe2dChannel_RotationZ},
		};

		// Tab選択に応じて表示Channelだけを絞り、Animationデータ自体は変更しない。
		const TrackRow* rows = allRows;
		int rowCount = static_cast<int>(sizeof(allRows) / sizeof(allRows[0]));
		if(ImGui::BeginTabBar("KeyframeTrackTabs")) {
			if(ImGui::BeginTabItem("All")) {
				selectedTrackTab_ = 0;
				ImGui::EndTabItem();
			}
			if(ImGui::BeginTabItem("Position")) {
				selectedTrackTab_ = 1;
				ImGui::EndTabItem();
			}
			if(ImGui::BeginTabItem("Scale")) {
				selectedTrackTab_ = 2;
				ImGui::EndTabItem();
			}
			if(ImGui::BeginTabItem("Rotation")) {
				selectedTrackTab_ = 3;
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}
		switch(selectedTrackTab_) {
		case 1:
			rows = positionRows;
			rowCount = static_cast<int>(sizeof(positionRows) / sizeof(positionRows[0]));
			break;
		case 2:
			rows = scaleRows;
			rowCount = static_cast<int>(sizeof(scaleRows) / sizeof(scaleRows[0]));
			break;
		case 3:
			rows = rotationRows;
			rowCount = static_cast<int>(sizeof(rotationRows) / sizeof(rotationRows[0]));
			break;
		default:
			break;
		}

		// Label領域と時間領域を分離し、横Scroll時も座標変換の基準を一定にする。
		const float leftWidth = 150.0f;
		const float rulerHeight = 30.0f;
		const float objectHeight = 24.0f;
		const float rowHeight = 22.0f;

		if(targets_.empty()) {
			ImGui::TextDisabled("Drop Object2D targets to edit transform keyframes.");
			return;
		}

		ImGui::BeginChild("KeyframeTimeline", ImVec2(-FLT_MIN, -FLT_MIN), true, ImGuiWindowFlags_HorizontalScrollbar);

		ImDrawList* drawList = ImGui::GetWindowDrawList();
		const ImVec2 origin = ImGui::GetCursorScreenPos();
		const ImVec2 avail = ImGui::GetContentRegionAvail();
		const float timelineWidth = (std::max)(520.0f, avail.x - leftWidth);
		const float timelineX = origin.x + leftWidth;
		const float endX = timelineX + timelineWidth;
		const float duration = (std::max)(0.1f, timelineDuration_);
		const int safeSamples = (std::max)(1, samples_);

		// Timeline時刻とScreen Xの相互変換を一か所へ集約し、Ruler・Key・Playheadで共有する。
		auto timeToX = [&](float time) {
			const float t = (std::max)(0.0f, (std::min)(time / duration, 1.0f));
			return timelineX + timelineWidth * t;
		};
		auto xToTime = [&](float x) {
			const float t = (std::max)(0.0f, (std::min)((x - timelineX) / timelineWidth, 1.0f));
			return duration * t;
		};
		auto applyTimeToAll = [&]() {
			for(auto& weak : targets_) {
				if(auto object = weak.lock()) {
					auto& animation = object->Get2DTransformAnimation();
					animation.SetCurrentTime(timelineTime_);
					animation.ApplyAt(object->GetWorldTransform(), timelineTime_);
				}
			}
		};

		drawList->AddRectFilled(origin, {endX, origin.y + rulerHeight}, IM_COL32(38, 38, 38, 255));
		drawList->AddRectFilled({origin.x, origin.y}, {timelineX, origin.y + rulerHeight}, IM_COL32(45, 45, 45, 255));
		drawList->AddText({origin.x + 6.0f, origin.y + 8.0f}, IM_COL32(220, 220, 220, 255), "Object");

		// Sample数から補助Gridを生成し、主要目盛だけに時刻Labelを表示する。
		const float frameStep = duration / static_cast<float>(safeSamples);
		for(int frame = 0; frame <= safeSamples; ++frame) {
			const float time = frameStep * static_cast<float>(frame);
			const float x = timeToX(time);
			const bool major = (frame % (std::max)(1, safeSamples / 6)) == 0;
			drawList->AddLine({x, origin.y}, {x, origin.y + rulerHeight}, major ? IM_COL32(125, 125, 125, 255) : IM_COL32(72, 72, 72, 255), 1.0f);
			if(major) {
				char label[32]{};
				std::snprintf(label, sizeof(label), "%.2f", time);
				drawList->AddText({x + 3.0f, origin.y + 7.0f}, IM_COL32(205, 205, 205, 255), label);
			}
		}

		// 描画したRuler上へ透明な操作領域を重ね、Click/DragでScrub可能にする。
		ImGui::SetCursorScreenPos({timelineX, origin.y});
		ImGui::InvisibleButton("##timelineRuler", ImVec2(timelineWidth, rulerHeight));
		if((ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) ||
		   (ImGui::IsItemActive() && ImGui::IsMouseDown(ImGuiMouseButton_Left))) {
			timelineTime_ = xToTime(ImGui::GetIO().MousePos.x);
			applyTimeToAll();
		}

		float y = origin.y + rulerHeight;
		for(size_t targetIndex = 0; targetIndex < targets_.size(); ++targetIndex) {
			auto object = targets_[targetIndex].lock();
			if(!object) continue;

			const float objectY = y;
			drawList->AddRectFilled({origin.x, objectY}, {endX, objectY + objectHeight}, IM_COL32(50, 50, 50, 255));
			drawList->AddText({origin.x + 6.0f, objectY + 5.0f}, IM_COL32(235, 235, 235, 255), object->GetDisplayName().c_str());
			y += objectHeight;

			// ObjectごとにChannel Rowを展開し、各Keyが保持するChannel maskに応じて表示する。
			for(int row = 0; row < rowCount; ++row) {
				const float rowY = y;
				const ImU32 bg = (row % 2 == 0) ? IM_COL32(31, 31, 31, 255) : IM_COL32(36, 36, 36, 255);
				drawList->AddRectFilled({origin.x, rowY}, {endX, rowY + rowHeight}, bg);
				drawList->AddText({origin.x + 22.0f, rowY + 4.0f}, IM_COL32(205, 205, 205, 255), rows[row].label);
				drawList->AddLine({timelineX, rowY}, {timelineX, rowY + rowHeight}, IM_COL32(70, 70, 70, 255), 1.0f);

				for(int frame = 0; frame <= safeSamples; ++frame) {
					const float x = timelineX + timelineWidth * (static_cast<float>(frame) / static_cast<float>(safeSamples));
					drawList->AddLine({x, rowY}, {x, rowY + rowHeight}, IM_COL32(55, 55, 55, 180), 1.0f);
				}

				ImGui::PushID(static_cast<int>(targetIndex));
				ImGui::PushID(row);
				ImGui::SetCursorScreenPos({timelineX, rowY});
				ImGui::InvisibleButton("##timelineRow", ImVec2(timelineWidth, rowHeight));
				if((ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) ||
				   (ImGui::IsItemActive() && ImGui::IsMouseDown(ImGuiMouseButton_Left))) {
					timelineTime_ = xToTime(ImGui::GetIO().MousePos.x);
					applyTimeToAll();
				}
				// RowのDouble Clickは該当Channelだけを現在TransformからCaptureする。
				if(ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
					timelineTime_ = xToTime(ImGui::GetIO().MousePos.x);
					auto& animation = object->Get2DTransformAnimation();
					animation.SetCurrentTime(timelineTime_);
					animation.CaptureKey(object->GetWorldTransform(), timelineTime_, rows[row].channel);
				}
				ImGui::PopID();
				ImGui::PopID();

				const auto keys = object->Get2DTransformAnimation().GetKeys();
				for(size_t keyIndex = 0; keyIndex < keys.size(); ++keyIndex) {
					const auto& key = keys[keyIndex];
					if(!HasChannel(key, rows[row].channel)) continue;
					const float keyX = timeToX(key.time);
					const ImVec2 keyCenter{keyX, rowY + rowHeight * 0.5f};
					DrawDiamond(drawList, keyCenter, 5.0f, IM_COL32(218, 218, 218, 255));

					ImGui::PushID(static_cast<int>(targetIndex));
					ImGui::PushID(row);
					ImGui::PushID(static_cast<int>(keyIndex));
					ImGui::SetCursorScreenPos({keyCenter.x - 7.0f, keyCenter.y - 7.0f});
					ImGui::InvisibleButton("##key", ImVec2(14.0f, 14.0f));
					if(ImGui::IsItemHovered()) {
						ImGui::SetTooltip("Left: move time / Right: delete key");
					}
					if(ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
						timelineTime_ = key.time;
						applyTimeToAll();
					}
					// 右ClickではKey全体でなく表示中Channelのみを削除する。
					if(ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
						object->Get2DTransformAnimation().RemoveKeyChannel(key.time, rows[row].channel);
						ImGui::PopID();
						ImGui::PopID();
						ImGui::PopID();
						break;
					}
					ImGui::PopID();
					ImGui::PopID();
					ImGui::PopID();
				}

				y += rowHeight;
			}
		}

		// 全Trackを貫くPlayheadを最後に描き、KeyやGridより前面へ表示する。
		const float playheadX = timeToX(timelineTime_);
		drawList->AddLine({playheadX, origin.y}, {playheadX, y}, IM_COL32(230, 45, 48, 255), 2.0f);
		drawList->AddTriangleFilled(
			{playheadX - 6.0f, origin.y},
			{playheadX + 6.0f, origin.y},
			{playheadX, origin.y + 8.0f},
			IM_COL32(230, 45, 48, 255));

		ImGui::SetCursorScreenPos(origin);
		ImGui::Dummy(ImVec2(leftWidth + timelineWidth, y - origin.y));
		ImGui::EndChild();
	}

	void KeyframePanel::RemoveExpiredTargets() {
		targets_.erase(
			std::remove_if(
				targets_.begin(),
				targets_.end(),
				[](const std::weak_ptr<SceneObject>& weak) {
					return weak.expired();
				}),
			targets_.end());
	}

	bool KeyframePanel::HasTarget(const SceneObject* object) const {
		if(!object) return false;
		for(const auto& weak : targets_) {
			if(auto current = weak.lock()) {
				if(current.get() == object) {
					return true;
				}
			}
		}
		return false;
	}

} // namespace CalyxEngine
