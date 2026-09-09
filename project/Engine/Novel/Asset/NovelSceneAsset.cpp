#include "NovelSceneAsset.h"

#include <algorithm>
#include <externals/nlohmann/json.hpp>
#include <fstream>

namespace CalyxEngine {
namespace {
const char *TypeName(NovelEventType type) {
  switch (type) {
  case NovelEventType::Dialogue:
    return "Dialogue";
  case NovelEventType::ShowImage:
    return "ShowImage";
  case NovelEventType::HideImage:
    return "HideImage";
  case NovelEventType::ChangeBackground:
    return "ChangeBackground";
  case NovelEventType::Wait:
    return "Wait";
  case NovelEventType::SceneObject:
    return "SceneObject";
  }
  return "Dialogue";
}
NovelEventType ParseType(const std::string &value) {
  if (value == "ShowImage")
    return NovelEventType::ShowImage;
  if (value == "HideImage")
    return NovelEventType::HideImage;
  if (value == "ChangeBackground")
    return NovelEventType::ChangeBackground;
  if (value == "Wait")
    return NovelEventType::Wait;
  if (value == "SceneObject")
    return NovelEventType::SceneObject;
  return NovelEventType::Dialogue;
}
nlohmann::json Serialize(const NovelEvent &event) {
  nlohmann::json data;
  std::visit(
      [&](const auto &value) {
        using T = std::decay_t<decltype(value)>;
        if constexpr (std::is_same_v<T, NovelDialogueEvent>)
          data = {{"dialogueAssetGuid", value.dialogueAssetGuid_},
                  {"dialogueLineGuid", value.dialogueLineGuid_}};
        else if constexpr (std::is_same_v<T, NovelShowImageEvent>)
          data = {{"textureGuid", value.textureGuid_},
                  {"position", value.position_},
                  {"scale", value.scale_}};
        else if constexpr (std::is_same_v<T, NovelHideImageEvent>)
          data = {{"targetGuid", value.targetGuid_}};
        else if constexpr (std::is_same_v<T, NovelBackgroundEvent>)
          data = {{"textureGuid", value.textureGuid_}};
        else if constexpr (std::is_same_v<T, NovelWaitEvent>)
          data = {{"duration", value.duration_}};
        else
          data = {{"sceneObjectGuid", value.sceneObjectGuid_},
                  {"advanceMode",
                   value.advanceMode_ == NovelClipAdvanceMode::Time ? "Time"
                                                                    : "Input"},
                  {"duration", value.duration_},
                  {"hideOnComplete", value.hideOnComplete_}};
      },
      event.data_);
  return {{"guid", event.guid_},
          {"type", TypeName(event.type_)},
          {"data", std::move(data)}};
}
NovelEvent Deserialize(const nlohmann::json &json) {
  NovelEvent event;
  event.guid_ = json.value("guid", Guid::New());
  if (!event.guid_.isValid())
    event.guid_ = Guid::New();
  event.type_ = ParseType(json.value("type", std::string{"Dialogue"}));
  const auto &d = json.value("data", nlohmann::json::object());
  switch (event.type_) {
  case NovelEventType::Dialogue:
    event.data_ = NovelDialogueEvent{d.value("dialogueAssetGuid", Guid{}),
                                     d.value("dialogueLineGuid", Guid{})};
    break;
  case NovelEventType::ShowImage:
    event.data_ = NovelShowImageEvent{d.value("textureGuid", Guid{}),
                                      d.value("position", Vector2{}),
                                      d.value("scale", Vector2{1, 1})};
    break;
  case NovelEventType::HideImage:
    event.data_ = NovelHideImageEvent{d.value("targetGuid", Guid{})};
    break;
  case NovelEventType::ChangeBackground:
    event.data_ = NovelBackgroundEvent{d.value("textureGuid", Guid{})};
    break;
  case NovelEventType::Wait:
    event.data_ = NovelWaitEvent{(std::max)(0.0f, d.value("duration", 1.0f))};
    break;
  case NovelEventType::SceneObject:
    event.data_ = NovelSceneObjectEvent{
        d.value("sceneObjectGuid", Guid{}),
        d.value("advanceMode", std::string{"Input"}) == "Time"
            ? NovelClipAdvanceMode::Time
            : NovelClipAdvanceMode::Input,
        (std::max)(0.0f, d.value("duration", 1.0f)),
        d.value("hideOnComplete", false)};
    break;
  }
  return event;
}

nlohmann::json SerializeTextStyle(const TextStyle &style) {
  return {
      {"fontSize", style.fontSize_},
      {"color", style.color_},
      {"letterSpacing", style.letterSpacing_},
      {"lineSpacing", style.lineSpacing_},
      {"maxWidth", style.maxWidth_},
      {"wordWrap", style.wordWrap_},
  };
}

void DeserializeTextStyle(const nlohmann::json &json, TextStyle &style) {
  style.fontSize_ = json.value("fontSize", style.fontSize_);
  style.color_ = json.value("color", style.color_);
  style.letterSpacing_ = json.value("letterSpacing", style.letterSpacing_);
  style.lineSpacing_ = json.value("lineSpacing", style.lineSpacing_);
  style.maxWidth_ = json.value("maxWidth", style.maxWidth_);
  style.wordWrap_ = json.value("wordWrap", style.wordWrap_);
}
} // namespace
NovelEvent *NovelSceneAsset::FindEvent(const Guid &guid) noexcept {
  auto it = std::ranges::find(events_, guid, &NovelEvent::guid_);
  return it == events_.end() ? nullptr : &*it;
}
const NovelEvent *NovelSceneAsset::FindEvent(const Guid &guid) const noexcept {
  auto it = std::ranges::find(events_, guid, &NovelEvent::guid_);
  return it == events_.end() ? nullptr : &*it;
}
NovelEvent &NovelSceneAsset::AddEvent(NovelEventType type) {
  return events_.emplace_back(
      NovelEvent{Guid::New(), type, MakeNovelEventData(type)});
}
bool NovelSceneAsset::RemoveEvent(const Guid &guid) {
  auto it = std::ranges::find(events_, guid, &NovelEvent::guid_);
  if (it == events_.end())
    return false;
  events_.erase(it);
  return true;
}
bool NovelSceneAsset::MoveEvent(const Guid &guid, int offset) {
  auto it = std::ranges::find(events_, guid, &NovelEvent::guid_);
  if (it == events_.end())
    return false;
  auto i = std::distance(events_.begin(), it);
  auto t = i + offset;
  if (t < 0 || t >= static_cast<std::ptrdiff_t>(events_.size()))
    return false;
  std::iter_swap(events_.begin() + i, events_.begin() + t);
  return true;
}
bool NovelSceneAsset::Load(const std::filesystem::path &path) {
  try {
    std::ifstream s(path);
    if (!s)
      return false;
    nlohmann::json j;
    s >> j;

    // Missing settings are accepted for compatibility with version 1 assets.
    const auto settings = j.value("textSettings", nlohmann::json::object());
    textSettings_.fontGuid_ =
        settings.value("fontGuid", textSettings_.fontGuid_);
    textSettings_.dialoguePosition_ =
        settings.value("dialoguePosition", textSettings_.dialoguePosition_);
    textSettings_.dialogueSize_ =
        settings.value("dialogueSize", textSettings_.dialogueSize_);
    textSettings_.speakerPosition_ =
        settings.value("speakerPosition", textSettings_.speakerPosition_);
    textSettings_.speakerSize_ =
        settings.value("speakerSize", textSettings_.speakerSize_);
    DeserializeTextStyle(
        settings.value("dialogueStyle", nlohmann::json::object()),
        textSettings_.dialogueStyle_);
    DeserializeTextStyle(
        settings.value("speakerStyle", nlohmann::json::object()),
        textSettings_.speakerStyle_);

    std::vector<NovelEvent> loaded;
    for (const auto &v : j.value("events", nlohmann::json::array()))
      loaded.push_back(Deserialize(v));
    events_ = std::move(loaded);
    return true;
  } catch (...) {
    return false;
  }
}
bool NovelSceneAsset::Save(const std::filesystem::path &path) const {
  try {
    if (!path.parent_path().empty())
      std::filesystem::create_directories(path.parent_path());
    nlohmann::json a = nlohmann::json::array();
    for (const auto &e : events_)
      a.push_back(Serialize(e));

    const nlohmann::json settings = {
        {"fontGuid", textSettings_.fontGuid_},
        {"dialoguePosition", textSettings_.dialoguePosition_},
        {"dialogueSize", textSettings_.dialogueSize_},
        {"speakerPosition", textSettings_.speakerPosition_},
        {"speakerSize", textSettings_.speakerSize_},
        {"dialogueStyle", SerializeTextStyle(textSettings_.dialogueStyle_)},
        {"speakerStyle", SerializeTextStyle(textSettings_.speakerStyle_)},
    };
    nlohmann::json j = {
        {"version", 2},
        {"textSettings", settings},
        {"events", std::move(a)},
    };
    std::ofstream s(path);
    if (!s)
      return false;
    s << j.dump(2);
    return bool(s);
  } catch (...) {
    return false;
  }
}
} // namespace CalyxEngine
