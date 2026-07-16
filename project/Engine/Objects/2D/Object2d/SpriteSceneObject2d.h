#pragma once

#include <Engine\Assets\DataAsset\SpriteAnimationAsset.h>
#include <Engine\Foundation\Math\Vector2.h>
#include <Engine\Foundation\Math\Vector4.h>
#include <Engine\Objects\2D\Animation\SpriteAnimator2d.h>
#include <Engine\Objects\2D\Object2d\SpriteObject2d.h>
#include <Engine\Objects\2D\Object2d\ISpriteRenderable.h>
#include <Engine\Objects\3D\Actor\SceneObject.h>
#include <Engine\Objects\ConfigurableObject\IConfigurable.h>

#include <memory>
#include <string>

class SpriteRenderer;

namespace CalyxEngine {

	class SpriteSceneObject2d
		: public SceneObject,
		  public IConfigurable,
		  public ISpriteRenderable {
	public:
		SpriteSceneObject2d();
		~SpriteSceneObject2d() override = default;

		void Initialize() override;
		void AlwaysUpdate(float dt) override;
		void ShowGui() override;
		void DrawSprite(SpriteRenderer* renderer) const;
		void SubmitSprites(SpriteRenderer& renderer) const override;
		void ApplyConfigFromJson(const nlohmann::json& j) override;
		void ExtractConfigToJson(nlohmann::json& j) const override;

		std::string_view GetObjectClassName() const override { return "SpriteSceneObject2d"; }
		const Vector2& GetAnchor() const { return anchor_; }
		void SetAnchor(const Vector2& anchor);

	protected:
		void EnsureSprite();
		void SyncSpriteFromTransform();
		void DrawBaseGui();
		void ApplyTextureByGuid(const Guid& guid);
		std::string ResolveTexturePath() const;

	protected:
		std::unique_ptr<SpriteObject2d> sprite_;
		Guid textureGuid_;
		std::string texturePath_ = "Textures/white1x1.dds";
		Vector2 anchor_ = {0.0f, 0.0f};
		Vector4 color_ = {1.0f, 1.0f, 1.0f, 1.0f};
	};

	class AnimatedSpriteSceneObject2d
		: public SpriteSceneObject2d {
	public:
		AnimatedSpriteSceneObject2d();
		~AnimatedSpriteSceneObject2d() override = default;

		void Initialize() override;
		void Update(float dt) override;
		void AlwaysUpdate(float dt) override;
		void ShowGui() override;
		void ApplyConfigFromJson(const nlohmann::json& j) override;
		void ExtractConfigToJson(nlohmann::json& j) const override;

		std::string_view GetObjectClassName() const override { return "AnimatedSpriteSceneObject2d"; }

	private:
		void SetAnimationGuid(const Guid& guid);
		void RefreshAnimationAsset();
		void PlayCurrentClip(bool restart);
		void UpdateAnimatedSprite(float dt, bool updateAnimation);
		void ApplyLoopSettings();
		void DrawAnimationAssetEditor(SpriteAnimationAsset& asset);
		void SaveAnimationAsset(const SpriteAnimationAsset& asset);
		void CreateAnimationAsset();

	private:
		SpriteAnimator2d animator_;
		Guid animationGuid_;
		Guid boundAnimationGuid_;
		std::string clipName_;
		bool autoPlay_ = true;
		bool useLoopOverride_ = false;
		bool loopOverride_ = true;
		int selectedClipIndex_ = 0;
	};

} // namespace CalyxEngine
