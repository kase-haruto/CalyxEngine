#pragma once

#include <Engine/Assets/System/AssetType.h>
#include <Engine/Foundation/Export/CalyxAPI.h>
#include <Engine/Novel/Asset/DialogueAsset.h>
#include <Engine/Novel/Asset/NovelSceneAsset.h>

#include <memory>
#include <unordered_map>

class SceneObject;

namespace CalyxEngine {
class TextSceneObject2d;
class SpriteObject2d;
class CALYX_API NovelPlayer {
public:
  void SetScene(const NovelSceneAsset *scene) noexcept;
  void Play();
  void Pause() noexcept;
  void Stop();
  void Update(float dt);
  void Next();
  bool IsPlaying() const noexcept {
    return playing_;
  }
  bool IsFinished() const noexcept {
    return finished_;
  }

private:
  void StartCurrentEvent();
  void Advance();
  void ClearPresentation();
  void PrepareSceneObjects();
  std::string ResolveAssetPath(const Guid &guid, AssetType type) const;
  const NovelSceneAsset *scene_ = nullptr;
  size_t currentEventIndex_ = 0;
  bool playing_ = false;
  bool finished_ = false;
  bool eventStarted_ = false;
  float eventElapsedTime_ = 0;
  float autoAdvanceElapsed_ = 0;
  DialogueAsset dialogueAsset_;
  std::shared_ptr<TextSceneObject2d> dialogueText_;
  std::shared_ptr<TextSceneObject2d> speakerText_;
  std::shared_ptr<SpriteObject2d> background_;
  std::unordered_map<Guid, std::shared_ptr<SpriteObject2d>> images_;
  std::shared_ptr<SceneObject> activeSceneObject_;
  bool activeSceneObjectHideOnComplete_ = false;
  std::unordered_map<Guid, bool> originalSceneObjectVisibility_;
};
} // namespace CalyxEngine
