#pragma once
#include <algorithm>
#include <chrono>

class ClockManager {
public:
	// シングルトン取得メソッド
	static ClockManager* GetInstance() {
		static ClockManager instance; // 唯一のインスタンス
		return &instance;
	}

	// コピーコンストラクタと代入演算子を削除
	ClockManager(const ClockManager&)	= delete;
	void operator=(const ClockManager&) = delete;

	void  Update(); // 毎フレーム呼び出し
	float GetRawDeltaTime() const;

	// グローバルな deltaTime（ヒットストップの影響を受けない）
	float GetDeltaTime() const { return globalDeltaTime_; }

	// プレイヤー関連用の deltaTime（ヒットストップの影響を受ける）
	float GetPlayerDeltaTime() const { return playerDeltaTime_; }

	void  SetTimeScale(float scale) { currentTimeScale_ = scale; }
	float GetTimeScale() const { return currentTimeScale_; }

	// スローモーション（ヒットストップとは異なり、playerDeltaTimeを直接スケールする）
	void StartSlowMotion(float scale, float duration);

	float GetTotalTime() const { return totalTime_; }
	float GetCurrentFPS() const { return currentFPS_; }
	float GetAverageFPS() const { return averageFPS_; }

	// ヒットストップ開始（イージングは使用しない）
	void StartHitStop(float duration);

private:
	// コンストラクタをprivate化
	ClockManager();

	std::chrono::high_resolution_clock::time_point firstFrameTime_;
	std::chrono::high_resolution_clock::time_point lastFrameTime_;
	float										   rawDeltaTime_ = 0.0f;
	// グローバル deltaTime（ヒットストップ影響なし）
	float globalDeltaTime_ = 0.016f;
	// プレイヤー用 deltaTime（ヒットストップでスケールされる）
	float playerDeltaTime_ = 0.016f;

	float			   totalTime_  = 0.0f;
	float			   currentFPS_ = 60.0f;
	float			   averageFPS_ = 60.0f;
	unsigned long long frameCount_ = 0;

	// ヒットストップ関連
	bool  isHitStopActive_	= false;
	float hitStopDuration_	= 0.0f;
	float hitStopElapsed_	= 0.0f;
	float normalTimeScale_	= 1.0f;
	float currentTimeScale_ = 1.0f;

	// スローモーション関連
	bool  isSlowMotionActive_ = false;
	float slowMotionDuration_ = 0.0f;
	float slowMotionElapsed_  = 0.0f;
	float slowMotionScale_	  = 1.0f;
};