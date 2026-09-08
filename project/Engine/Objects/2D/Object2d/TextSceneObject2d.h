#pragma once

#include <Engine/Foundation/Export/CalyxAPI.h>
#include <Engine/Foundation/Math/Vector4.h>
#include <Engine/Objects/2D/Object2d/ITextRenderable.h>
#include <Engine/Objects/3D/Actor/SceneObject.h>
#include <Engine/Objects/ConfigurableObject/IConfigurable.h>
#include <Engine/Renderer/Text/TextStyle.h>

#include <array>
#include <cstddef>
#include <string>

namespace CalyxEngine {
	/*-----------------------------------------------------------------------------------------
	 * TextSceneObject2d
	 * - Scene上に配置可能なスクリーンスペースText Box
	 * - Text内容、レイアウト、簡易Typewriter再生設定を保持する
	 *---------------------------------------------------------------------------------------*/
	class CALYX_API TextSceneObject2d
		: public SceneObject,
		  public IConfigurable,
		  public ITextRenderable {
	public:
		TextSceneObject2d();
		~TextSceneObject2d() override = default;

		void Initialize() override;
		void AlwaysUpdate(float dt) override;
		void ShowGui() override;
		void SubmitText(TextService& service) const override;
		void ApplyConfigFromJson(const nlohmann::json& j) override;
		void ExtractConfigToJson(nlohmann::json& j) const override;
		std::string_view GetObjectClassName() const override { return "TextSceneObject2d"; }

		void Play();
		void Pause();
		void Restart();
		void Stop();
		void SetText(std::string text);
		void SetCharactersPerSecond(float value) noexcept;
		[[nodiscard]] float GetCharactersPerSecond() const noexcept { return charactersPerSecond_; }
		void SetTypewriter(bool value) noexcept;
		[[nodiscard]] bool IsCompleted() const noexcept { return completed_; }
		void Complete() noexcept;
		void			   SetIsTypewriter(bool flag) { typewriter_ = flag; }
		void			   SetIsAutoPlay(bool flag) { autoPlay_ = flag; }
		void			   SetCharacterPerSecond(float second) { charactersPerSecond_ = second; }
		void			   SetFontPath(const std::string& path) { fontPath_ = path; }
		void SetVisibleCharacterCount(size_t count) { visibleCharacterCount_ = count; }
		[[nodiscard]] bool IsComplete() const { return completed_; }
		[[nodiscard]] bool IsTypewriter() const { return typewriter_; }
	private:
		void SyncEditBuffer();
		void UpdatePlayback(float dt);
		[[nodiscard]] size_t CharacterCount() const;
		[[nodiscard]] std::string ResolveFontPath() const;

		std::string text_ = "Text";
		std::array<char, 8192> editBuffer_{};
		Guid fontGuid_{};
		std::string fontPath_ = "Fonts/NotoSansJP-Regular.ttf";
		TextStyle style_{};
		bool typewriter_ = false;
		bool autoPlay_ = false;
		bool loop_ = false;
		bool playing_ = false;
		bool completed_ = false;
		bool useDuration_ = false;
		float duration_ = 3.0f;
		float charactersPerSecond_ = 20.0f;
		float startDelay_ = 0.0f;
		float elapsedTime_ = 0.0f;
		size_t visibleCharacterCount_ = 0;
	};
}
