#pragma once
/* ========================================================================
/*          include space
/* ===================================================================== */
#include <Engine/Graphics/Camera/Base/BaseCamera.h>
#include <Engine/Graphics/Camera/Frustum/Frustum.h>

// fwd
class WorldTransform;

/*-----------------------------------------------------------------------------------------
 * Camera3d
 * - メインカメラクラス
 * - 追従機能、視錐台カリング、定数バッファの更新などを計算する
 *---------------------------------------------------------------------------------------*/
class Camera3d : public BaseCamera {
public:
    //==================================================================*//
    //          public functions
    //==================================================================*//
    /**
     * \brief コンストラクタ
     */
    Camera3d();
    /**
     * \brief コンストラクタ
     * \param name カメラ名
     */
    Camera3d(const std::string& name);
    /**
     * \brief デストラクタ
     */
    ~Camera3d() override = default;

    /**
     * \brief 初期化
     */
    void Initialize();
    /**
     * \brief 常に実行される更新処理
     * \param dt デルタタイム
     */
    void AlwaysUpdate(float dt) override;
    /**
     * \brief GUI表示
     */
    void ShowGui() override;
    /**
     * \brief シャドウ用視錐台の四隅を取得
     * \param outCorners 出力先
     * \param shadowFar 遠方距離
     */
	void GetShadowFrustumCorners(CalyxMath::Vector3 outCorners[8], float shadowFar) const;
    //--------- accessor -----------------------------------------------------
    /**
     * \brief AABBが視野内か
     * \param aabb 判定対象
     * \return 視野内か
     */
    bool IsVisible(const class AABB& aabb) const;
    /**
     * \brief タイプ名を取得
     * \return タイプ名
     */
    std::string_view GetTypeName() const override { return "Camera3d"; }

    //--------- follow target -------------------------------------------------
    /**
     * \brief 追従対象を設定
     * \param wt ターゲットのトランスフォーム
     */
    void SetFollowTarget(const WorldTransform* wt) { follow_.target = wt; }
    /**
     * \brief 追従対象を取得
     * \return 追従対象
     */
    const WorldTransform* GetFollowTarget() const { return follow_.target; }
    /**
     * \brief 追従の有効/無効を設定
     * \param e 有効か
     */
    void EnableFollow(bool e) { follow_.enabled = e; }
    /**
     * \brief 追従が有効か
     * \return 有効か
     */
    bool IsFollowEnabled() const { return follow_.enabled; }
    /**
     * \brief 前方ベクトルを取得
     * \return 前方ベクトル
     */
	CalyxMath::Vector3 GetForward()const;

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
	//==================================================================*//
	//          private variables
	//==================================================================*//
	struct FollowSettings {
		const WorldTransform* target = nullptr; //< 追従対象
		float				  distance = 13.0f; //< 追従距離
		float				  heightOffset = 13.0f; //< 高さオフセット
		float				  smoothTime   = 0.3f; //< スムーズ時間
		CalyxMath::Vector3	  velocity	   = {0.0f, 0.0f, 0.0f}; //< 速度ベクトル（内部計算用）
		bool				  enabled	   = true; //< 有効フラグ
		float				  extraPitchDeg = -10.0f; //< 俯角
	} follow_; //< 追従設定

	CalyxMath::Vector4 frustumPlanes_[6]; //< 視錐台平面（カリング用）
};