#pragma once
#include <Engine/Foundation/Export/CalyxAPI.h>

// engine
#include <Engine/Foundation/Math/Vector2.h>
#include <Engine/Objects/2D/Object2d/ISpriteRenderable.h>
#include <Engine/Objects/3D/Actor/SceneObject.h>

// stde
#include "Engine/Foundation/Math/Vector4.h"

#include <memory>
#include <string>

class SpriteRenderer;
class Sprite;

namespace CalyxEngine {

	/*-----------------------------------------------------------------------------------------
	 * SpriteObject2d
	 * - 2Dスプライトオブジェクトクラス
	 * - spriteクラスを使いやすくしたラッパ
	 *---------------------------------------------------------------------------------------*/
	/**
	 * @brief 1つのSpriteを所有するRuntime向け2Dシーンオブジェクト。
	 *
	 * Instantiateで生成したインスタンスはSceneObjectLibraryが所有する。
	 * このクラスはSpriteの更新とフレーム単位の描画登録を担当するが、Rendererの
	 * 所有やシリアライズ対象の管理は行わない。SceneObjectRegistryへ別途登録されない
	 * RuntimeOwnedインスタンスは、シーン保存対象にならない。
	 */
	class CALYX_API SpriteObject2d : public SceneObject, public ISpriteRenderable {
	public:
		/** \brief コンストラクタ・デストラクタ */
		SpriteObject2d();
		~SpriteObject2d() override;

		void Initialize() override;

		/**
		 * \brief 初期化処理
		 * \param filePath
		 */
		void Initialize(const std::string& filePath);
		/**
		 * \brief 更新処理
		 * \param dt デルタタイム
		 */
		void Update(float dt) override;
		/**
		 * \brief 描画処理
		 * \param renderer レンダラー
		 */
		void Draw(SpriteRenderer* renderer) const;

		/**
		 * \brief デバッグGui
		 */
		void ShowGui() override;
		void SubmitSprites(SpriteRenderer& renderer) const override;
		std::string_view GetObjectClassName() const override { return "SpriteObject2d"; }

	public:
		// getter
		const CalyxEngine::Vector2&		   GetPosition() const;
		const CalyxEngine::Vector2&		   GetScale() const;
		Sprite*							   GetSprite() const;
		CalyxEngine::Vector2 GetUvTranslate() const;
		float GetUvRotate() const;

		// setter
		void SetTexture(const std::string& texturePath) const;
		/** 分割画像を設定して区画0を固定表示する。不正な分割数なら変更せずfalseを返す。 */
		bool SetTextureSheet(const std::string& texturePath, int32_t columns, int32_t rows = 1) const;
		/** 現在の画像の分割数を設定する。画像の再読み込みや時間による再生は行わない。 */
		bool SetTextureGrid(int32_t columns, int32_t rows = 1) const;
		/** 左上から横方向に数えた0始まりの区画を固定表示する。範囲外は端に丸める。 */
		void SetTextureFrame(int32_t frame) const;
		void SetPosition(const CalyxEngine::Vector2& position) const;
		void SetScale(const CalyxEngine::Vector2& scale) const;
		void SetRotation(float rotation) const;
		void SetAlpha(float alpha) const;
		void SetColor(const CalyxEngine::Vector4 & color) const;
		void SetVisibility(bool visible) const;
		void SetAnchorPoint(const CalyxEngine::Vector2& anchor) const;
		void SetUvTranslate(const CalyxEngine::Vector2& uv) const;
		void SetUvRotate(float rot) const;
		void SetUvScale(const CalyxEngine::Vector2& scale) const;
		void SetUvOffset(const CalyxEngine::Vector2& offset) const; // 加算したいなら

		void SetFillAmount(float fillAmount) const;
		void SetFillOrigin(float x,float y);
		void SetFillMethod(int method);

	private:
		std::unique_ptr<Sprite> sprite_ = nullptr;
	};

} // namespace CalyxEngine
