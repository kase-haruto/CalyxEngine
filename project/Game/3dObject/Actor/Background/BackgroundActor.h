#pragma once
// ===================================================================== */
//  include space
// ===================================================================== */
#include <Engine/Objects/3D/Actor/Actor.h>
#include <Engine/Application/Effects/FxObject.h>
#include <Engine/Foundation/Serialization/SerializableObject.h>
// externals / stl
#include <externals/imgui/imgui.h>
#include <externals/nlohmann/json.hpp>
#include <optional>
#include <string>
#include <string_view>

/*-----------------------------------------------------------------------------------------
 * BackgroundActor
 * - 背景オブジェクトの基底クラス
 * - 背景オブジェクトの共通インターフェースや基本機能を提供
 * - 具体的な背景オブジェクトはこのクラスを継承して実装する
 *---------------------------------------------------------------------------------------*/
class BackgroundActor
	: public Actor {
public:
	//===================================================================*/
	//			public methods
	//===================================================================*/
	/**
	 * \brief コンストラクタ
	 * \param modelName モデル名
	 * \param objectName オブジェクト名
	 */
	BackgroundActor(const std::string&         modelName,
					std::optional<std::string> objectName = std::nullopt);

	BackgroundActor();
	~BackgroundActor() override;

	void Initialize() override;

	void Update(float dt) override;

	/**
	 * \brief 派生クラスのGUI表示
	 */
	void DerivativeGui() override;

	/**
	 * \brief 進捗停止フラグを取得
	 * \return 停止するか
	 */
	bool IsStopRail() const { return isStopRail_; }
	/**
	 * \brief 停止進捗を取得
	 * \return 停止進捗
	 */
	float GetStopProgress() const { return stopProgress_; }
	/**
	 * \brief 停止オフセットを取得
	 * \return オフセット距離
	 */
	float GetStopOffset() const { return stopOffset_; }

	/**
	 * \brief 進捗停止フラグを設定
	 * \param stop 停止するか
	 */
	void SetStopRail(bool stop) { isStopRail_ = stop; }
	/**
	 * \brief 停止進捗を設定
	 * \param progress 停止進捗
	 */
	void SetStopProgress(float progress) { stopProgress_ = progress; }
	/**
	 * \brief 停止オフセットを設定
	 * \param offset オフセット距離
	 */
	void SetStopOffset(float offset) { stopOffset_ = offset; }

	/**
	 * \brief 自動計算フラグを取得
	 * \return 自動計算するか
	 */
	bool IsAutoCalculateProgress() const { return autoCalculateProgress_; }
	/**
	 * \brief 自動計算フラグを設定
	 * \param autoCalc 自動計算するか
	 */
	void SetAutoCalculateProgress(bool autoCalc) { autoCalculateProgress_ = autoCalc; }

	/**
	 * \brief タイプ名を取得
	 * \return タイプ名
	 */
	std::string_view GetTypeName() const override { return "BackgroundActor"; }

	void OnCollisionEnter(Collider* other) override;

protected:
	/**
	 * \brief JSONから派生設定を適用
	 * \param root ルートJSON
	 * \param derived 派生設定JSON
	 */
	void ApplyDerivedConfigFromJson(const nlohmann::json& root,const nlohmann::json* derived) override;
	/**
	 * \brief 派生設定をJSONに抽出
	 * \param root ルートJSON
	 * \param derived 派生設定JSON
	 */
	void ExtractDerivedConfigToJson(nlohmann::json& root,nlohmann::json& derived) const override;

private:
	bool  isStopRail_            = false; //< レールを止めるか
	float stopProgress_          = 0.0f;  //< 止める進捗
	float stopOffset_            = 2.0f;  //< 止める際の手前へのオフセット
	bool  autoCalculateProgress_ = true;  //< 自動計算するか
	float fadeElapsed_           = 0.0f;  //< 死亡フェードの経過時間

	struct BackGroundActorData
		: public CalyxEngine::SerializableObject {

		BackGroundActorData();
		CalyxEngine::ParamPath GetParamPath() const override;


		int32_t fadeEaseType = static_cast<int32_t>(CalyxEase::EaseType::EaseInQuad); //< フェードのイージングタイプ
		float   fadeDuration = 1.0f;                                                  //< フェードの時間
	}param_;

	std::weak_ptr<CalyxEffect::FxObject> hitEffects_; //< ヒットエフェクト群
};