#pragma once

#include <Engine/Foundation/Math/Vector2.h>
#include <Engine/Foundation/Utility/Guid/Guid.h>

#include <variant>

namespace CalyxEngine {
enum class NovelEventType {
  Dialogue,
  ShowImage,
  HideImage,
  ChangeBackground,
  Wait,
  SceneObject,
};

// Timeline clips either finish after a duration or wait for an external Next
// call.
enum class NovelClipAdvanceMode {
  Time,
  Input,
};

struct NovelDialogueEvent {
  Guid dialogueAssetGuid_{};
  Guid dialogueLineGuid_{};
};

struct NovelShowImageEvent {
  Guid textureGuid_{};
  Vector2 position_{};
  Vector2 scale_{1.0f, 1.0f};
};

struct NovelHideImageEvent {
  Guid targetGuid_{};
};

struct NovelBackgroundEvent {
  Guid textureGuid_{};
};

struct NovelWaitEvent {
  float duration_ = 1.0f;
};

// References an object already owned and serialized by the Scene.
// The Novel asset stores only its GUID and never owns the SceneObject.
struct NovelSceneObjectEvent {
  Guid sceneObjectGuid_{};
  NovelClipAdvanceMode advanceMode_ = NovelClipAdvanceMode::Input;
  float duration_ = 1.0f;
  bool hideOnComplete_ = false;
};

using NovelEventData =
    std::variant<NovelDialogueEvent, NovelShowImageEvent, NovelHideImageEvent,
                 NovelBackgroundEvent, NovelWaitEvent, NovelSceneObjectEvent>;

struct NovelEvent {
  Guid guid_{};
  NovelEventType type_ = NovelEventType::Dialogue;
  NovelEventData data_ = NovelDialogueEvent{};
};

inline NovelEventData MakeNovelEventData(NovelEventType type) {
  switch (type) {
  case NovelEventType::ShowImage:
    return NovelShowImageEvent{};
  case NovelEventType::HideImage:
    return NovelHideImageEvent{};
  case NovelEventType::ChangeBackground:
    return NovelBackgroundEvent{};
  case NovelEventType::Wait:
    return NovelWaitEvent{};
  case NovelEventType::SceneObject:
    return NovelSceneObjectEvent{};
  default:
    return NovelDialogueEvent{};
  }
}
} // namespace CalyxEngine
