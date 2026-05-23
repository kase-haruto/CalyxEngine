#pragma once
#include <Engine/Renderer/Sprite/Sprite.h>
#include <Game/Scene/Details/SceneType.h>
#include <Engine/Objects/2D/Object2d/SpriteObject2d.h>
#include <Engine/Objects/2D/Animation/SpriteAnimator2d.h>
#include <functional>
#include <memory>
#include <string>
#include <vector>

enum class TransitionState {
	Idle,	 // 何もしない
	Closing, // 画面を隠していく
	Full,	 // 完全に隠れた
	Opening	 // 画面を開いていく
};

/// <summary>
/// TransitionControl
/// </summary>
class TransitionControl {
public:
	TransitionControl();
	~TransitionControl();

	/// <summary>
	/// 初期化
	/// </summary>
	/// <param name="texPath1">1枚目</param>
	/// <param name="texPath2">2枚目分割(スライド等)</param>
	/// <param name="texPath3">3枚目分割(スライド等)</param>
	void Initialize(const std::string& texPath1, const std::string& texPath2 = "", const std::string& texPath3 = "");
	void Update(float dt);
	void Draw(class SpriteRenderer* renderer);

	/// <summary>
	/// 画面を閉じる演出を開始
	/// </summary>
	/// <param name="duration">時間</param>
	/// <param name="onCovered">完了時（画面が隠れた時）のコールバック</param>
	void StartClosing(float duration, std::function<void()> onCovered = nullptr);

	/// <summary>
	/// 画面を開く演出を開始
	/// </summary>
	/// <param name="duration">時間</param>
	/// <param name="onOpened">完了時（画面が露出した時）のコールバック</param>
	void StartOpening(float duration, std::function<void()> onOpened = nullptr);
	
	/// <summary>
	/// カスタム演出用の更新関数をセット
	/// </summary>
	/// <param name="func">(progress[0-1], plate1, plate2) </param>
	void SetUpdateFunc(std::function<void(float progress, CalyxEngine::SpriteObject2d* plate1, CalyxEngine::SpriteObject2d* plate2, CalyxEngine::SpriteObject2d* plate3)> func) {
		updateFunc_ = func;
	}

	/// <summary>
	/// スライド演出をセット
	/// </summary>
	/// <param name="toRight">右へ動くか</param>
	void SetPresetSlide(bool toRight);

	/// <summary>
	/// フェード演出をセット
	/// </summary>
	void SetPresetFade();

	/// <summary>
	/// 観音開き（中央から左右 / 左右から中央）演出をセット
	/// </summary>
	void SetPresetSplit();

	/// <summary>
	/// スライド（上：左から / 下：右から）演出をセット
	/// </summary>
	void SetPresetUpDownSlide();

	/// <summary>
	/// シーンタイプに基づいて演出を自動設定
	/// </summary>
	/// <param name="now"> 現在のシーンタイプ </param>
	/// <param name="next"> 次のシーンタイプ </param>
	void SetAutoPreset(SceneType now, SceneType next);

	/// <summary>
	/// 以前のシーンタイプに基づいて演出を自動設定 (Opening用)
	/// </summary>
	/// <param name="prev"> 以前のシーンタイプ </param>
	/// <param name="now"> 現在のシーンタイプ </param>
	void SetAutoPresetFromPrevious(SceneType prev, SceneType now);

	// --- 状態取得 ---
	TransitionState GetState() const { return state_; }
	bool			IsIdle() const { return state_ == TransitionState::Idle; }
	bool			IsOpening() const { return state_ == TransitionState::Opening; }
	bool			IsClosing() const { return state_ == TransitionState::Closing; }
	bool			IsFull() const { return state_ == TransitionState::Full; }

	CalyxEngine::SpriteObject2d* GetPlate1() const { return plate1_.get(); }
	CalyxEngine::SpriteObject2d* GetPlate2() const { return plate2_.get(); }

	void SetTexturePlate1(const std::string& texPath1);
	void SetTexturePlate2(const std::string& texPath2);
	void SetTexturePlate3(const std::string& texPath3);

private:
	void InitAnim();

	TransitionState state_ = TransitionState::Idle;

	std::unique_ptr<CalyxEngine::SpriteObject2d> plate1_ = nullptr;
	std::unique_ptr<CalyxEngine::SpriteObject2d> plate2_ = nullptr;
	std::unique_ptr<CalyxEngine::SpriteObject2d> plate3_ = nullptr;
	std::unique_ptr<CalyxEngine::SpriteAnimator2d> plateAnim1_ = nullptr;
	std::unique_ptr<CalyxEngine::SpriteAnimator2d> plateAnim2_ = nullptr;
	std::unique_ptr<CalyxEngine::SpriteAnimator2d> plateAnim3_ = nullptr;

	float timer_	= 0.0f;
	float duration_ = 1.0f;
	bool  isDrawPlate2_ = false;
	bool  isDrawPlate3_ = false;

	std::function<void(float progress, CalyxEngine::SpriteObject2d* plate1, CalyxEngine::SpriteObject2d* plate2, CalyxEngine::SpriteObject2d* plate3)> updateFunc_;
	std::function<void()>												onFinishedCallback_ = nullptr;
};
