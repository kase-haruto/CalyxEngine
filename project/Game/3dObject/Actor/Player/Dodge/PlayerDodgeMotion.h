#pragma once
#include <memory>
#include <cmath>
#include <numbers>
#include <algorithm>

/* engine */
#include <Engine/Foundation/Math/Vector3.h>
#include <Engine/Foundation/Math/Quaternion.h>
#include <Engine/Graphics/Camera/Manager/CameraManager.h>

/* ease */
#include <Engine/Foundation/Utility/Ease/Ease.h>

class Player;
class PlayerDodge;

class PlayerDodgeMotion {
public:
    PlayerDodgeMotion();
    ~PlayerDodgeMotion();

    void Initialize(Player* owner, PlayerDodge* dodge);
    void Update(float dt);

private:
    void OnDodgeStart();
    void OnDodgeEnd();
    void OnPerfect();

    void ApplySpinAndCurve(float dt);
    void ApplyProceduralPose(float dt);

private:
    Player* owner_ = nullptr;
    PlayerDodge* dodge_ = nullptr;

    // ==== 姿勢系 ====
    Quaternion baseRot_{ 0,0,0,1 }; // 回避開始時の基準姿勢
    Quaternion spinQ_{ 0,0,0,1 };   // そのフレームの絶対スピン回転

    float additiveRoll_ = 0.0f; // Z傾き
    float additivePitch_ = 0.0f; // X傾き
    float leanLerp_ = 0.0f; // 傾きのブレンド係数

    // ==== 位置サーボ ====
    Vector3 appliedOffset_{ 0,0,0 }; 

    // ==== 沈み（Y） ====
    float sinkCurrent_ = 0.0f;
};
