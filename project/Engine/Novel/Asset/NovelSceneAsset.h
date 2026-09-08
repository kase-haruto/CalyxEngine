#pragma once

#include <Engine/Foundation/Export/CalyxAPI.h>
#include <Engine/Novel/Asset/NovelEvent.h>
#include <Engine/Renderer/Text/TextStyle.h>

#include <filesystem>
#include <vector>

namespace CalyxEngine {

// Presentation settings shared by every Dialogue event in one Novel scene.
// SceneObject clips keep using the settings stored by their own SceneObject.
struct NovelTextSettings {
  Guid fontGuid_{};
  Vector2 dialoguePosition_{80.0f, 760.0f};
  Vector2 dialogueSize_{1760.0f, 220.0f};
  Vector2 speakerPosition_{80.0f, 700.0f};
  Vector2 speakerSize_{600.0f, 60.0f};
  TextStyle dialogueStyle_{};
  TextStyle speakerStyle_{};

  NovelTextSettings() {
    dialogueStyle_.fontSize_ = 32.0f;
    dialogueStyle_.wordWrap_ = true;
    speakerStyle_.fontSize_ = 28.0f;
    speakerStyle_.wordWrap_ = false;
  }
};

class CALYX_API NovelSceneAsset {
public:
  const std::vector<NovelEvent> &GetEvents() const noexcept {
    return events_;
  }
  std::vector<NovelEvent> &GetEvents() noexcept {
    return events_;
  }
  NovelEvent *FindEvent(const Guid &guid) noexcept;
  const NovelEvent *FindEvent(const Guid &guid) const noexcept;
  NovelEvent &AddEvent(NovelEventType type);
  bool RemoveEvent(const Guid &guid);
  bool MoveEvent(const Guid &guid, int offset);
  bool Load(const std::filesystem::path &path);
  bool Save(const std::filesystem::path &path) const;
  const NovelTextSettings &GetTextSettings() const noexcept {
    return textSettings_;
  }
  NovelTextSettings &GetTextSettings() noexcept {
    return textSettings_;
  }

private:
  NovelTextSettings textSettings_{};
  std::vector<NovelEvent> events_;
};
} // namespace CalyxEngine
