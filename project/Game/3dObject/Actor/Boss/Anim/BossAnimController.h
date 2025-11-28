#pragma once

#include <Engine/Assets/Animation/AnimationModel.h>

/**
 * \brief ボスのアニメーション制御クラス
 * AnimationModel への Facade（仲介クラス）
 */
class BossAnimController {
public:
	// コンストラクタ
	BossAnimController(AnimationModel* animModel);
	~BossAnimController();

	/**
	 * \brief 初期化
	 */
	void Initialize()const;
	/**
	 * @brief アニメーションの登録
	 * @param id
	 * @param name
	 * @param fileName ファイル名（省略時は name と同じ）
	 */
	void Register(int16_t id, const std::string& name,
				  const std::optional<std::string>& fileName = std::nullopt)const;
	/**
	 * @brief アニメーション再生
	 * @param id
	 * @param blend
	 */
	void Play(int16_t id, float blend = 0.2f)const;
	/**
	 * @brief ワンショットアニメーション再生
	 * @param id 
	 * @param returnId 
	 * @param blend 
	 */
	void PlayOneShot(int16_t id, int16_t returnId, float blend = 0.1f)const;
	/**
	 * @brief ループ設定
	 * @param id 
	 * @param isLoop 
	 */
	void SetLoop(int16_t id, bool isLoop)const;

	// getter
	std::string GetCurrentAnimName() const;

private:
	AnimationModel* animModel_;
};
