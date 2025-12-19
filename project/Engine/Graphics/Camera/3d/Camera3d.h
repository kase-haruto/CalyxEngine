#pragma once
/* ========================================================================
/*          include space
/* ===================================================================== */
#include <Engine/Graphics/Camera/Base/BaseCamera.h>
#include <Engine/Graphics/Camera/Frustum/Frustum.h>

// fwd
class WorldTransform;

/* ========================================================================
/*          メインカメラ
/* ===================================================================== */
class Camera3d : public BaseCamera {
public:
    //==================================================================*//
    //          public functions
    //==================================================================*//
    Camera3d();
    Camera3d(const std::string& name);
    ~Camera3d() override = default;

    void Initialize();
    void AlwaysUpdate(float dt) override;
    void ShowGui() override;

    //--------- accessor -----------------------------------------------------
    bool IsVisible(const class AABB& aabb) const;
    std::string_view GetTypeName() const override { return "Camera3d"; }

    //--------- follow target -------------------------------------------------
    void SetFollowTarget(const WorldTransform* wt) { follow_.target = wt; }
    const WorldTransform* GetFollowTarget() const { return follow_.target; }
    void EnableFollow(bool e) { follow_.enabled = e; }
    bool IsFollowEnabled() const { return follow_.enabled; }

private:
    //==================================================================*//
    //          private functions
    //==================================================================*//
    void UpdateFollow(float dt);

    // ベクトル版 SmoothDamp（Unity 近似）
    static CalyxMath::Vector3 SmoothDampVec(const CalyxMath::Vector3& current,
                                 const CalyxMath::Vector3& target,
                                 CalyxMath::Vector3& currentVelocity,
                                 float smoothTime, float dt);

    // 回転の指数補間率（0..1）
    static float ExpLerpAlpha(float dt, float tau);

private:
    Frustum frustum_; // 視錐台

    //======================= 追従用データ ==============================
    struct FollowSettings {
        bool   enabled          = true;            // 有効/無効
        float  distanceBack     = 13.0f;             // 後方距離（-F * distanceBack）
        float  heightOffset     = 4.0f;             // 上方向(Y)オフセット
        float  sideOffset       = 0.0f;             // 右(+)左(-)オフセット
        CalyxMath::Vector3 lookAtOffset    = {0.0f, 1.5f, 0.0f}; // 必要なら使用

        // 位置スムージング
        float  posSmoothTime    = 0.78f;
        // 回転スムージング（時定数）
        float  rotTimeConstant  = 0.52f;

        // 俯角（ターゲットの forward を向きつつ少し下を見る）
        float  extraPitchDeg    = -10.0f;

        const WorldTransform* target = nullptr;     // 追従対象
        CalyxMath::Vector3 posVel = {0,0,0};                   // SmoothDamp 用速度
    } follow_;
};