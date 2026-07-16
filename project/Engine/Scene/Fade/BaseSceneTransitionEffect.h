#pragma once

// engine
#include <Engine/Foundation/Export/CalyxAPI.h>
#include <Engine/Foundation/Serialization/SerializableObject.h>

namespace CalyxEngine {

	/*-----------------------------------------------------------------------------------------
	 * BaseTransitionEffectParam
	 * - シーン遷移効果のパラメータを管理する構造体
	 * - 遷移効果の持続時間や使用するテクスチャなどを定義する
	 *---------------------------------------------------------------------------------------*/
	struct BaseTransitionEffectParam
		: SerializableObject {

		BaseTransitionEffectParam();
		ParamPath GetParamPath() const override;

		float		time_		 = 1.0f;		   //< 遷移効果の持続時間（秒）
		std::string textureName_ = "white1x1.png"; //< 遷移効果に使用するテクスチャのパス
	};

	/*-----------------------------------------------------------------------------------------
	 * BaseSceneTransitionEffect
	 * - シーン遷移効果を管理するクラス
	 * - シーン遷移時に適用されるエフェクトやアニメーションを定義する
	 *---------------------------------------------------------------------------------------*/
	class CALYX_API BaseSceneTransitionEffect {
	public:
		//===================================================================*/
		//                    public methods
		//===================================================================*/
		BaseSceneTransitionEffect()	 = default;
		~BaseSceneTransitionEffect() = default;

		// フェードイン処理 =====================================================//
		/**
		 * \brief フェードイン更新
		 * \param dt デルタタイム
		 */
		void FadeInUpdate(float dt);

		/**
		 * \brief フェードイン開始
		 */
		virtual void StartFadeIn();

		/**
		 * \brief フェードイン終了
		 */
		virtual void OnEndFadeIn();

		// フェードアウト処理 =====================================================//
		/**
		 * \brief フェードアウト更新
		 * \param dt デルタタイム
		 */
		void FadeOutUpdate(float dt);

		/**
		 * \brief フェードアウト開始
		 */
		virtual void StartFadeOut();

		/**
		 * \brief フェードアウト終了
		 */
		virtual void OnEndFadeOut();

		// accessors =====================================================//
		// getter
		const BaseTransitionEffectParam& GetParam() const { return param_; }

		// setter
		void SetTextureName(const std::string& textureName) { param_.textureName_ = textureName; }
		void SetTime(float time) { param_.time_ = time; }

	private:
		//===================================================================*/
		//                    private methods
		//===================================================================*/
		BaseTransitionEffectParam param_; //< 遷移効果のパラメータ

		bool isFadingIn_  = false; //< フェードイン中か
		bool isFadingOut_ = false; //< フェードアウト中か
	};

} // namespace CalyxEngine