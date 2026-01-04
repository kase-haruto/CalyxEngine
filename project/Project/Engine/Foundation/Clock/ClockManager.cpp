#include "ClockManager.h"
#include <algorithm>
#include <chrono>

ClockManager::ClockManager(){
    firstFrameTime_ = std::chrono::high_resolution_clock::now();
    lastFrameTime_ = firstFrameTime_;
}

void ClockManager::Update(){
    auto currentFrameTime = std::chrono::high_resolution_clock::now();

    // 実際の deltaTime を計算（秒単位）
    float dt = std::chrono::duration<float, std::milli>(currentFrameTime - lastFrameTime_).count() / 1000.0f;
    // 急激な dt の変動を防ぐための clamp（グローバル更新用）
    dt = std::clamp(dt, 0.0f, 0.017f);

    // グローバルな deltaTime はそのまま dt を使用
    globalDeltaTime_ = dt;

    // FPS 計算（dt が 0 でないことを前提）
    if (dt > 0.0f){
        currentFPS_ = 1.0f / dt;
    }

    // ヒットストップ処理（プレイヤー用 time scale の計算）
    if (isHitStopActive_){
        hitStopElapsed_ += dt;
        // イージングなし：ヒットストップ中は一定のスローモーション倍率を適用（例：0.1倍速）
        currentTimeScale_ = 0.1f;

        if (hitStopElapsed_ >= hitStopDuration_){
            isHitStopActive_ = false;
            currentTimeScale_ = normalTimeScale_;
        }
    } else{
        currentTimeScale_ = normalTimeScale_;
    }

    // プレイヤー用 deltaTime は、global dt に現在の time scale を掛ける
    playerDeltaTime_ = dt * currentTimeScale_;

    // totalTime の更新（秒単位）
    totalTime_ = std::chrono::duration<float>(currentFrameTime - firstFrameTime_).count();

    frameCount_++;
    if (totalTime_ > 0.0f){
        averageFPS_ = static_cast< float >(frameCount_) / totalTime_;
    }

    lastFrameTime_ = currentFrameTime;
}

void ClockManager::StartHitStop(float duration){
    isHitStopActive_ = true;
    hitStopDuration_ = duration;
    hitStopElapsed_ = 0.0f;
}
