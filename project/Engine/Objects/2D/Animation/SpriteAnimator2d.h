#pragma once
#include <Engine/Foundation/Export/CalyxAPI.h>

#include <Engine\Assets\DataAsset\SpriteAnimationAsset.h>

#include <cstdint>
#include <memory>
#include <string>

class BaseGameObject;
class BaseModel;

namespace CalyxEngine {
	class SpriteObject2d;

	/**
	 * @brief SpriteAnimator2dの機能を提供するクラスです。
	 */
	class CALYX_API SpriteAnimator2d {
	public:
		void Bind(SpriteObject2d* target);
		// モデルオブジェクトを非所有参照で接続する。対象はAnimatorより長く生存させる。
		void BindModel(BaseGameObject* target);
		// 等間隔の分割画像を設定する。番号は左上から横方向に進み、アセットファイルは不要。
		bool SetTextureSheet(const std::string& texturePath, int32_t columns, int32_t rows);
		bool PlayFrames(int32_t startFrame, int32_t frameCount, float frameDuration = 0.1f, bool loop = true);
		void ShowFrame(int32_t frame); // 再生を止め、指定した区画で表示を固定する。
		void SetTexture(const std::string& texturePath); // 再生を止め、画像全体を表示するUVへ戻す。
		void SetAnimationAsset(std::shared_ptr<SpriteAnimationAsset> asset);
		bool Play(const std::string& clipName, bool restart = true);
		void Stop();
		void Reset();
		void Update(float dt);
		void ApplyFrame(int32_t frame);

		void SetReversed(bool reversed) { reversed_ = reversed; }
		void SetLoopOverride(bool loop);
		void ClearLoopOverride();

		bool IsPlaying() const { return playing_; }
		bool IsFinished() const { return finished_; }
		const std::string& GetCurrentClipName() const { return currentClipName_; }
		int32_t GetCurrentFrame() const { return currentFrame_; }
		std::shared_ptr<SpriteAnimationAsset> GetAnimationAsset() const { return asset_; }
		bool IsReversed() const { return reversed_; }
		bool HasLoopOverride() const { return useLoopOverride_; }
		bool GetLoopOverride() const { return loopOverride_; }

	private:
		void ApplyTexture();
		const SpriteAnimationClip* GetCurrentClip() const;
		bool ShouldLoop(const SpriteAnimationClip& clip) const;

	private:
		SpriteObject2d* target_ = nullptr;
		BaseGameObject* modelTarget_ = nullptr;
		BaseModel* appliedModel_ = nullptr; // モデル差し替えの検出専用。所有権は対象オブジェクトが持つ。
		std::shared_ptr<SpriteAnimationAsset> asset_;
		std::string currentClipName_;
		int32_t currentFrame_ = 0;
		float frameTime_ = 0.0f;
		bool playing_ = false;
		bool finished_ = false;
		bool reversed_ = false;
		bool useLoopOverride_ = false;
		bool loopOverride_ = true;
	};

} // namespace CalyxEngine
