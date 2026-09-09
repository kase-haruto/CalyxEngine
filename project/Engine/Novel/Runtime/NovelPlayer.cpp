#include "NovelPlayer.h"

#include <Engine/Assets/Database/AssetDatabase.h>
#include <Engine/Objects/2D/Object2d/SpriteObject2d.h>
#include <Engine/Objects/2D/Object2d/TextSceneObject2d.h>
#include <Engine/Scene/Reference/SceneObjectReference.h>
#include <Engine/Scene/Utility/SceneUtility.h>
#include <algorithm>
#include <filesystem>

namespace CalyxEngine {
void NovelPlayer::SetScene(const NovelSceneAsset *scene) noexcept {
  if (scene_ != scene) {
    Stop();
    scene_ = scene;
  }
}
void NovelPlayer::Play() {
  if (!scene_)
    return;
  ClearPresentation();
  PrepareSceneObjects();
  currentEventIndex_ = 0;
  eventStarted_ = false;
  finished_ = false;
  playing_ = true;
}
void NovelPlayer::Pause() noexcept {
  playing_ = false;
  if (dialogueText_)
    dialogueText_->Pause();
}
void NovelPlayer::Stop() {
  playing_ = false;
  finished_ = false;
  currentEventIndex_ = 0;
  eventStarted_ = false;
  ClearPresentation();
}
std::string NovelPlayer::ResolveAssetPath(const Guid &guid,
                                          AssetType type) const {
  const auto *db = AssetDatabase::GetInstance();
  const auto *r = guid.isValid() ? db->Get(guid) : nullptr;
  if (!r || r->type != type)
    return {};
  std::error_code e;
  auto p = std::filesystem::relative(r->sourcePath, db->GetRoot(), e);
  return e ? r->sourcePath.generic_string() : p.generic_string();
}
void NovelPlayer::ClearPresentation() {
  if (dialogueText_)
    SceneAPI::RemoveObject(dialogueText_);
  if (speakerText_)
    SceneAPI::RemoveObject(speakerText_);
  if (background_)
    SceneAPI::RemoveObject(background_);
  for (auto &[_, v] : images_)
    if (v)
      SceneAPI::RemoveObject(v);

  // Preview must restore the editor scene to the state it had before playback.
  const auto *resolver = GetCurrentSceneObjectResolver();
  for (const auto &[guid, wasVisible] : originalSceneObjectVisibility_) {
    if (!resolver) {
      break;
    }
    if (auto object = resolver->ResolveSceneObject(guid)) {
      object->SetDrawEnable(wasVisible);
    }
  }
  originalSceneObjectVisibility_.clear();
  activeSceneObject_.reset();
  activeSceneObjectHideOnComplete_ = false;
  dialogueText_.reset();
  speakerText_.reset();
  background_.reset();
  images_.clear();
}

void NovelPlayer::PrepareSceneObjects() {
  const auto *resolver = GetCurrentSceneObjectResolver();
  if (!resolver || !scene_) {
    return;
  }

  // Timeline-controlled objects start hidden and are revealed by their clip.
  for (const auto &event : scene_->GetEvents()) {
    if (event.type_ != NovelEventType::SceneObject) {
      continue;
    }
    const auto &clip = std::get<NovelSceneObjectEvent>(event.data_);
    if (auto object = resolver->ResolveSceneObject(clip.sceneObjectGuid_)) {
      originalSceneObjectVisibility_.try_emplace(clip.sceneObjectGuid_,
                                                 object->IsDrawEnable());
      object->SetDrawEnable(false);
    }
  }
}
void NovelPlayer::Advance() {
  if (activeSceneObject_ && activeSceneObjectHideOnComplete_)
    activeSceneObject_->SetDrawEnable(false);
  activeSceneObject_.reset();
  activeSceneObjectHideOnComplete_ = false;
  eventStarted_ = false;
  eventElapsedTime_ = 0;
  autoAdvanceElapsed_ = 0;
  if (++currentEventIndex_ >= scene_->GetEvents().size()) {
    playing_ = false;
    finished_ = true;
  }
}
void NovelPlayer::StartCurrentEvent() {
  if (!scene_ || currentEventIndex_ >= scene_->GetEvents().size()) {
    playing_ = false;
    finished_ = true;
    return;
  }
  eventStarted_ = true;
  const auto &e = scene_->GetEvents()[currentEventIndex_];
  switch (e.type_) {
  case NovelEventType::Dialogue: {
    const auto &d = std::get<NovelDialogueEvent>(e.data_);
    const auto *r = AssetDatabase::GetInstance()->Get(d.dialogueAssetGuid_);
    if (!r || r->type != AssetType::Dialogue ||
        !dialogueAsset_.Load(r->sourcePath)) {
      Advance();
      return;
    }
    const auto *line = dialogueAsset_.FindLine(d.dialogueLineGuid_);
    if (!line) {
      Advance();
      return;
    }
    const auto &settings = scene_->GetTextSettings();
    if (!dialogueText_) {
      dialogueText_ = SceneAPI::Instantiate<TextSceneObject2d>();
      speakerText_ = SceneAPI::Instantiate<TextSceneObject2d>();
    }
    dialogueText_->GetWorldTransform().translation = {
        settings.dialoguePosition_.x, settings.dialoguePosition_.y, 0.0f};
    dialogueText_->GetWorldTransform().scale = {settings.dialogueSize_.x,
                                                settings.dialogueSize_.y, 1.0f};
    speakerText_->GetWorldTransform().translation = {
        settings.speakerPosition_.x, settings.speakerPosition_.y, 0.0f};
    speakerText_->GetWorldTransform().scale = {settings.speakerSize_.x,
                                               settings.speakerSize_.y, 1.0f};
    dialogueText_->SetFontGuid(settings.fontGuid_);
    dialogueText_->SetTextStyle(settings.dialogueStyle_);
    speakerText_->SetFontGuid(settings.fontGuid_);
    speakerText_->SetTextStyle(settings.speakerStyle_);
    speakerText_->SetTypewriter(false);
    speakerText_->SetText(line->speaker_);
    dialogueText_->SetTypewriter(true);
    dialogueText_->SetCharactersPerSecond(line->charactersPerSecond_);
    dialogueText_->SetText(line->text_);
    dialogueText_->Restart();
    break;
  }
  case NovelEventType::ShowImage: {
    const auto &d = std::get<NovelShowImageEvent>(e.data_);
    auto p = ResolveAssetPath(d.textureGuid_, AssetType::Texture);
    if (!p.empty()) {
      auto s = SceneAPI::Instantiate<SpriteObject2d>();
      s->Initialize(p);
      s->SetPosition(d.position_);
      s->SetScale(d.scale_);
      images_[e.guid_] = s;
    }
    Advance();
    break;
  }
  case NovelEventType::HideImage: {
    auto target = std::get<NovelHideImageEvent>(e.data_).targetGuid_;
    auto it = images_.find(target);
    if (it != images_.end()) {
      SceneAPI::RemoveObject(it->second);
      images_.erase(it);
    }
    Advance();
    break;
  }
  case NovelEventType::ChangeBackground: {
    auto p =
        ResolveAssetPath(std::get<NovelBackgroundEvent>(e.data_).textureGuid_,
                         AssetType::Texture);
    if (background_)
      SceneAPI::RemoveObject(background_);
    background_.reset();
    if (!p.empty()) {
      background_ = SceneAPI::Instantiate<SpriteObject2d>();
      background_->Initialize(p);
      background_->SetPosition({960, 540});
    }
    Advance();
    break;
  }
  case NovelEventType::SceneObject: {
    const auto &d = std::get<NovelSceneObjectEvent>(e.data_);
    const auto *resolver = GetCurrentSceneObjectResolver();
    activeSceneObject_ =
        resolver ? resolver->ResolveSceneObject(d.sceneObjectGuid_) : nullptr;
    activeSceneObjectHideOnComplete_ = d.hideOnComplete_;
    if (!activeSceneObject_) {
      Advance();
      return;
    }
    activeSceneObject_->SetDrawEnable(true);
    if (auto text =
            std::dynamic_pointer_cast<TextSceneObject2d>(activeSceneObject_)) {
      text->Restart();
    }
    break;
  }
  case NovelEventType::Wait:
    break;
  }
}
void NovelPlayer::Update(float dt) {
  if (!playing_ || finished_)
    return;
  int guard = 0;
  while (playing_ && !eventStarted_ && guard++ < 64)
    StartCurrentEvent();
  if (!playing_ || !eventStarted_)
    return;
  dt = (std::max)(0.0f, dt);
  const auto &e = scene_->GetEvents()[currentEventIndex_];
  if (e.type_ == NovelEventType::Wait) {
    eventElapsedTime_ += dt;
    if (eventElapsedTime_ >= std::get<NovelWaitEvent>(e.data_).duration_)
      Advance();
  } else if (e.type_ == NovelEventType::SceneObject) {
    const auto &d = std::get<NovelSceneObjectEvent>(e.data_);
    if (d.advanceMode_ == NovelClipAdvanceMode::Time) {
      eventElapsedTime_ += dt;
      if (eventElapsedTime_ >= d.duration_)
        Advance();
    }
  } else if (e.type_ == NovelEventType::Dialogue && dialogueText_ &&
             dialogueText_->IsCompleted()) {
    const auto &d = std::get<NovelDialogueEvent>(e.data_);
    const auto *line = dialogueAsset_.FindLine(d.dialogueLineGuid_);
    if (line && line->autoAdvance_) {
      autoAdvanceElapsed_ += dt;
      if (autoAdvanceElapsed_ >= line->autoAdvanceDelay_)
        Advance();
    }
  }
}
void NovelPlayer::Next() {
  if (!playing_ || !eventStarted_)
    return;
  const auto &e = scene_->GetEvents()[currentEventIndex_];
  if (e.type_ == NovelEventType::Dialogue && dialogueText_ &&
      !dialogueText_->IsCompleted()) {
    dialogueText_->Complete();
    autoAdvanceElapsed_ = 0;
    return;
  }
  if (e.type_ == NovelEventType::SceneObject) {
    const auto &d = std::get<NovelSceneObjectEvent>(e.data_);
    if (d.advanceMode_ != NovelClipAdvanceMode::Input)
      return;
    if (auto text =
            std::dynamic_pointer_cast<TextSceneObject2d>(activeSceneObject_);
        text && !text->IsCompleted()) {
      text->Complete();
      return;
    }
    Advance();
    return;
  }
  if (e.type_ == NovelEventType::Dialogue || e.type_ == NovelEventType::Wait)
    Advance();
}
} // namespace CalyxEngine
