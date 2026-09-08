#include "TextSceneObject2d.h"

#include <Engine/Application/UI/Panels/AssetPanel.h>
#include <Engine/Application/UI/Panels/InspectorPanel.h>
#include <Engine/Assets/Database/AssetDatabase.h>
#include <Engine/Assets/System/AssetRecord.h>
#include <Engine/Foundation/Utility/Text/Utf8Decoder.h>
#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>
#include <Engine/Renderer/Text/TextService.h>
#include <Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h>
#include <externals/imgui/imgui.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <limits>

namespace CalyxEngine {
	namespace {
		std::string RelativeAssetPath(const std::filesystem::path& path) {
			auto* database = AssetDatabase::GetInstance();
			if(!database) return path.generic_string();
			std::error_code error;
			auto relative = std::filesystem::relative(path, database->GetRoot(), error);
			return error ? path.generic_string() : relative.generic_string();
		}
	}

	TextSceneObject2d::TextSceneObject2d() {
		SetName("Text2D", ObjectType::Object2D);
		SetCastShadow(false);
		worldTransform_.translation = {100.0f, 100.0f, 0.0f};
		worldTransform_.scale = {600.0f, 180.0f, 1.0f};
		style_.fontSize_ = 32.0f;
		style_.wordWrap_ = true;
		SyncEditBuffer();
	}

	void TextSceneObject2d::Initialize() {
		worldTransform_.Update();
		if(autoPlay_ && typewriter_) Restart();
	}

	void TextSceneObject2d::AlwaysUpdate(float dt) {
		transformAnimation2d_.Update(worldTransform_, dt);
		worldTransform_.Update();
		UpdatePlayback(dt);
	}

	void TextSceneObject2d::SetText(std::string text) {
		text_ = std::move(text);
		SyncEditBuffer();
		visibleCharacterCount_ = typewriter_ ? 0 : CharacterCount();
		completed_ = !typewriter_;
	}

	void TextSceneObject2d::SyncEditBuffer() {
		editBuffer_.fill('\0');
		const size_t count = (std::min)(text_.size(), editBuffer_.size() - 1);
		std::memcpy(editBuffer_.data(), text_.data(), count);
	}

	size_t TextSceneObject2d::CharacterCount() const {
		const auto view = std::u8string_view(reinterpret_cast<const char8_t*>(text_.data()), text_.size());
		return Utf8Decoder::Decode(view).size();
	}

	void TextSceneObject2d::Play() { playing_ = true; }
	void TextSceneObject2d::Pause() { playing_ = false; }
	void TextSceneObject2d::Restart() {
		elapsedTime_ = 0.0f;
		visibleCharacterCount_ = 0;
		completed_ = false;
		playing_ = true;
	}
	void TextSceneObject2d::Stop() {
		playing_ = false;
		elapsedTime_ = 0.0f;
		visibleCharacterCount_ = typewriter_ ? 0 : CharacterCount();
		completed_ = !typewriter_;
	}

	void TextSceneObject2d::UpdatePlayback(float dt) {
		const size_t count = CharacterCount();
		if(!typewriter_) {
			visibleCharacterCount_ = count;
			completed_ = true;
			return;
		}
		if(!playing_) return;
		elapsedTime_ += (std::max)(0.0f, dt);
		const float revealTime = (std::max)(0.0f, elapsedTime_ - startDelay_);
		const float rate = useDuration_
			? (duration_ > 0.0f ? static_cast<float>(count) / duration_ : static_cast<float>(count))
			: (std::max)(0.01f, charactersPerSecond_);
		visibleCharacterCount_ = (std::min)(count, static_cast<size_t>(std::floor(revealTime * rate)));
		completed_ = visibleCharacterCount_ >= count;
		if(completed_) {
			if(loop_) Restart();
			else playing_ = false;
		}
	}

	std::string TextSceneObject2d::ResolveFontPath() const {
		if(fontGuid_.isValid()) {
			if(const AssetRecord* record = AssetDatabase::GetInstance()->Get(fontGuid_);
			   record && record->type == AssetType::Font) return RelativeAssetPath(record->sourcePath);
		}
		return fontPath_;
	}

	void TextSceneObject2d::SubmitText(TextService& service) const {
		if(!IsDrawEnable() || text_.empty()) return;
		const FontHandle font = service.LoadFont(ResolveFontPath());
		if(!font) return;
		TextStyle drawStyle = style_;
		drawStyle.maxWidth_ = (std::max)(0.0f, worldTransform_.scale.x);
		const auto view = std::u8string_view(reinterpret_cast<const char8_t*>(text_.data()), text_.size());
		const size_t visible = typewriter_ ? visibleCharacterCount_ : (std::numeric_limits<size_t>::max)();
		service.Draw(font, view, {worldTransform_.translation.x, worldTransform_.translation.y}, drawStyle, visible);
	}

	void TextSceneObject2d::ShowGui() {
		if(GuiCmd::BeginSection(ParamFilterSection::Object)) {
			worldTransform_.ShowImGui("Text Box Transform");
			GuiCmd::EndSection();
		}
		if(GuiCmd::BeginSection(ParamFilterSection::ParameterData)) {
			if(ImGui::InputTextMultiline("Text", editBuffer_.data(), editBuffer_.size(), ImVec2(-1.0f, 120.0f))) {
				text_ = editBuffer_.data();
				if(!typewriter_) visibleCharacterCount_ = CharacterCount();
			}
			Guid dropped = fontGuid_;
			if(AssetPanel::DrawAssetDropTarget(AssetType::Font, &dropped)) {
				fontGuid_ = dropped;
				if(const AssetRecord* record = AssetDatabase::GetInstance()->Get(fontGuid_)) fontPath_ = RelativeAssetPath(record->sourcePath);
			}
			ImGui::TextDisabled("Font: %s", ResolveFontPath().c_str());
			ImGui::DragFloat("Font Size", &style_.fontSize_, 1.0f, 1.0f, 256.0f, "%.0f px");
			ImGui::ColorEdit4("Color", &style_.color_.x);
			ImGui::DragFloat("Letter Spacing", &style_.letterSpacing_, 0.1f, -20.0f, 100.0f);
			ImGui::DragFloat("Line Spacing", &style_.lineSpacing_, 0.01f, 0.1f, 5.0f);
			ImGui::Checkbox("Auto Wrap", &style_.wordWrap_);
			ImGui::SeparatorText("Playback");
			if(ImGui::Checkbox("Typewriter", &typewriter_)) Stop();
			ImGui::Checkbox("Auto Play", &autoPlay_);
			ImGui::Checkbox("Loop", &loop_);
			ImGui::DragFloat("Start Delay", &startDelay_, 0.05f, 0.0f, 60.0f, "%.2f sec");
			ImGui::Checkbox("Use Duration", &useDuration_);
			if(useDuration_) ImGui::DragFloat("Reveal Duration", &duration_, 0.05f, 0.01f, 600.0f, "%.2f sec");
			else ImGui::DragFloat("Characters / Sec", &charactersPerSecond_, 0.5f, 0.1f, 1000.0f);
			if(ImGui::Button(playing_ ? "Pause" : "Play")) playing_ ? Pause() : Play();
			ImGui::SameLine();
			if(ImGui::Button("Restart")) Restart();
			ImGui::SameLine();
			if(ImGui::Button("Stop")) Stop();
			const size_t count = CharacterCount();
			int visible = static_cast<int>((std::min)(visibleCharacterCount_, count));
			if(ImGui::SliderInt("Visible Characters", &visible, 0, static_cast<int>(count))) {
				visibleCharacterCount_ = static_cast<size_t>(visible);
				playing_ = false;
			}
			GuiCmd::EndSection();
		}
	}

	void TextSceneObject2d::ApplyConfigFromJson(const nlohmann::json& j) {
		id_ = j.value("guid", id_);
		name_ = j.value("name", name_);
		parentId_ = j.value("parentGuid", parentId_);
		text_ = j.value("text", text_);
		fontGuid_ = j.value("fontGuid", fontGuid_);
		fontPath_ = j.value("fontPath", fontPath_);
		style_.fontSize_ = j.value("fontSize", style_.fontSize_);
		style_.color_ = j.value("color", style_.color_);
		style_.letterSpacing_ = j.value("letterSpacing", style_.letterSpacing_);
		style_.lineSpacing_ = j.value("lineSpacing", style_.lineSpacing_);
		style_.wordWrap_ = j.value("autoWrap", style_.wordWrap_);
		typewriter_ = j.value("typewriter", typewriter_);
		autoPlay_ = j.value("autoPlay", autoPlay_);
		loop_ = j.value("loop", loop_);
		useDuration_ = j.value("useDuration", useDuration_);
		duration_ = j.value("duration", duration_);
		charactersPerSecond_ = j.value("charactersPerSecond", charactersPerSecond_);
		startDelay_ = j.value("startDelay", startDelay_);
		if(j.contains("transform")) worldTransform_.ApplyConfig(j.at("transform").get<WorldTransformConfig>());
		if(j.contains("transformAnimation2d")) transformAnimation2d_.ApplyConfigFromJson(j.at("transformAnimation2d"));
		SyncEditBuffer();
		Stop();
		if(autoPlay_ && typewriter_) Restart();
	}

	void TextSceneObject2d::ExtractConfigToJson(nlohmann::json& j) const {
		j["name"] = name_;
		j["guid"] = id_;
		j["parentGuid"] = parentId_;
		j["transform"] = const_cast<WorldTransform&>(worldTransform_).ExtractConfig();
		j["text"] = text_;
		j["fontGuid"] = fontGuid_;
		j["fontPath"] = fontPath_;
		j["fontSize"] = style_.fontSize_;
		j["color"] = style_.color_;
		j["letterSpacing"] = style_.letterSpacing_;
		j["lineSpacing"] = style_.lineSpacing_;
		j["autoWrap"] = style_.wordWrap_;
		j["typewriter"] = typewriter_;
		j["autoPlay"] = autoPlay_;
		j["loop"] = loop_;
		j["useDuration"] = useDuration_;
		j["duration"] = duration_;
		j["charactersPerSecond"] = charactersPerSecond_;
		j["startDelay"] = startDelay_;
		if(!transformAnimation2d_.IsEmpty()) {
			nlohmann::json animation;
			transformAnimation2d_.ExtractConfigToJson(animation);
			j["transformAnimation2d"] = animation;
		}
	}
}

namespace {
	const bool registerTextSceneObject2d = [] {
		SceneObjectRegistry::Get().Register(
			"TextSceneObject2d", "TextSceneObject2d", ObjectType::Object2D, "",
			false, false, false, true, true, &CreateSceneObject<CalyxEngine::TextSceneObject2d>);
		return true;
	}();
}
